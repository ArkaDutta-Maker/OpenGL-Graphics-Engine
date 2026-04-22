cmake_minimum_required(VERSION 3.20)

project(OpenGLGraphicsEngine LANGUAGES C CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/OpenGL")
set(ENGINE_SRC "${ENGINE_ROOT}/src")

set(ENGINE_SOURCES
    "${ENGINE_SRC}/Application.cpp"
    "${ENGINE_SRC}/Camera.cpp"
    "${ENGINE_SRC}/IndexBuffer.cpp"
    "${ENGINE_SRC}/Renderer.cpp"
    "${ENGINE_SRC}/Shader.cpp"
    "${ENGINE_SRC}/Texture.cpp"
    "${ENGINE_SRC}/VertexArray.cpp"
    "${ENGINE_SRC}/VertexBuffer.cpp"
    "${ENGINE_SRC}/VertexBufferLayout.cpp"
    "${ENGINE_SRC}/tests/Test.cpp"
    "${ENGINE_SRC}/tests/TestClearColor.cpp"
    "${ENGINE_SRC}/vendor/glad/src/glad.c"
    "${ENGINE_SRC}/vendor/glm/detail/glm.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui_demo.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui_draw.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui_impl_glfw.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui_impl_opengl3.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui_tables.cpp"
    "${ENGINE_SRC}/vendor/imgui/imgui_widgets.cpp"
    "${ENGINE_SRC}/vendor/stb_image/stb_image.cpp"
)

add_executable(OpenGL ${ENGINE_SOURCES})

target_include_directories(OpenGL PRIVATE
    "${ENGINE_SRC}"
    "${ENGINE_SRC}/vendor"
    "${ENGINE_SRC}/vendor/glad/include"
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(OpenGL PRIVATE glfw OpenGL::GL)

if(WIN32)
    target_link_libraries(OpenGL PRIVATE gdi32 user32 shell32)
endif()

set_target_properties(OpenGL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    set_property(TARGET OpenGL PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

add_custom_command(TARGET OpenGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ENGINE_ROOT}/res"
            "$<TARGET_FILE_DIR:OpenGL>/res"
)
