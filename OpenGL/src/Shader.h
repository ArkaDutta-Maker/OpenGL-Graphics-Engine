#pragma once
#include <glm/glm.hpp>
#include <string>
#include <tuple>
#include <unordered_map>

std::string get_file_contents(const char* filename);

class Shader
{
private:
	std::string m_Fragfilepath;
	std::string m_Vertfilepath;
	unsigned int m_RendererID;
	std::unordered_map<std::string, unsigned int> m_UniformLocationCache;

public:
	Shader(const char* fragmentFilepath, const char* vertexFilepath);
	~Shader();

	void Bind() const;
	void UnBind() const;


	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniform1i(const std::string& name, int value);
	void SetUniform1f(const std::string& name, float value);
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
	static unsigned int CompileShader(unsigned int type, const std::string& source);
	unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
	
	unsigned int GetUniformLocation(const std::string& name);
};
