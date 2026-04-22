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
  int id;
	Shape shape;
    float rotationY;
	float scaleVal;
	glm::vec3 location;
};
class Renderer
{
private:
	std::vector<GLfloat> position;
	std::vector<unsigned int> indices;
	std::vector<Info> m_Objects;
 int m_NextObjectId = 1;
public:
	// write an enum containing the different shapes
	
	void Clear();
  void Draw(Shader& shader, Camera& camera, float camera_angle);
	void DrawCube(const Shader& shader);
	void DrawPyramid(const Shader& shader);
	void DrawSphere();
   int AddObject(Shape shape, const glm::vec3& location, float rotationY = 0.0f, float scaleVal = 1.0f);
	std::vector<Info>& GetObjects();
	Info* GetObjectById(int id);
	void AutoRotateObjects(float deltaDegrees);
	int PickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const;
};
