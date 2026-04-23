#include "Camera.h"
#include "Renderer.h"
#include "Shader.h"
#include "glm/gtc/type_ptr.inl"
#include <glm/gtx/vector_angle.hpp>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace
{
	constexpr int kPickingDownsample = 1;
	constexpr std::size_t kPickPixelSizeBytes = 4;

	struct FrustumPlane
	{
		glm::vec3 normal;
		float distance;
	};

	using FrustumPlanes = std::array<FrustumPlane, 6>;

	FrustumPlane NormalizePlane(const glm::vec4 &plane)
	{
		const glm::vec3 normal(plane.x, plane.y, plane.z);
		const float magnitude = glm::length(normal);
		if (magnitude <= 0.0f)
		{
			return {glm::vec3(0.0f, 1.0f, 0.0f), 0.0f};
		}

		return {normal / magnitude, plane.w / magnitude};
	}

	FrustumPlanes BuildFrustumPlanes(const glm::mat4 &viewProjection)
	{
		const glm::vec4 row0(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0], viewProjection[3][0]);
		const glm::vec4 row1(viewProjection[0][1], viewProjection[1][1], viewProjection[2][1], viewProjection[3][1]);
		const glm::vec4 row2(viewProjection[0][2], viewProjection[1][2], viewProjection[2][2], viewProjection[3][2]);
		const glm::vec4 row3(viewProjection[0][3], viewProjection[1][3], viewProjection[2][3], viewProjection[3][3]);

		return {
			NormalizePlane(row3 + row0),
			NormalizePlane(row3 - row0),
			NormalizePlane(row3 + row1),
			NormalizePlane(row3 - row1),
			NormalizePlane(row3 + row2),
			NormalizePlane(row3 - row2)};
	}

	bool IsSphereVisible(const FrustumPlanes &planes, const glm::vec3 &center, float radius)
	{
		for (const auto &plane : planes)
		{
			const float distance = glm::dot(plane.normal, center) + plane.distance;
			if (distance < -radius)
			{
				return false;
			}
		}
		return true;
	}

	struct CachedMesh
	{
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ibo = 0;
		unsigned int indexCount = 0;

		CachedMesh() = default;

		CachedMesh(const std::vector<GLfloat> &vertices, const std::vector<unsigned int> &indices)
		{
			Upload(vertices, indices);
		}

		CachedMesh(const CachedMesh &) = delete;
		CachedMesh &operator=(const CachedMesh &) = delete;

		CachedMesh(CachedMesh &&other) noexcept
		{
			MoveFrom(other);
		}

		CachedMesh &operator=(CachedMesh &&other) noexcept
		{
			if (this != &other)
			{
				Release();
				MoveFrom(other);
			}
			return *this;
		}

		~CachedMesh()
		{
			Release();
		}

		void Upload(const std::vector<GLfloat> &vertices, const std::vector<unsigned int> &indices)
		{
			Release();
			if (vertices.empty() || indices.empty())
			{
				return;
			}

			indexCount = static_cast<unsigned int>(indices.size());
			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glGenBuffers(1, &ibo);

			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)), vertices.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

			const GLsizei stride = static_cast<GLsizei>(8 * sizeof(GLfloat));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(0));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(3 * sizeof(GLfloat)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void *>(6 * sizeof(GLfloat)));

			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		void Draw() const
		{
			if (vao == 0 || ibo == 0 || indexCount == 0)
			{
				return;
			}

			glBindVertexArray(vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
		}

		void Release()
		{
			if (ibo != 0)
			{
				glDeleteBuffers(1, &ibo);
				ibo = 0;
			}
			if (vbo != 0)
			{
				glDeleteBuffers(1, &vbo);
				vbo = 0;
			}
			if (vao != 0)
			{
				glDeleteVertexArrays(1, &vao);
				vao = 0;
			}
			indexCount = 0;
		}

		void MoveFrom(CachedMesh &other)
		{
			vao = other.vao;
			vbo = other.vbo;
			ibo = other.ibo;
			indexCount = other.indexCount;

			other.vao = 0;
			other.vbo = 0;
			other.ibo = 0;
			other.indexCount = 0;
		}
	};

	CachedMesh CreateCubeMesh()
	{
		const std::vector<GLfloat> vertices = {
			0.5f, 0.5f, 0.5f, 1.f, 0.f, 0.f, 0.0f, 0.0f,
			-0.5f, 0.5f, 0.5f, 0.f, 1.f, 0.f, 5.0f, 0.0f,
			-0.5f, -0.5f, 0.5f, 0.f, 0.f, 1.f, 0.0f, 0.0f,
			0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.f, 5.0f, 0.0f,
			0.5f, 0.5f, -0.5f, 1.f, 0.f, 0.f, 0.0f, 0.0f,
			-0.5f, 0.5f, -0.5f, 0.f, 1.f, 0.f, 5.0f, 0.0f,
			-0.5f, -0.5f, -0.5f, 0.f, 0.f, 1.f, 0.0f, 0.0f,
			0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.f, 5.0f, 0.0f};

		const std::vector<unsigned int> indices = {
			0, 1, 2, 2, 3, 0,
			0, 3, 7, 7, 4, 0,
			2, 6, 7, 7, 3, 2,
			1, 5, 6, 6, 2, 1,
			4, 7, 6, 6, 5, 4,
			5, 1, 0, 0, 4, 5};

		return CachedMesh(vertices, indices);
	}

	CachedMesh CreatePyramidMesh()
	{
		const std::vector<GLfloat> vertices = {
			-0.5f, 0.0f, 0.5f, 0.f, 1.f, 0.f, 0.0f, 0.0f,
			-0.5f, 0.0f, -0.5f, 0.f, 0.f, 1.f, 5.0f, 0.0f,
			0.5f, 0.0f, -0.5f, 1.f, 0.f, 0.f, 0.0f, 0.0f,
			0.5f, 0.0f, 0.5f, 0.f, 1.f, 0.f, 5.0f, 0.0f,
			0.0f, 0.8f, 0.0f, 0.f, 0.f, 1.f, 2.5f, 5.0f};

		const std::vector<unsigned int> indices = {
			0, 1, 2,
			0, 2, 3,
			0, 1, 4,
			1, 2, 4,
			2, 3, 4,
			3, 0, 4};

		return CachedMesh(vertices, indices);
	}

	CachedMesh CreateSphereMesh()
	{
		constexpr int sectorCount = 24;
		constexpr int stackCount = 16;
		constexpr float radius = 0.5f;

		std::vector<GLfloat> vertices;
		std::vector<unsigned int> indices;
		vertices.reserve((stackCount + 1) * (sectorCount + 1) * 8);

		const float pi = 3.14159265359f;
		for (int stack = 0; stack <= stackCount; ++stack)
		{
			const float stackAngle = pi / 2.0f - static_cast<float>(stack) * (pi / static_cast<float>(stackCount));
			const float xy = radius * std::cos(stackAngle);
			const float z = radius * std::sin(stackAngle);

			for (int sector = 0; sector <= sectorCount; ++sector)
			{
				const float sectorAngle = static_cast<float>(sector) * (2.0f * pi / static_cast<float>(sectorCount));
				const float x = xy * std::cos(sectorAngle);
				const float y = xy * std::sin(sectorAngle);
				const glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
				const float u = static_cast<float>(sector) / static_cast<float>(sectorCount);
				const float v = static_cast<float>(stack) / static_cast<float>(stackCount);

				vertices.push_back(x);
				vertices.push_back(z);
				vertices.push_back(y);
				vertices.push_back(normal.x);
				vertices.push_back(normal.z);
				vertices.push_back(normal.y);
				vertices.push_back(u);
				vertices.push_back(v);
			}
		}

		for (int stack = 0; stack < stackCount; ++stack)
		{
			const int k1 = stack * (sectorCount + 1);
			const int k2 = k1 + sectorCount + 1;

			for (int sector = 0; sector < sectorCount; ++sector)
			{
				if (stack != 0)
				{
					indices.push_back(static_cast<unsigned int>(k1 + sector));
					indices.push_back(static_cast<unsigned int>(k2 + sector));
					indices.push_back(static_cast<unsigned int>(k1 + sector + 1));
				}

				if (stack != (stackCount - 1))
				{
					indices.push_back(static_cast<unsigned int>(k1 + sector + 1));
					indices.push_back(static_cast<unsigned int>(k2 + sector));
					indices.push_back(static_cast<unsigned int>(k2 + sector + 1));
				}
			}
		}

		return CachedMesh(vertices, indices);
	}

	class CubeRenderStrategy final : public IShapeRenderStrategy
	{
	private:
		CachedMesh m_Mesh;

	public:
		CubeRenderStrategy()
			: m_Mesh(CreateCubeMesh())
		{
		}

		void Draw(const Shader &shader) const override
		{
			(void)shader;
			m_Mesh.Draw();
		}

		float PickRadius(float scale) const override
		{
			return 0.9f * scale;
		}
	};

	class PyramidRenderStrategy final : public IShapeRenderStrategy
	{
	private:
		CachedMesh m_Mesh;

	public:
		PyramidRenderStrategy()
			: m_Mesh(CreatePyramidMesh())
		{
		}

		void Draw(const Shader &shader) const override
		{
			(void)shader;
			m_Mesh.Draw();
		}

		float PickRadius(float scale) const override
		{
			return 0.9f * scale;
		}
	};

	class SphereRenderStrategy final : public IShapeRenderStrategy
	{
	private:
		CachedMesh m_Mesh;

	public:
		SphereRenderStrategy()
			: m_Mesh(CreateSphereMesh())
		{
		}

		void Draw(const Shader &shader) const override
		{
			(void)shader;
			m_Mesh.Draw();
		}

		float PickRadius(float scale) const override
		{
			return 0.6f * scale;
		}
	};
} // namespace

