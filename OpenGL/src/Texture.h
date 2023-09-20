#pragma once
#include "Renderer.h"

class Texture
{
private:
	unsigned int m_RendererID;
	std::string m_FilePath;
	unsigned char* m_LocalBuffer;
	int m_Width, m_Height, m_Bpp;

public:
	Texture(const std::string& path);
	~Texture();
	void Bind(unsigned int slot = 0) const;
	void UnBind() const;

	int GetWidth() { return m_Width; }
	int GetHeight() { return m_Height; }
};
