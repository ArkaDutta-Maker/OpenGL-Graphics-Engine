#pragma once

#include <glad/glad.h>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class Camera;
class Shader;
class VertexArray;
class IndexBuffer;

class Texture;
#define Break __debugbreak()

void APIENTRY openglCallbackFunction(GLenum source,
									 GLenum type,
									 GLuint id,
									 GLenum severity,
									 GLsizei length,
									 const GLchar *message,
									 const void *userParam);
void glCheckError();

static std::string DebugConvertSourceToString(GLenum Source);
static std::string DebugConvertSeverityToString(GLenum severity);
static std::string DebugConvertTypeToString(GLenum type);

enum class Shape
{
	Cube,
	Pyramid,
	Sphere
};

class IShapeRenderStrategy
{
public:
	virtual ~IShapeRenderStrategy() = default;
	virtual void Draw(const Shader &shader) const = 0;
	virtual float PickRadius(float scale) const = 0;
};

struct Info
{
	int id;
	Shape shape;
	float rotationY;
	float scaleVal;
	glm::vec3 location;
};

class Renderer
{
private:
	std::vector<Info> m_Objects;
	std::map<Shape, std::unique_ptr<IShapeRenderStrategy>> m_DrawStrategies;
	int m_NextObjectId = 1;
	unsigned int m_PickingFbo = 0;
	unsigned int m_PickingColorTex = 0;
	unsigned int m_PickingDepthRbo = 0;
	unsigned int m_PickingPbos[2] = {0, 0};
	GLsync m_PendingPickFence = nullptr;
	int m_PendingPickPboIndex = -1;
	int m_PickingPboWriteIndex = 0;
	int m_PickingWidth = 0;
	int m_PickingHeight = 0;

	void DrawByShape(Shape shape, const Shader &shader);
	void EnsurePickingResources(int viewportWidth, int viewportHeight);
	void EnsurePickingReadbackBuffers();
	int ValidatePickedId(int pickedId) const;
	static glm::vec4 EncodeObjectIdColor(int objectId);
	static int DecodeObjectIdColor(unsigned char r, unsigned char g, unsigned char b);

public:
	Renderer();
	~Renderer();
	void Clear();
	void Draw(Shader &shader, Shader &outlineShader, Camera &camera, float camera_angle, int selectedObjectId);
	void DrawCube(const Shader &shader);
	void DrawPyramid(const Shader &shader);
	void DrawSphere(const Shader &shader);
	int AddObject(Shape shape, const glm::vec3 &location, float rotationY = 0.0f, float scaleVal = 1.0f);
	std::vector<Info> &GetObjects();
	Info *GetObjectById(int id);
	void AutoRotateObjects(float deltaDegrees);
	int PickObject(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const;
	void QueuePickRequestGpu(Shader &pickingShader, Camera &camera, float camera_angle, int viewportWidth, int viewportHeight, double mouseX, double mouseY);
	bool TryConsumePickResultGpu(int &outObjectId);
};
