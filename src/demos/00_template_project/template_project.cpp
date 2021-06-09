#include "template_project.h"
#include "filesystem.h"
#include "input.h"
#include "gui/gui.h"

TemplateProject::TemplateProject()
{
}

TemplateProject::~TemplateProject()
{
}

void TemplateProject::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);

    glEnable(GL_MULTISAMPLE);
}

void TemplateProject::input()
{
    /* Close the application when Esc is released. */
    if (RGL::Input::getKeyUp(RGL::KeyCode::Escape))
    {
        stop();
    }

    /* It's also possible to take a screenshot. */
    if (RGL::Input::getKeyUp(RGL::KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "00_template_project";
        if (take_screenshot_png(filename, RGL::Window::getWidth() / 2.0, RGL::Window::getHeight() / 2.0))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << RGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
    }
}

void TemplateProject::update(double delta_time)
{
    /* Update variables here. */
}

void TemplateProject::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT);
}

void TemplateProject::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos = ImVec2(10.0, 220.0);
    ImVec2 window_pos_pivot = ImVec2(0.0f, 1.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 250, 0 });

    ImGui::Begin("Info");
    {
        ImGui::Text("Controls info: \n\n"
                    "F1  - take a screenshot\n"
                    "Esc - close the app\n\n");

        ImGui::TextWrapped("See src/demos/00_template_project/template_project.h for documentation.");
    }
    ImGui::End();
}
