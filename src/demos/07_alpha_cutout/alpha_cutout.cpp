#include "alpha_cutout.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <random>

#include "poisson_disk_sampling.h"

AlphaCutout::AlphaCutout()
    : m_specular_power         (120.0f),
      m_specular_intenstiy     (0.0f),
      m_ambient_factor         (0.18f),
      m_gamma                  (2.2),
      m_dir_light_angles       (0.0f, 50.0f),
      m_alpha_cutout_threshold (0.15)
{
}

AlphaCutout::~AlphaCutout()
{
}

void AlphaCutout::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.76, 0.913, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 3.0);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 3.5f;
    m_dir_light_properties.setDirection(m_dir_light_angles);

    /* Create models. */
    m_pine_tree.Load(RGL::FileSystem::getResourcesPath() / "models/pine/snow_pine_tree.obj");

    constexpr auto kRadius    = 2.5f;
    constexpr float area_size = 15.0f;

    m_ground_plane.GenPlane(area_size * 2.0 + kRadius, area_size * 2.0 + kRadius, area_size * 2.0, area_size * 2.0);

    m_ground_plane_model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, -0.5, 0.0));

    /* Set model matrices for each model. */
    constexpr auto kXMin = std::array<float, 2>{ {-area_size, -area_size}};
    constexpr auto kXMax = std::array<float, 2>{ { area_size,  area_size}};

    const auto samples = thinks::PoissonDiskSampling(kRadius, kXMin, kXMax);

    for (unsigned i = 0; i < samples.size(); ++i)
    {
        m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(samples[i][0], -0.5, samples[i][1])) * glm::scale(glm::mat4(1.0), glm::vec3(0.02)));
    }
   

    /* Add textures to the objects. */
    auto pine_texture = std::make_shared<RGL::Texture2D>();
    pine_texture->Load(RGL::FileSystem::getResourcesPath() / "models/pine/diffuse_half.tga", true);
    pine_texture->SetAnisotropy(16);

    auto ground_texture = std::make_shared<RGL::Texture2D>();
    ground_texture->Load(RGL::FileSystem::getResourcesPath() / "textures/grass_green_d.jpg", true);
    ground_texture->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT);
    ground_texture->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
    ground_texture->SetAnisotropy(16);

    m_pine_tree.AddTexture(pine_texture);
    m_ground_plane.AddTexture(ground_texture);

    /* Create shader. */
    std::string dir          = "src/demos/07_alpha_cutout/";
    std::string dir_lighting = "src/demos/03_lighting/";

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir_lighting + "lighting.vert", dir + "lighting-directional_alpha_cutout.frag");
    m_directional_light_shader->link();
}

void AlphaCutout::input()
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
        std::string filename = "07_alpha_cutout";
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

void AlphaCutout::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void AlphaCutout::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Render directional light(s) */
    m_directional_light_shader->bind();

    m_directional_light_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    
    m_directional_light_shader->setUniform("cam_pos",                m_camera->position());
    m_directional_light_shader->setUniform("specular_intensity",     m_specular_intenstiy.x);
    m_directional_light_shader->setUniform("specular_power",         m_specular_power.x);
    m_directional_light_shader->setUniform("gamma",                  m_gamma);
    m_directional_light_shader->setUniform("ambient_factor",         m_ambient_factor);
    m_directional_light_shader->setUniform("alpha_cutout_threshold", m_alpha_cutout_threshold);

    for (unsigned i = 0; i < m_objects_model_matrices.size(); ++i)
    {
        m_directional_light_shader->setUniform("model",         m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp",           view_projection * m_objects_model_matrices[i]);

        m_pine_tree.Render();
    }

    /* Render ground plane */
    m_directional_light_shader->setUniform("model", m_ground_plane_model);
    m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_ground_plane_model))));
    m_directional_light_shader->setUniform("mvp", view_projection * m_ground_plane_model);
    
    m_ground_plane.Render();
}

void AlphaCutout::render_gui()
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
        ImGui::SliderFloat("Ambient color", &m_ambient_factor, 0.0, 1.0,  "%.2f");
        ImGui::SliderFloat("Gamma",         &m_gamma,          0.0, 10.0, "%.1f");

        ImGui::SliderFloat("Alpha cutout threshold", &m_alpha_cutout_threshold, 0.0, 1.0, "%.2f");

        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
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
