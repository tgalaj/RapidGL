#include "simple_3d.h"
#include "filesystem.h"
#include "input.h"
#include "gui/gui.h"

Simple3d::Simple3d()
{
}

Simple3d::~Simple3d()
{
}

void Simple3d::init_app()
{
    /* TODO:
     * 
     * - Show model loding
     * - show camera navigation
     * - show model generation using built-in functions
     * - show texturing
     * 
    */

    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
}

void Simple3d::input()
{
    /* Close the application when Esc is released. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Escape))
    {
        stop();
    }

    /* It's also possible to take a screenshot. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Alpha1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "02_simple_3d";
        if (take_screenshot_png(filename, 400, 300))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << RapidGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RapidGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
    }
}

void Simple3d::update(double delta_time)
{
    /* Update variables here. */
}

void Simple3d::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT);
}

void Simple3d::renderGUI()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call renderGUI() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::renderGUI();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos = ImVec2(10.0, 220.0);
    ImVec2 window_pos_pivot = ImVec2(0.0f, 1.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 250, 0 });

    ImGui::Begin("Info");
    {
        ImGui::Text("Controls info: \n\n"
                    "Alpha 1 - take a screenshot\n"
                    "Esc     - close the app\n\n");
    }
    ImGui::End();
}
