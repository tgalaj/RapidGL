#pragma once

#include <string>

#define GLFW_INCLUDE_NONE
#define MIN_GL_VERSION_MAJOR 4
#define MIN_GL_VERSION_MINOR 6

namespace RGL
{
    class CoreApp
    {
    public:
        CoreApp();
        virtual ~CoreApp();

        CoreApp(const CoreApp&)            = delete;
        CoreApp& operator=(const CoreApp&) = delete;

        virtual void init(unsigned int width, unsigned int height, const std::string & title, double framerate = 60.0) final;

        virtual void init_app()                = 0;
        virtual void input()                   = 0;
        virtual void update(double delta_time) = 0;
        virtual void render()                  = 0;
        virtual void render_gui();

        unsigned int get_fps() const;

        virtual void start() final;
        virtual void stop()  final;

        virtual bool take_screenshot_png(const std::string & filename, size_t dst_width = 0, size_t dst_height = 0);

    private:
        void run();

        double       m_frame_time;
        unsigned int m_fps;
        unsigned int m_fpsToReturn;
        bool         m_is_running;
    };
}
