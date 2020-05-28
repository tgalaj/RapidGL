#pragma once

#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

namespace Vertex
{
    class Window final
    {
    public:
        Window();
        ~Window();

        static void createWindow(unsigned int width, unsigned int height, const std::string & title);
        static void endFrame();

        static int isCloseRequested();

        static int       getWidth();
        static int       getHeight();
        static glm::vec2 getCenter();
        static float     getAspectRatio();

        static const std::string & getTitle();

        static void setVSync(bool enabled);
        static void bindDefaultFramebuffer();

    private:
        static GLFWwindow * m_window;
        static std::string  m_title;
        static glm::vec2    m_window_size;

        static void error_callback(int error, const char* description)
        {
            std::cerr << description << std::endl;
        }

        static void framebuffer_size_callback(GLFWwindow * window, int width, int height);
    };
}
