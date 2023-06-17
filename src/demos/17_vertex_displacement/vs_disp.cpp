#include "vs_disp.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/quaternion.hpp>

VertexDisplacement::VertexDisplacement()
    : m_specular_power    (120.0f),
      m_specular_intenstiy(0.0f),
      m_dir_light_angles  (67.5f),
      m_ambient_color     (0.18f),
      m_time              (0.0f),
      m_amplitude         (0.8f),
      m_velocity          (3.4f),
      m_frequency         (1.6f)

{
}

VertexDisplacement::~VertexDisplacement()
{
}

void VertexDisplacement::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-3.0, 1.5, 10.0);
    m_camera->setOrientation(5.0f, 20.0f, 0.0f);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 0.8f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    /* Create object model */
    m_model = std::make_shared<RGL::StaticModel>();
    m_model->GenPlane(10, 5, 100, 50);

    glm::quat rotation(glm::radians(glm::vec3(70.0, 0.0, 0.0)));
    m_world_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.0, 0)) * glm::mat4_cast(rotation);

    /* Create shader. */
    std::string dir  = "src/demos/17_vertex_displacement/";
    m_vs_disp_shader = std::make_shared<RGL::Shader>(dir + "vs_disp.vert", dir + "vs_disp.frag");
    m_vs_disp_shader->link();
}

void VertexDisplacement::input()
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
        std::string filename = "17_vertex_displacement";
        if (take_screenshot_png(filename, RGL::Window::getWidth() / 2.0, RGL::Window::getHeight() / 2.0))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << RGL::FileSystem::getRootPath() / "screenshots/" << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RGL::FileSystem::getRootPath() / "screenshots/" << std::endl;
        }
    }
}

void VertexDisplacement::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
    m_time += delta_time;
}

void VertexDisplacement::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Draw curve */
    m_vs_disp_shader->bind();
    m_vs_disp_shader->setUniform("cam_pos",                          m_camera->position());
    m_vs_disp_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    m_vs_disp_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_vs_disp_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    m_vs_disp_shader->setUniform("ambient",                          m_ambient_color);
    m_vs_disp_shader->setUniform("specular_intensity",               m_specular_intenstiy.x);
    m_vs_disp_shader->setUniform("specular_power",                   m_specular_power.x);
    m_vs_disp_shader->setUniform("mvp",                              view_projection * m_world_matrix);
    m_vs_disp_shader->setUniform("model",                            m_world_matrix);
    m_vs_disp_shader->setUniform("normal_matrix",                    glm::mat3(glm::transpose(glm::inverse(m_world_matrix))));
    m_vs_disp_shader->setUniform("time",                             m_time);
    m_vs_disp_shader->setUniform("amplitude",                        m_amplitude);
    m_vs_disp_shader->setUniform("velocity",                         m_velocity);
    m_vs_disp_shader->setUniform("frequency",                        m_frequency);
    m_model->Render();
}

void VertexDisplacement::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos       = ImVec2(RGL::Window::getWidth() - 10.0, 10.0);
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
        ImGui::SliderFloat("Amplitude", &m_amplitude, 0.0, 4.0,  "%.1f");
        ImGui::SliderFloat("Velocity",  &m_velocity,  0.0, 20.0, "%.1f");
        ImGui::SliderFloat("Frequency", &m_frequency, 0.0, 20.0, "%.1f");
        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                {
                    ImGui::ColorEdit3 ("Light color",        &m_dir_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",    &m_dir_light_properties.intensity, 0.0, 10.0,  "%.1f");
                    ImGui::SliderFloat("Specular power",     &m_specular_power.x,               1.0, 120.0, "%.0f");
                    ImGui::SliderFloat("Specular intensity", &m_specular_intenstiy.x,           0.0, 1.0,   "%.2f");

                    static float ambient_factor = m_ambient_color.r;
                    if (ImGui::SliderFloat("Ambient color", &ambient_factor, 0.0, 1.0, "%.2f"))
                    {
                        m_ambient_color = glm::vec3(ambient_factor);
                    }

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
