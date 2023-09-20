#include "TestClearColor.h"
#include "imgui/imgui.h"
#include <Renderer.h>
namespace test
{
	TestClearColor::TestClearColor():m_ClearColor{0.2f,0.3f,0.0f,1.f}
	{

	}

	TestClearColor::~TestClearColor()
	{
	}

	void TestClearColor::OnImGUIRender()
	{
		Test::OnImGUIRender();
		ImGui::ColorPicker4("Clear Color", m_ClearColor);
	}

	void TestClearColor::OnRender()
	{
		Test::OnRender();
		glClearColor(m_ClearColor[0],m_ClearColor[1],m_ClearColor[2],m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void TestClearColor::OnUpdate(float deltaTime)
	{
		Test::OnUpdate(deltaTime);
	}
}