static std::string DebugConvertSeverityToString(GLenum severity)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		return "GL_DEBUG_SEVERITY_LOW:Redundant state change performance warning, or unimportant undefined behavior";
	case GL_DEBUG_SEVERITY_MEDIUM:
		return "GL_DEBUG_SEVERITY_MEDIUM:Major performance warnings, shader compilation/linking warnings, or the use of deprecated functionality";
	case GL_DEBUG_SEVERITY_HIGH:
		return "GL_DEBUG_SEVERITY_HIGH:All OpenGL Errors, shader compilation/linking errors, or highly-dangerous undefined behavior";
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		return "GL_DEBUG_SEVERITY_NOTIFICATION:Anything that isn't an error or performance issue.";
	}
	return "GL_DEBUG_SEVERITY_UNKNOWN";
}

static std::string DebugConvertSourceToString(GLenum Source)
{
	switch (Source)
	{
	case GL_DEBUG_SOURCE_API:
		return "GL_DEBUG_SOURCE_API: Calls to the OpenGL API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return "GL_DEBUG_SOURCE_WINDOW_SYSTEM: Calls to a window-system API";
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return "GL_DEBUG_SOURCE_SHADER_COMPILER: A compiler for a shading language";
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return "GL_DEBUG_SOURCE_THIRD_PARTY:An application associated with OpenGL";
	case GL_DEBUG_SOURCE_APPLICATION:
		return "GL_DEBUG_SOURCE_APPLICATION:Generated by the user of this application";
	case GL_DEBUG_SOURCE_OTHER:
		return "GL_DEBUG_SOURCE_OTHER:Some source that isn't one of these";
	}
	return "GL_DEBUG_SOURCE_UNKNOWN";
}

