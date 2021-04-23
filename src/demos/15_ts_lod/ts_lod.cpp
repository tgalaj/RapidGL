#include "ts_lod.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/random.hpp>

TessellationLoD::TessellationLoD()
    : m_specular_power    (120.0f),
      m_specular_intenstiy(0.0f),
      m_dir_light_angles  (40.0f, 18.0f),
      m_line_color        (glm::vec4(107, 205, 96, 255) / 255.0f),
      m_line_width        (0.5f),
      m_ambient_color     (0.18f),
      m_tessellation_level(10)
{
}

TessellationLoD::~TessellationLoD()
{
}

void TessellationLoD::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glPatchParameteri(GL_PATCH_VERTICES, 3);

    GLint maxVerts;
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxVerts);
    printf("Max patch vertices: %d\n", maxVerts);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(0.0, 0.0, 10.5);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 0.8f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    /* Create object model */
    m_model = std::make_shared<RapidGL::Model>();
    m_model->load(RapidGL::FileSystem::getPath("models/lowpolytree.fbx"));
    //m_model->genSphere(2.0, 8);
    m_model->setDrawMode(GL_PATCHES);

    /* Create shader. */
    std::string dir  = "../src/demos/15_ts_lod/";
    m_pn_tessellation_shader = std::make_shared<RapidGL::Shader>(dir + "ts_lod.vert", dir + "ts_lod.frag", dir + "ts_lod.tcs", dir + "ts_lod.tes");
    m_pn_tessellation_shader->link();
}

void TessellationLoD::input()
{
    /* Close the application when Esc is released. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Escape))
    {
        stop();
    }

    /* Toggle between wireframe and solid rendering */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::F2))
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
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "15_ts_lod";
        if (take_screenshot_png(filename, RapidGL::Window::getWidth() / 2.0, RapidGL::Window::getHeight() / 2.0))
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

void TessellationLoD::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void TessellationLoD::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2));
    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Draw curve */
    m_pn_tessellation_shader->bind();
    m_pn_tessellation_shader->setUniform("model",                            model);
    m_pn_tessellation_shader->setUniform("normal_matrix",                    glm::mat3(glm::transpose(glm::inverse(model))));
    m_pn_tessellation_shader->setUniform("cam_pos",                          m_camera->position());
    m_pn_tessellation_shader->setUniform("view_projection",                  view_projection);
    m_pn_tessellation_shader->setUniform("tessellation_level",               static_cast<float>(m_tessellation_level));
    m_pn_tessellation_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    m_pn_tessellation_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_pn_tessellation_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    m_pn_tessellation_shader->setUniform("ambient",                          m_ambient_color);
    m_pn_tessellation_shader->setUniform("specular_intensity",               m_specular_intenstiy.x);
    m_pn_tessellation_shader->setUniform("specular_power",                   m_specular_power.x);

    m_model->render(m_pn_tessellation_shader, false);
}

void TessellationLoD::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos       = ImVec2(RapidGL::Window::getWidth() - 10.0, 10.0);
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
                        "RMB    - toggle cursor lock and rotate camera\n"
                        "Esc    - close the app\n\n");
        }

        ImGui::Spacing();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
        ImGui::SliderInt("Tessellation level", &m_tessellation_level, 1, 64);
        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
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
