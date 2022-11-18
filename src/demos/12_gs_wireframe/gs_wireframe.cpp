#include "gs_wireframe.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

GSWireframe::GSWireframe()
    : m_specular_power    (120.0f),
      m_specular_intenstiy(0.0f),
      m_dir_light_angles  (40.0f, 18.0f),
      m_line_color        (glm::vec4(107, 205, 96, 255) / 255.0f),
      m_line_width        (0.5f),
      m_ambient_color     (0.18f)
{
}

GSWireframe::~GSWireframe()
{
}

void GSWireframe::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-1, 1.0, 2.0);
    m_camera->setOrientation(0.0f, 35.0f, 0.0f);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 0.8f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    /* Create models. */
    m_objects.emplace_back(std::make_shared<RGL::StaticModel>());

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0]->Load(RGL::FileSystem::getPath("models/armadillo.obj"));

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 0.0)) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.02)));

    /* Create shader. */
    std::string dir = "../src/demos/12_gs_wireframe/";
    m_directional_light_shader = std::make_shared<RGL::Shader>(dir + "gs_wireframe.vert", dir + "gs_wireframe.frag", dir + "gs_wireframe.geom");
    m_directional_light_shader->link();
}

void GSWireframe::input()
{
    /* Close the application when Esc is released. */
    if (RGL::Input::getKeyUp(RGL::KeyCode::Escape))
    {
        stop();
    }

    /* Toggle between wireframe and solid rendering */
    if (RGL::Input::getKeyUp(RGL::KeyCode::F2))
    {
        static bool toggle_wireframe = false;

        toggle_wireframe = !toggle_wireframe;

        if (toggle_wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    /* It's also possible to take a screenshot. */
    if (RGL::Input::getKeyUp(RGL::KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "12_gs_wireframe";
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

void GSWireframe::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void GSWireframe::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_directional_light_shader->bind();
    m_directional_light_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    m_directional_light_shader->setUniform("cam_pos",                          m_camera->position());
    m_directional_light_shader->setUniform("ambient",                          m_ambient_color);
    m_directional_light_shader->setUniform("specular_intensity",               m_specular_intenstiy.x);
    m_directional_light_shader->setUniform("specular_power",                   m_specular_power.x);
    m_directional_light_shader->setUniform("viewport_matrix",                  RGL::Window::getViewportMatrix());
    m_directional_light_shader->setUniform("line_info.width",                  m_line_width * 0.5f);
    m_directional_light_shader->setUniform("line_info.color",                  m_line_color);

    const auto view_projection = m_camera->m_projection * m_camera->m_view;
    
    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_directional_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->Render();
    }
}

void GSWireframe::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos = ImVec2(RGL::Window::getWidth() - 10.0, 10.0);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 400, 0 });

    ImGui::Begin("Info");
    {
        if (ImGui::CollapsingHeader("Help"))
        {
            ImGui::Text("Controls info: \n\n"
                "F1     - take a screenshot\n"
                "F2     - toggle wireframe rendering\n"
                "WASDQE - control camera movement\n"
                "RMB    - press to rotate the camera\n"
                "Esc    - close the app\n\n");
        }

        ImGui::Spacing();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        static float ambient_factor = m_ambient_color.r;
        if(ImGui::SliderFloat("Ambient color", &ambient_factor, 0.0, 1.0, "%.2f"))
        {
            m_ambient_color = glm::vec3(ambient_factor);
        }

        ImGui::Spacing();

        ImGui::SliderFloat("Line width", &m_line_width, 0.0, 10.0, "%.1f");
        ImGui::ColorEdit4("Line color", &m_line_color[0]);

        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                {
                    ImGui::ColorEdit3("Color", &m_dir_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity", &m_dir_light_properties.intensity, 0.0, 10.0, "%.1f");
                    ImGui::SliderFloat("Specular power", &m_specular_power.x, 1.0, 120.0, "%.0f");
                    ImGui::SliderFloat("Specular intensity", &m_specular_intenstiy.x, 0.0, 1.0, "%.2f");

                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_dir_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
