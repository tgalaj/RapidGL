#pragma once
#include "common.h"
#include <string>

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

        uint32_t get_fps() const;

        virtual void start() final;
        virtual void stop()  final;

        virtual bool take_screenshot_png(const std::string & filename, size_t dst_width = 0, size_t dst_height = 0);

    private:
        void run();

        double   m_frame_time;
        uint32_t m_fps;
        bool     m_is_running;
    };
}
