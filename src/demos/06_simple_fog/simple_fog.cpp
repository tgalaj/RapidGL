#include "simple_fog.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <random>

SimpleFog::SimpleFog()
    : m_specular_power    (120.0f),
      m_specular_intenstiy(0.2f),
      m_ambient_factor    (0.18f),
      m_gamma             (2.2),
      m_dir_light_angles  (0.0f, 0.0f),
      m_fog_color         (0.5),
      m_fog_distances     (0.01, 30.0),
      m_fog_density_exp   (0.1),
      m_fog_density_exp2  (0.1),
      m_fog_equation      (FogEquation::LINEAR)
{
    m_fog_equation_names = { "fog_factor_linear", "fog_factor_exp", "fog_factor_exp2" };
}

SimpleFog::~SimpleFog()
{
}

void SimpleFog::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(m_fog_color.r, m_fog_color.g, m_fog_color.b, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 3.0);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 1.0f;
    m_dir_light_properties.setDirection(m_dir_light_angles);

    /* Create models. */
    m_objects.emplace_back(std::make_shared<RapidGL::Model>());
    m_objects[0]->genPQTorusKnot(256, 16, 2, 3, 0.75, 0.15);

    /* Set model matrices for each model. */
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(0.0, 1.0);

    float step = 2.0;
    for (unsigned i = 0; i < 20; ++i)
    {
        m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(step * i, 0.0, -step * i)));
        m_objects_colors.emplace_back(dist(gen), dist(gen), dist(gen));
    }

    /* Add textures to the objects. */
    RapidGL::Texture default_diffuse_texture;
    default_diffuse_texture.m_id = RapidGL::Util::loadGLTexture("default_diffuse.png", "textures", true);
    default_diffuse_texture.m_type = "texture_diffuse";

    for (auto& model : m_objects)
    {
        if (model->getMesh(0).getTexturesCount() == 0)
        {
            model->getMesh(0).addTexture(default_diffuse_texture);
        }
    }

    /* Create shader. */
    std::string dir          = "../src/demos/06_simple_fog/";
    std::string dir_lighting = "../src/demos/03_lighting/";

    m_directional_light_shader = std::make_shared<RapidGL::Shader>(dir_lighting + "lighting.vert", dir + "lighting-directional_w_fog.frag");
    m_directional_light_shader->link();
}

void SimpleFog::input()
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
        std::string filename = "06_simple_fog";
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

void SimpleFog::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void SimpleFog::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Render directional light(s) */
    m_directional_light_shader->bind();

    m_directional_light_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    
    m_directional_light_shader->setUniform("cam_pos",            m_camera->position());
    m_directional_light_shader->setUniform("specular_intensity", m_specular_intenstiy.x);
    m_directional_light_shader->setUniform("specular_power",     m_specular_power.x);
    m_directional_light_shader->setUniform("gamma",              m_gamma);
    m_directional_light_shader->setUniform("ambient_factor",     m_ambient_factor);

    m_directional_light_shader->setUniform("fog_color",        m_fog_color);

    m_directional_light_shader->setSubroutine(RapidGL::Shader::ShaderType::FRAGMENT, m_fog_equation_names[int(m_fog_equation)]);

    if (m_fog_equation == FogEquation::LINEAR)
    {
        m_directional_light_shader->setUniform("fog_min_distance", m_fog_distances.x);
        m_directional_light_shader->setUniform("fog_max_distance", m_fog_distances.y);
    }

    if (m_fog_equation == FogEquation::EXP)
    {
        m_directional_light_shader->setUniform("fog_density", m_fog_density_exp);
    }

    if (m_fog_equation == FogEquation::EXP2)
    {
        m_directional_light_shader->setUniform("fog_density", m_fog_density_exp2);
    }

    for (unsigned i = 0; i < m_objects_model_matrices.size(); ++i)
    {
        m_directional_light_shader->setUniform("model",         m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp",           view_projection * m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("object_color",  m_objects_colors[i]);

        m_objects[0]->render(m_directional_light_shader);
    }
}

void SimpleFog::render_gui()
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

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
        ImGui::SliderFloat("Ambient color", &m_ambient_factor, 0.0, 1.0,  "%.2f");
        ImGui::SliderFloat("Gamma",         &m_gamma,          0.0, 10.0, "%.1f");

        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Fog"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    if (ImGui::BeginCombo("Fog equation", m_fog_equation_names[int(m_fog_equation)].c_str()))
                    {
                        for (int i = 0; i < m_fog_equation_names.size(); ++i)
                        {
                            bool is_selected = (m_fog_equation == FogEquation(i));
                            if (ImGui::Selectable(m_fog_equation_names[i].c_str(), is_selected))
                            {
                                m_fog_equation = FogEquation(i);
                            }

                            if (is_selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    if (ImGui::ColorEdit3("Color", &m_fog_color[0]))
                    {
                        glClearColor(m_fog_color.r, m_fog_color.g, m_fog_color.b, 1.0);
                    }

                    if (m_fog_equation == FogEquation::LINEAR)
                    {
                        if (ImGui::SliderFloat2("Min/Max distance", &m_fog_distances[0], 0.0, 100.0, "%.1f"))
                        {
                            if (m_fog_distances.y <= m_fog_distances.x)
                            {
                                m_fog_distances.y = m_fog_distances.x + 0.1;
                            }
                        }
                    }

                    if (m_fog_equation == FogEquation::EXP)
                    {
                        ImGui::SliderFloat("Density", &m_fog_density_exp, 0.0, 1.0, "%.3f");
                    }

                    if (m_fog_equation == FogEquation::EXP2)
                    {
                        ImGui::SliderFloat("Density", &m_fog_density_exp2, 0.0, 1.0, "%.3f");
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3 ("Color",                 &m_dir_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",       &m_dir_light_properties.intensity, 0.0, 10.0,  "%.1f");
                    ImGui::SliderFloat("Specular power",        &m_specular_power.x,               1.0, 120.0, "%.0f");
                    ImGui::SliderFloat("Specular intensity",    &m_specular_intenstiy.x,           0.0, 1.0,   "%.2f");
                    
                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_dir_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_dir_light_properties.setDirection(m_dir_light_angles);
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
