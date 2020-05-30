#include "core_app.h"

#include <vector>

#include "stb_image_write.h"
#include "stb_image_resize.h"

#include "filesystem.h"
#include "input.h"
#include "timer.h"
#include "window.h"

#include "gui/gui.h"

namespace RapidGL
{
    CoreApp::CoreApp()
        : m_frame_time(0.0),
          m_fps(0),
          m_fpsToReturn(0),
          m_is_running(false)
    {
    }

    CoreApp::~CoreApp()
    {
    }

    void CoreApp::init(unsigned int width, unsigned int height, const std::string & title, double framerate)
    {
        m_frame_time = 1.0 / framerate;

        /* Init window */
        Window::createWindow(width, height, title);

        init_app();
    }

    void CoreApp::renderGUI()
    {
        /* Overlay start */
        const float DISTANCE = 10.0f;
        static int corner = 0;
        ImVec2 window_pos = ImVec2((corner & 1) ? ImGui::GetIO().DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? ImGui::GetIO().DisplaySize.y - DISTANCE : DISTANCE);
        ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
        if (corner != -1)
        {
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            ImGui::SetNextWindowSize({ 250, 0 });
        }

        ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
        if (ImGui::Begin("Perf info", 0, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
            ImGui::Text("Performance info\n");
            ImGui::Separator();
            ImGui::Text("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        }
        ImGui::End();
        /* Overlay end */
    }

    unsigned int CoreApp::getFPS() const
    {
        return m_fpsToReturn;
    }

    void CoreApp::start()
    {
        if (m_is_running)
        {
            return;
        }

        run();
    }

    void CoreApp::stop()
    {
        if (!m_is_running)
        {
            return;
        }

        m_is_running = false;
    }

    bool CoreApp::take_screenshot_png(const std::string & filename, size_t dst_width, size_t dst_height)
    {
        size_t width  = Window::getWidth();
        size_t height = Window::getHeight();
        bool   resize = true;

        if (dst_width == 0 || dst_height == 0)
        {
            resize = false;
        }

        std::vector<uint8_t> image;
        image.resize(width * height * 4);

        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

        if (resize)
        {
            auto resized_image = image;
            stbir_resize_uint8(image.data(), width, height, 0, resized_image.data(), dst_width, dst_height, 0, 4);

            width  = dst_width;
            height = dst_height;
            image  = resized_image;
        }

        auto screenshots_dir = FileSystem::getPath("../screenshots");
        if (!FileSystem::directoryExists(screenshots_dir))
        {
            FileSystem::createDirectory(screenshots_dir);
        }

        auto filepath = screenshots_dir + "/" + filename + ".png";

        stbi_flip_vertically_on_write(true);
        auto ret = stbi_write_png(filepath.c_str(), width, height, 4, image.data(), 0);

        return ret;
    }

    void CoreApp::run()
    {
        m_is_running = true;

        int frames = 0;
        double frame_counter = 0.0;

        double last_time = Timer::getTime();
        double unprocessed_time = 0.0;

        double start_time = 0.0;
        double passed_time = 0.0;

        bool should_render = false;

        while (m_is_running)
        {
            should_render = false;

            start_time = Timer::getTime();
            passed_time = start_time - last_time;

            last_time = start_time;

            unprocessed_time += passed_time;
            frame_counter += passed_time;

            while (unprocessed_time > m_frame_time)
            {
                should_render = true;

                unprocessed_time -= m_frame_time;

                if (Window::isCloseRequested())
                {
                    m_is_running = false;
                }

                /* Update input, game entities, etc. */
                input();
                update(m_frame_time);
                Input::update();

                if (frame_counter >= 1.0)
                {
                    #ifdef _DEBUG
                    //std::cout << 1000.0 / (double)frames << std::endl;
                    #endif

                    frames = 0;
                    frame_counter = 0;
                }
            }

            if (should_render)
            {
                /* Render */
                render();

                GUI::prepare();
                {
                    renderGUI();
                }
                GUI::render();

                Window::endFrame();
                frames++;
            }
        }
    }
}