static std::string DebugConvertTypeToString(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		return "GL_DEBUG_TYPE_ERROR: An error, typically from the API";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR	: Some behavior marked deprecated has been used";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: Something has invoked undefined behavior";
	case GL_DEBUG_TYPE_PORTABILITY:
		return "GL_DEBUG_TYPE_PORTABILITY:Some functionality the user relies upon is not portable";
	case GL_DEBUG_TYPE_PERFORMANCE:
		return "GL_DEBUG_TYPE_PERFORMANCE:Code has triggered possible performance issues";
	case GL_DEBUG_TYPE_MARKER:
		return "GL_DEBUG_TYPE_MARKER:Command stream annotation";
	case GL_DEBUG_TYPE_POP_GROUP:
		return "GL_DEBUG_TYPE_POP_GROUP:Group popping";
	case GL_DEBUG_TYPE_PUSH_GROUP:
		return "GL_DEBUG_TYPE_PUSH_GROUP:Group pushing";
	default:
		return "GL_DEBUG_TYPE_OTHER: Some type that isn't one of these";
	}
}

void Renderer::Clear()
{
	// Specify the color of the background
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	// Clean the back buffer, depth buffer, and stencil buffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

Renderer::Renderer()
{
	m_DrawStrategies.emplace(Shape::Cube, std::make_unique<CubeRenderStrategy>());
	m_DrawStrategies.emplace(Shape::Pyramid, std::make_unique<PyramidRenderStrategy>());
	m_DrawStrategies.emplace(Shape::Sphere, std::make_unique<SphereRenderStrategy>());
}

Renderer::~Renderer()
{
	if (m_PendingPickFence != nullptr)
	{
		glDeleteSync(m_PendingPickFence);
		m_PendingPickFence = nullptr;
	}
	if (m_PickingPbos[0] != 0 || m_PickingPbos[1] != 0)
	{
		glDeleteBuffers(2, m_PickingPbos);
		m_PickingPbos[0] = 0;
		m_PickingPbos[1] = 0;
	}
	if (m_PickingDepthRbo != 0)
	{
		glDeleteRenderbuffers(1, &m_PickingDepthRbo);
	}
	if (m_PickingColorTex != 0)
	{
		glDeleteTextures(1, &m_PickingColorTex);
	}
	if (m_PickingFbo != 0)
	{
		glDeleteFramebuffers(1, &m_PickingFbo);
	}
}

void Renderer::DrawByShape(Shape shape, const Shader &shader)
{
	const auto strategy = m_DrawStrategies.find(shape);
	if (strategy != m_DrawStrategies.end())
	{
		strategy->second->Draw(shader);
	}
}

void Renderer::Draw(Shader &shader, Shader &outlineShader, Camera &camera, float camera_angle, int selectedObjectId)
{
	glm::mat4 cam_mat4 = camera.Matrix(camera_angle, 0.1f, 100.f);
	const FrustumPlanes frustumPlanes = BuildFrustumPlanes(cam_mat4);
	const Info *selectedObject = nullptr;
	glStencilMask(0x00);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);
	shader.Bind();

	for (auto &m_Object : m_Objects)
	{
		const auto strategy = m_DrawStrategies.find(m_Object.shape);
		if (strategy == m_DrawStrategies.end())
		{
			continue;
		}

		const float objectRadius = strategy->second->PickRadius(m_Object.scaleVal);
		if (!IsSphereVisible(frustumPlanes, m_Object.location, objectRadius))
		{
			continue;
		}

		const bool isSelected = (m_Object.id == selectedObjectId);
		if (isSelected)
		{
			selectedObject = &m_Object;
			glStencilMask(0xFF);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
		}
		else
		{
			glStencilMask(0x00);
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
		}

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, m_Object.location);
		model = glm::rotate(model, glm::radians(m_Object.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(m_Object.scaleVal));
		glm::mat4 mvp = cam_mat4 * model;

		shader.SetUniformMat4f("u_mvp", mvp);
		strategy->second->Draw(shader);
	}

	if (selectedObject != nullptr)
	{
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);

		const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
		const GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
		if (!cullFaceWasEnabled)
		{
			glEnable(GL_CULL_FACE);
		}
		if (!depthTestWasEnabled)
		{
			glEnable(GL_DEPTH_TEST);
		}
		glCullFace(GL_FRONT);

		outlineShader.Bind();
		glm::mat4 outlineModel = glm::mat4(1.0f);
		outlineModel = glm::translate(outlineModel, selectedObject->location);
		outlineModel = glm::rotate(outlineModel, glm::radians(selectedObject->rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
		outlineModel = glm::scale(outlineModel, glm::vec3(selectedObject->scaleVal * 1.08f));
		const glm::mat4 outlineMvp = cam_mat4 * outlineModel;
		outlineShader.SetUniformMat4f("u_mvp", outlineMvp);
		DrawByShape(selectedObject->shape, outlineShader);

		glCullFace(GL_BACK);
		if (!cullFaceWasEnabled)
		{
			glDisable(GL_CULL_FACE);
		}
		if (!depthTestWasEnabled)
		{
			glDisable(GL_DEPTH_TEST);
		}
		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
	}

	// Always restore a predictable stencil state for subsequent passes.
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 0, 0xFF);
}

