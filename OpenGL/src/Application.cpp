#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/type_ptr.inl"

int width = 800;
int height = 800;
bool show = false;

std::filesystem::path GetExecutableDirectory()
{
#ifdef _WIN32
	char buffer[MAX_PATH];
	DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	if (length == 0 || length == MAX_PATH)
	{
		return std::filesystem::current_path();
	}

	return std::filesystem::path(buffer).parent_path();
#else
	return std::filesystem::current_path();
#endif
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow *window, int m_width, int m_height)
{
	width = m_width;
	height = m_height;
	glViewport(0, 0, width, height);
}
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_RELEASE)
		return; // only handle press events
	if (key == GLFW_KEY_R)
		show = !show;
}
void Rotate(float &rotation, double &prevTime)
{

	double crntTime = glfwGetTime();
	if (rotation > 360.f)
	{
		rotation = 0.f;
	}
	if (crntTime - prevTime >= 1 / 60 && rotation <= 360.f)
	{
		rotation += 1.f;
		prevTime = crntTime;
	}
}
void OnMouse(GLFWwindow *window, double xpos, double ypos)
{
}

glm::vec3 ScreenPointToRay(GLFWwindow *window, const Camera &camera, float cameraAngleDeg)
{
	double mouseX = 0.0;
	double mouseY = 0.0;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float ndcX = (2.0f * static_cast<float>(mouseX) / static_cast<float>(width)) - 1.0f;
	float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(height));

	glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(cameraAngleDeg), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
	glm::vec4 rayEye = glm::inverse(projection) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

	glm::mat4 view = glm::lookAt(camera.Position, camera.Position + camera.Orientation, camera.Up);
	glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
	return rayWorld;
}

