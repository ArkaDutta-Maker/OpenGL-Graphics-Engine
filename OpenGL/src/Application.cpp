#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"

int width = 800;
int height = 800;
bool show = false;

//TODO:-
//1)  IMPLEMENT DrawSphere();(TBD)
//2) IMPLEMENT AddCubes();(TBD)
//3) Implement Ligting;(Doing)


void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
	
}
void framebuffer_size_callback(GLFWwindow* window, int m_width, int m_height)
{
	width = m_width;
	height = m_height;
	glViewport(0,0,width,height);
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_RELEASE) return; //only handle press events
    if(key == GLFW_KEY_R) show = !show;
}
void Rotate(float& rotation, double& prevTime)
{
	
		double crntTime = glfwGetTime();
		if(rotation > 360.f)
		{
			rotation = 0.f;
		}
		if (crntTime - prevTime >= 1 / 60 && rotation <= 360.f)
		{
			rotation += 1.f;
			prevTime = crntTime;
		}
}
int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	const char* glsl_version = "#version 460";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

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


	GLfloat position[] =
	{ // COORDINATES                  
		// -0.5f, 0.0f,  0.5f,     0.f, 1.f, 0.f,	//0.0f, 0.0f,
		// -0.5f, 0.0f, -0.5f,     0.f, 0.f, 1.f,	//5.0f, 0.0f,
		//  0.5f, 0.0f, -0.5f,     1.f, 0.f, 0.f,	//0.0f, 0.0f,
		//  0.5f, 0.0f,  0.5f,     0.f, 1.f, 0.f,	//5.0f, 0.0f,
		//  0.0f, 0.8f,  0.0f,     0.f, 0.f, 1.f,	//2.5f, 5.0f

		//CUBE
		 0.5f, 0.5f,  0.5f,		1.f, 0.f, 0.f,	0.0f, 0.0f,
		-0.5f, 0.5f,  0.5f,		0.f, 1.f, 0.f,	5.0f, 0.0f, 
		-0.5f,-0.5f,  0.5f,		0.f, 0.f, 1.f,	0.0f, 0.0f,
		0.5f,  -0.5f,  0.5f,	1.f, 1.f, 1.f,	5.0f, 0.0f,
		// back
		 0.5,  0.5, -0.5,		1.f, 0.f, 0.f,	0.0f, 0.0f,
		-0.5,  0.5, -0.5,		0.f, 1.f, 0.f,	5.0f, 0.0f,
		 -0.5, -0.5, -0.5,		0.f, 0.f, 1.f,	0.0f, 0.0f,
		 0.5, -0.5, -0.5,		1.f, 1.f, 1.f,	5.0f, 0.0f
	};
	unsigned int indices[] = {
		//PYRAMID
		// 0, 1, 2,
		// 0, 2, 3,
		// 0, 1, 4,
		// 1, 2, 4,
		// 2, 3, 4,
		// 3, 0, 4
		//CUBE
		0, 1, 2,
		2, 3, 0,

      // Right
      0, 3, 7,
      7, 4, 0,

      // Bottom
      2, 6, 7,
      7, 3, 2,

      // Left
      1, 5, 6,
      6, 2, 1,

      // Back
      4, 7, 6,
      6, 5, 4,

      // Top
      5, 1, 0,
      0, 4, 5,
	};

	glm::mat4 model(1.0f);
	glm::mat4 view (1.0f);
	glm::mat4 proj (1.0f);
	glm::mat4 mvp = model*view*proj;
	Shader shader("res/Shader/Frag.frag","res/Shader/Vertex.vert");
	shader.Bind();

	shader.SetUniformMat4f("u_mvp", mvp);
	
	shader.SetUniform1f("scale",1.f);

	Texture texture("res/Texture/brick.png");
	texture.Bind();
	shader.SetUniform1i("tex0", 0);
	// float r = 0.f;
	// float increment = 0.05f;
	shader.UnBind();
	Renderer renderer;

		// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
	ImGui::StyleColorsDark();
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);

	ImGui_ImplOpenGL3_Init(glsl_version);


	glCheckError();

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	float camera_angle = 90.f;
	glfwSwapInterval(1);
	// Enables the Depth Buffer
	glEnable(GL_DEPTH_TEST);

	float rotation1 = 0.0f;
	float rotation2= 0.0f;

	//double prevTime = glfwGetTime();
	bool Wireframe =false;

	Camera camera(width,height, glm::vec3(0.0f, 0.0f, 2.0f));
	float scaleCubeA = 1.f;
	float scaleCubeB = 1.f;
	bool AutoRotate = false;
	double prevTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		
		processInput(window);

		renderer.Clear();
		Wireframe ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE): glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		
		ImGui::Begin("Alert", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::Text("Press R for toolbox");
		

		ImGui::End();

		ImGui::Begin("Transparent",0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
		
        ImGui::TextColored(ImVec4(1.f,1.f,1.f,1.f),"FPS: %.1f",  io.Framerate);
		ImGui::SetWindowPos(ImVec2(0,width));
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(500.f,100.f));

		camera.Inputs(window);
		camera.m_focused = show;
		camera.width = width;
		camera.height = height;

		{
			shader.Bind();
			if(AutoRotate) Rotate(rotation2, prevTime);

			// Initializes matrices so they are not the null matrix
			glm::mat4 model = glm::mat4(1.0f);
			
			glm::mat4 cam_mat4 = camera.Matrix(camera_angle, 0.1f, 100.f);
			model = glm::rotate(model, glm::radians(rotation2), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 mvp = cam_mat4 * model;
			mvp = glm::translate(mvp, glm::vec3(0.f,2.f,0.f));
			mvp = glm::scale(mvp, glm::vec3(scaleCubeB));


			shader.SetUniformMat4f("u_mvp", mvp);
			
			//shader.SetUniform1f("scale",1.f);
			texture.Bind();
			renderer.DrawPyramid(shader);
		}
		{
			shader.Bind();
			if(AutoRotate) Rotate(rotation1, prevTime);
			// Initializes matrices so they are not the null matrix
			glm::mat4 model = glm::mat4(1.0f);
			
			glm::mat4 cam_mat4 = camera.Matrix(camera_angle, 0.1f, 100.f);
			model = glm::rotate(model, glm::radians(rotation1), glm::vec3(0.0f, 1.0f, 0.0f));
			

			glm::mat4 mvp = cam_mat4 * model;

			mvp = glm::scale(mvp, glm::vec3(scaleCubeA));
			shader.SetUniformMat4f("u_mvp", mvp);
			
			//shader.SetUniform1f("scale",1.f);
			texture.Bind();
			renderer.DrawCube(shader);
			shader.UnBind();
			texture.UnBind();
		}
		renderer.DrawSphere(); //Implementation Not done yet

		if(show)
		{
			
			ImGui::Begin("Toolbox(Press R to Hide)",0);
			ImGui::SliderFloat("Rotate Item 1", &rotation1,0.f,360.f);
			ImGui::SliderFloat("Rotate Item 2", &rotation2,0.f,360.f);
			ImGui::SliderFloat("Scale Item 1", &scaleCubeA,0.5f,100.f);
			ImGui::SliderFloat("Scale Item 2", &scaleCubeB,0.5f,100.f);
			ImGui::SliderFloat("FOV", &camera_angle,0.f,360.f);
			ImGui::Checkbox("WireFrame",&Wireframe );
			ImGui::Checkbox("Auto Rotate",&AutoRotate );
			
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