void Renderer::EnsurePickingResources(int viewportWidth, int viewportHeight)

{
	const int desiredWidth = std::max(1, viewportWidth / kPickingDownsample);
	const int desiredHeight = std::max(1, viewportHeight / kPickingDownsample);

	GLint previousFbo = 0;
	GLint previousTexture2D = 0;
	GLint previousRenderbuffer = 0;
	GLint previousActiveTexture = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
	glActiveTexture(GL_TEXTURE0);

	if (m_PickingFbo == 0)
	{
		glGenFramebuffers(1, &m_PickingFbo);
		glGenTextures(1, &m_PickingColorTex);
		glGenRenderbuffers(1, &m_PickingDepthRbo);
	}

	if (desiredWidth == m_PickingWidth && desiredHeight == m_PickingHeight)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, previousFbo);
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture2D));
		glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
		glActiveTexture(static_cast<GLenum>(previousActiveTexture));
		return;
	}

	m_PickingWidth = desiredWidth;
	m_PickingHeight = desiredHeight;

	glBindFramebuffer(GL_FRAMEBUFFER, m_PickingFbo);

	glBindTexture(GL_TEXTURE_2D, m_PickingColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_PickingWidth, m_PickingHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PickingColorTex, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, m_PickingDepthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_PickingWidth, m_PickingHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_PickingDepthRbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "GPU picking framebuffer is incomplete" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, previousFbo);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture2D));
	glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
	glActiveTexture(static_cast<GLenum>(previousActiveTexture));
}