int main()
{
	GLFWwindow *window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	const char *glsl_version = "#version 460";

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "MyWindow", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	gladLoadGL();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glfwSwapInterval(1);
	glViewport(0, 0, width, height);
	std::cout << glGetString(GL_VERSION);
	auto executableDirectory = GetExecutableDirectory();
	auto resourceDirectory = executableDirectory / "res";

	glm::mat4 mvp = glm::mat4(1.f);

	auto fragmentShaderPath = (resourceDirectory / "Shader" / "Frag.frag").string();
	auto vertexShaderPath = (resourceDirectory / "Shader" / "Vertex.vert").string();
	auto pickFragmentShaderPath = (resourceDirectory / "Shader" / "PickFrag.frag").string();
	auto pickVertexShaderPath = (resourceDirectory / "Shader" / "PickVertex.vert").string();
	auto outlineFragmentShaderPath = (resourceDirectory / "Shader" / "OutlineFrag.frag").string();
	auto outlineVertexShaderPath = (resourceDirectory / "Shader" / "OutlineVertex.vert").string();
	Shader shader(fragmentShaderPath.c_str(), vertexShaderPath.c_str());
	Shader pickingShader(pickFragmentShaderPath.c_str(), pickVertexShaderPath.c_str());
	Shader outlineShader(outlineFragmentShaderPath.c_str(), outlineVertexShaderPath.c_str());
	shader.Bind();

	shader.SetUniformMat4f("u_mvp", mvp);

	shader.SetUniform1f("scale", 1.f);

	Texture texture((resourceDirectory / "Texture" / "brick.png").string());
	texture.Bind();
	shader.SetUniform1i("tex0", 0);
	// float r = 0.f;
	// float increment = 0.05f;
	shader.UnBind();
	Renderer renderer;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	// io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
	ImGui::StyleColorsDark();
	auto imguiIniPath = (executableDirectory / "imgui.ini").string();
	io.IniFilename = imguiIniPath.c_str();
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);

	ImGui_ImplOpenGL3_Init(glsl_version);

	glCheckError();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	// glfwSetCursorPosCallback(window, OnMouse);

	float camera_angle = 90.f;
	glfwSwapInterval(1);
	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	bool Wireframe = false;
	bool previousWireframe = !Wireframe;

	Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));
	bool AutoRotate = false;
	float autoRotateSpeed = 60.0f;
	renderer.AddObject(Shape::Cube, glm::vec3(0.f, 0.f, 0.f));
	double lastFrameTime = glfwGetTime();
	bool leftMouseWasDown = false;
	int selectedObjectId = -1;
	int i = 1;
	auto NextSpawnLocation = [&i]()
	{
		int col = i % 5;
		int row = i / 5;
		glm::vec3 location((col - 2) * 1.5f, 0.f, -row * 1.5f);
		i++;
		return location;
	};
	auto ShapeName = [](Shape shape)
	{
		switch (shape)
		{
		case Shape::Cube:
			return "Box";
		case Shape::Pyramid:
			return "Pyramid";
		case Shape::Sphere:
			return "Sphere";
		default:
			return "Object";
		}
	};
	while (!glfwWindowShouldClose(window))
	{

		processInput(window);

		double currentFrameTime = glfwGetTime();
		float deltaSeconds = static_cast<float>(currentFrameTime - lastFrameTime);
		lastFrameTime = currentFrameTime;

		if (AutoRotate)
		{
			renderer.AutoRotateObjects(autoRotateSpeed * deltaSeconds);
		}

		renderer.Clear();
		if (Wireframe != previousWireframe)
		{
			glPolygonMode(GL_FRONT_AND_BACK, Wireframe ? GL_LINE : GL_FILL);
			previousWireframe = Wireframe;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (!show)
		{
			ImGui::Begin("Alert", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
			ImGui::Text("Press R for toolbox");
			ImGui::End();
		}
		ImGui::Begin("Transparent", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);

		ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 1.f), "FPS: %.1f", io.Framerate);
		ImGui::SetWindowPos(ImVec2(0, width));
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(500.f, 100.f), ImGuiCond_FirstUseEver);

		const bool leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		const bool sceneClick = leftMouseDown && !leftMouseWasDown && !io.WantCaptureMouse;
		double clickMouseX = 0.0;
		double clickMouseY = 0.0;
		if (sceneClick)
		{
			glfwGetCursorPos(window, &clickMouseX, &clickMouseY);
			renderer.QueuePickRequestGpu(pickingShader, camera, camera_angle, width, height, clickMouseX, clickMouseY);
		}

		int pickedObjectId = -1;
		if (renderer.TryConsumePickResultGpu(pickedObjectId))
		{
			selectedObjectId = pickedObjectId;
			if (pickedObjectId != -1)
			{
				show = true;
			}
		}

		camera.m_focused = show;
		camera.width = width;
		camera.height = height;
		camera.Inputs(window);

		texture.Bind();
		renderer.Draw(shader, outlineShader, camera, camera_angle, selectedObjectId);
		leftMouseWasDown = leftMouseDown;

		if (show)
		{
			ImGui::SetNextWindowSize(ImVec2(460.f, 360.f), ImGuiCond_FirstUseEver);
			ImGui::Begin("Toolbox(Press R to Hide)", 0);
			if (ImGui::Button("+ Box"))
			{
				selectedObjectId = renderer.AddObject(Shape::Cube, NextSpawnLocation());
			}
			ImGui::SameLine();
			if (ImGui::Button("+ Pyramid"))
			{
				selectedObjectId = renderer.AddObject(Shape::Pyramid, NextSpawnLocation());
			}
			ImGui::SameLine();
			if (ImGui::Button("+ Sphere"))
			{
				selectedObjectId = renderer.AddObject(Shape::Sphere, NextSpawnLocation());
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear Selection"))
			{
				selectedObjectId = -1;
			}
			ImGui::Text("Objects: %d", static_cast<int>(renderer.GetObjects().size()));
			ImGui::Text("Click an object in the scene to select it");
			ImGui::Separator();

			for (const auto &object : renderer.GetObjects())
			{
				char label[64];
				std::snprintf(label, sizeof(label), "%s ID %d", ShapeName(object.shape), object.id);
				if (ImGui::Selectable(label, selectedObjectId == object.id))
				{
					selectedObjectId = object.id;
				}
			}

			Info *selectedObject = renderer.GetObjectById(selectedObjectId);
			if (selectedObject != nullptr)
			{
				ImGui::Separator();
				ImGui::Text("Selected ID: %d", selectedObject->id);
				ImGui::DragFloat3("Position", glm::value_ptr(selectedObject->location), 0.05f);
				ImGui::SliderFloat("Rotation", &selectedObject->rotationY, 0.f, 360.f);
				ImGui::SliderFloat("Scale", &selectedObject->scaleVal, 0.1f, 10.f);
			}

			ImGui::SliderFloat("FOV", &camera_angle, 0.f, 360.f);
			ImGui::SliderFloat("Auto Rotate Speed", &autoRotateSpeed, 0.0f, 360.0f);
			ImGui::Checkbox("WireFrame", &Wireframe);
			ImGui::Checkbox("Auto Rotate", &AutoRotate);

			ImGui::End();
		}
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}
