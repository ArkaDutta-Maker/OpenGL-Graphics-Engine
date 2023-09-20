#pragma once
class IndexBuffer
{
private:
	unsigned int m_RendererID;
	unsigned int m_Count;

public:
	IndexBuffer(const void* data, unsigned int count);
	~IndexBuffer();

	void Bind() const;
	void UnBind() const;

	unsigned int GetCount() const { return m_Count; }
};