void Renderer::EnsurePickingReadbackBuffers()
{
	if (m_PickingPbos[0] != 0 && m_PickingPbos[1] != 0)
	{
		return;
	}

	GLint previousPixelPackBuffer = 0;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
	glGenBuffers(2, m_PickingPbos);
	for (int i = 0; i < 2; ++i)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PickingPbos[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(kPickPixelSizeBytes), nullptr, GL_STREAM_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPackBuffer));
}

int Renderer::ValidatePickedId(int pickedId) const
{
	if (pickedId == -1)
	{
		return -1;
	}

	for (const auto &object : m_Objects)
	{
		if (object.id == pickedId)
		{
			return pickedId;
		}
	}

	return -1;
}

glm::vec4 Renderer::EncodeObjectIdColor(int objectId)

{
	const unsigned int uid = static_cast<unsigned int>(objectId);
	const float r = static_cast<float>((uid & 0x000000FFu)) / 255.0f;
	const float g = static_cast<float>((uid & 0x0000FF00u) >> 8) / 255.0f;
	const float b = static_cast<float>((uid & 0x00FF0000u) >> 16) / 255.0f;
	return glm::vec4(r, g, b, 1.0f);
}

int Renderer::DecodeObjectIdColor(unsigned char r, unsigned char g, unsigned char b)

{
	const int id = static_cast<int>(r) | (static_cast<int>(g) << 8) | (static_cast<int>(b) << 16);
	return id == 0 ? -1 : id;
}

void Renderer::QueuePickRequestGpu(Shader &pickingShader, Camera &camera, float camera_angle, int viewportWidth, int viewportHeight, double mouseX, double mouseY)

