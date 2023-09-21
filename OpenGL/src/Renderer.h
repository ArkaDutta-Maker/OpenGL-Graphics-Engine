#pragma once

#include <glad/glad.h>

#include "IndexBuffer.h"
#include "Shader.h"
#include "VertexArray.h"

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

class Renderer
{
private:
    std::vector<GLfloat> position;
	std::vector<unsigned int> indices;
public:
	void Clear();
	void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;
	void DrawCube(const Shader& shader);
	void DrawPyramid(const Shader& shader);
};
