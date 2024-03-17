#pragma once

#include <glad/glad.h>
#include <iostream>
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
                                     const GLchar* message,
                                     const void* userParam);
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
struct Info
{
	glm::mat4 mvp;
	Shape shape;
	float& camera_angle;
	float& Rotation;
	float& scaleVal;
	Camera& camera;
	glm::vec3 location;
};
class Renderer
{
private:
	std::vector<GLfloat> position;
	std::vector<unsigned int> indices;
	std::vector<Info> m_Objects;
public:
	// write an enum containing the different shapes
	
	void Clear();
	void Draw(Shader& shader);
	void DrawCube(const Shader& shader);
	void DrawPyramid(const Shader& shader);
	void DrawSphere();
	void AddObject(Shader& shader, Texture& texture, Camera& camera, float& camera_angle, float& Rotation, float& scaleVal, glm::vec3 location, Shape shape);
};