{
	if (viewportWidth <= 0 || viewportHeight <= 0)
	{
		return;
	}

	EnsurePickingResources(viewportWidth, viewportHeight);
	EnsurePickingReadbackBuffers();

	if (m_PendingPickFence != nullptr)
	{
		glDeleteSync(m_PendingPickFence);
		m_PendingPickFence = nullptr;
		m_PendingPickPboIndex = -1;
	}

	GLint previousFbo = 0;
	GLint previousViewport[4] = {0, 0, 0, 0};
	GLint previousReadBuffer = 0;
	GLint previousPolygonMode[2] = {GL_FILL, GL_FILL};
	GLint previousActiveTexture = 0;
	GLint previousTexture2D = 0;
	GLint previousPixelPackBuffer = 0;
	GLfloat previousClearColor[4] = {0.f, 0.f, 0.f, 1.f};
	const GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean previousStencilTest = glIsEnabled(GL_STENCIL_TEST);
	const GLboolean previousCullFace = glIsEnabled(GL_CULL_FACE);
	const GLboolean previousBlend = glIsEnabled(GL_BLEND);
	const GLboolean previousDither = glIsEnabled(GL_DITHER);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
	glGetIntegerv(GL_VIEWPORT, previousViewport);
	glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);
	glGetIntegerv(GL_POLYGON_MODE, previousPolygonMode);
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);

	glBindFramebuffer(GL_FRAMEBUFFER, m_PickingFbo);
	glViewport(0, 0, m_PickingWidth, m_PickingHeight);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	const glm::mat4 cam_mat4 = camera.Matrix(camera_angle, 0.1f, 100.f);
	const FrustumPlanes frustumPlanes = BuildFrustumPlanes(cam_mat4);
	pickingShader.Bind();
	for (const auto &object : m_Objects)
	{
		const auto strategy = m_DrawStrategies.find(object.shape);
		if (strategy == m_DrawStrategies.end())
		{
			continue;
		}

		const float objectRadius = strategy->second->PickRadius(object.scaleVal);
		if (!IsSphereVisible(frustumPlanes, object.location, objectRadius))
		{
			continue;
		}

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, object.location);
		model = glm::rotate(model, glm::radians(object.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(object.scaleVal));
		const glm::mat4 mvp = cam_mat4 * model;
		const glm::vec4 idColor = EncodeObjectIdColor(object.id);

		pickingShader.SetUniformMat4f("u_mvp", mvp);
		pickingShader.SetUniform4f("u_IdColor", idColor.r, idColor.g, idColor.b, 1.0f);
		strategy->second->Draw(pickingShader);
	}

	const int pickX = std::clamp(static_cast<int>(mouseX * static_cast<double>(m_PickingWidth) / static_cast<double>(viewportWidth)), 0, m_PickingWidth - 1);
	const int pickY = std::clamp(m_PickingHeight - 1 - static_cast<int>(mouseY * static_cast<double>(m_PickingHeight) / static_cast<double>(viewportHeight)), 0, m_PickingHeight - 1);

	const int pboIndex = m_PickingPboWriteIndex;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PickingPbos[pboIndex]);
	glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(kPickPixelSizeBytes), nullptr, GL_STREAM_READ);
	glReadPixels(pickX, pickY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	m_PendingPickFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	m_PendingPickPboIndex = pboIndex;
	m_PickingPboWriteIndex = (m_PickingPboWriteIndex + 1) % 2;

	glBindFramebuffer(GL_FRAMEBUFFER, previousFbo);
	glReadBuffer(previousReadBuffer);
	glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(previousPolygonMode[0]));
	glActiveTexture(static_cast<GLenum>(previousActiveTexture));
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture2D));
	glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPackBuffer));
	glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
	if (previousDepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	if (previousStencilTest)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);
	if (previousCullFace)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	if (previousBlend)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
	if (previousDither)
		glEnable(GL_DITHER);
	else
		glDisable(GL_DITHER);
	glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

