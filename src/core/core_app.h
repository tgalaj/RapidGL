#pragma once

#include <string>

#define GLFW_INCLUDE_NONE
#define MIN_GL_VERSION_MAJOR 4
#define MIN_GL_VERSION_MINOR 6

namespace RapidGL
{
    class CoreApp
    {
    public:
        CoreApp();
        ~CoreApp();

        CoreApp(const CoreApp&)            = delete;
        CoreApp& operator=(const CoreApp&) = delete;

        virtual void init(unsigned int width, unsigned int height, const std::string & title, double framerate = 60.0) final;

        virtual void init_app()                = 0;
        virtual void input()                   = 0;
        virtual void update(double delta_time) = 0;
        virtual void render()                  = 0;
        virtual void renderGUI();

        unsigned int getFPS() const;

        virtual void start() final;
        virtual void stop()  final;

    private:
        void run();

        double       m_frame_time;
        unsigned int m_fps;
        unsigned int m_fpsToReturn;
        bool         m_is_running;
    };
}
