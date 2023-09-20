#pragma once
#include <stdexcept>
#include <vector>
#include <glad/glad.h>

struct VertexBufferElement
{
	unsigned int type;
	unsigned int count;
	bool normalised;

	static unsigned int GetSizeOfType(unsigned int type)
	{
		switch (type)
		{
		case GL_FLOAT: return 4;
		case GL_UNSIGNED_INT: return 4;
		case GL_UNSIGNED_BYTE: return 1;
			default: return NULL;
		}
	}
};

class VertexBufferLayout
{
private:
	std::vector<VertexBufferElement> m_Elements;
	unsigned int m_Stride;

public:
	VertexBufferLayout(): m_Stride(0)
	{
	}

	template <typename T>
	void Push(unsigned int count)
	{
		throw std::runtime_error(nullptr);
	}

	template <>
	void Push<float>(unsigned int count)
	{
		m_Elements.push_back({GL_FLOAT, count, false});
		m_Stride += count * VertexBufferElement::GetSizeOfType(GL_FLOAT);
	}

	template <>
	void Push<unsigned int>(unsigned int count)
	{
		m_Elements.push_back({GL_UNSIGNED_INT, count, false});
		m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_INT);
	}

	const std::vector<VertexBufferElement> GetElements() const
	{
		return m_Elements;
	}

	unsigned int GetStride() const { return m_Stride; }
};