bool Renderer::TryConsumePickResultGpu(int &outObjectId)
{
	outObjectId = -1;
	if (m_PendingPickFence == nullptr || m_PendingPickPboIndex < 0)
	{
		return false;
	}

	const GLenum waitResult = glClientWaitSync(m_PendingPickFence, 0, 0);
	if (waitResult == GL_TIMEOUT_EXPIRED)
	{
		return false;
	}

	if (waitResult == GL_WAIT_FAILED)
	{
		glDeleteSync(m_PendingPickFence);
		m_PendingPickFence = nullptr;
		m_PendingPickPboIndex = -1;
		return true;
	}

	GLint previousPixelPackBuffer = 0;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPixelPackBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PickingPbos[m_PendingPickPboIndex]);
	unsigned char *pixel = static_cast<unsigned char *>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, static_cast<GLsizeiptr>(kPickPixelSizeBytes), GL_MAP_READ_BIT));
	if (pixel != nullptr)
	{
		outObjectId = ValidatePickedId(DecodeObjectIdColor(pixel[0], pixel[1], pixel[2]));
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPixelPackBuffer));

	glDeleteSync(m_PendingPickFence);
	m_PendingPickFence = nullptr;
	m_PendingPickPboIndex = -1;
	return true;
}

void Renderer::DrawCube(const Shader &shader)
{
	const auto strategy = m_DrawStrategies.find(Shape::Cube);
	if (strategy != m_DrawStrategies.end())
	{
		strategy->second->Draw(shader);
	}
}

void Renderer::DrawPyramid(const Shader &shader)
{
	const auto strategy = m_DrawStrategies.find(Shape::Pyramid);
	if (strategy != m_DrawStrategies.end())
	{
		strategy->second->Draw(shader);
	}
}

void Renderer::DrawSphere(const Shader &shader)
{
	const auto strategy = m_DrawStrategies.find(Shape::Sphere);
	if (strategy != m_DrawStrategies.end())
	{
		strategy->second->Draw(shader);
	}
}

int Renderer::AddObject(Shape shape, const glm::vec3 &location, float rotationY, float scaleVal)
{
	const int objectId = m_NextObjectId++;
	m_Objects.push_back({objectId, shape, rotationY, scaleVal, location});
	return objectId;
}

std::vector<Info> &Renderer::GetObjects()
{
	return m_Objects;
}

Info *Renderer::GetObjectById(int id)
{
	for (auto &object : m_Objects)
	{
		if (object.id == id)
			return &object;
	}

	return nullptr;
}

void Renderer::AutoRotateObjects(float deltaDegrees)
{
	for (auto &object : m_Objects)
	{
		object.rotationY += deltaDegrees;
		if (object.rotationY >= 360.0f)
			object.rotationY -= 360.0f;
	}
}

int Renderer::PickObject(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const
{
	float nearestT = std::numeric_limits<float>::max();
	int selectedId = -1;

	for (const auto &object : m_Objects)
	{
		float radius = 0.9f * object.scaleVal;
		const auto strategy = m_DrawStrategies.find(object.shape);
		if (strategy != m_DrawStrategies.end())
		{
			radius = strategy->second->PickRadius(object.scaleVal);
		}
		const glm::vec3 oc = rayOrigin - object.location;
		const float b = 2.0f * glm::dot(oc, rayDirection);
		const float c = glm::dot(oc, oc) - (radius * radius);
		const float discriminant = (b * b) - (4.0f * c);

		if (discriminant < 0.0f)
			continue;

		const float sqrtDisc = sqrtf(discriminant);
		const float t0 = (-b - sqrtDisc) * 0.5f;
		const float t1 = (-b + sqrtDisc) * 0.5f;
		const float t = (t0 > 0.0f) ? t0 : t1;

		if (t > 0.0f && t < nearestT)
		{
			nearestT = t;
			selectedId = object.id;
		}
	}

	return selectedId;
}

void APIENTRY openglCallbackFunction(GLenum source,
									 GLenum type,
									 GLuint id,
									 GLenum severity,
									 GLsizei length,
									 const GLchar *message,
									 const void *userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;

	std::cout << "[OpenGL ERROR]\nSource: " << DebugConvertSourceToString(source) << "\nType: " << DebugConvertTypeToString(type) << "\nID: " << id << "\nSeverity: " << DebugConvertSeverityToString(severity) << "\nMessage: " << message << std::endl;
	if (severity == GL_DEBUG_SEVERITY_HIGH)
		Break;
}

void glCheckError()
{
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}
