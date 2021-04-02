#include "enviro_mapping.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>

EnvironmentMapping::EnvironmentMapping()
    : m_specular_power         (120.0f),
      m_specular_intenstiy     (0.0f),
      m_ambient_factor         (0.18f),
      m_gamma                  (2.2),
      m_dir_light_angles       (45.0f, 50.0f),
      m_alpha_cutout_threshold (0.15)
{
}

EnvironmentMapping::~EnvironmentMapping()
{
}

void EnvironmentMapping::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.76, 0.913, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(0.0, 5.0, 9.0);
    m_camera->setOrientation(glm::vec3(0.0, 3.0, -9.0));

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 3.5f;
    m_dir_light_properties.setDirection(m_dir_light_angles);

    /* Create models. */
    m_objects.emplace_back(std::make_shared<RapidGL::Model>());
    m_objects[0]->load(RapidGL::FileSystem::getPath("models/xyzrgb_dragon.obj"));
    m_xyzrgb_dragon = m_objects[0];

    m_objects.emplace_back(std::make_shared<RapidGL::Model>());
    m_objects[1]->load(RapidGL::FileSystem::getPath("models/lucy.obj"));
    m_lucy = m_objects[1];

    constexpr auto kRadius    = 2.5f; 
    constexpr float area_size = 15.0f;

    m_objects.emplace_back(std::make_shared<RapidGL::Model>());
    m_objects[2]->genPlane(area_size * 2.0 + kRadius, area_size * 2.0 + kRadius, area_size * 2.0, area_size * 2.0);
    m_ground_plane = m_objects[2];

    /* Set model matrices for each model. */
    /* xyzrgb dragon */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-4.0f, 0.31f, -1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.02)));

    /* lucy */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(4.0f, 0.71f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(135.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.002)));

    /* plane */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(0.0, -0.5, 0.0)));

    /* Set the color tints for xyzrgb dragon, lucy and the plane respectively. */
    m_color_tints.emplace_back(glm::vec3(1.0));
    m_color_tints.emplace_back(glm::vec3(1.0));
    m_color_tints.emplace_back(glm::vec3(1.0));

    /* Add textures to the objects. */
    RapidGL::Texture default_diffuse_texture;
    default_diffuse_texture.m_id = RapidGL::Util::loadGLTexture2D("default_diffuse.png", "textures", true);
    default_diffuse_texture.m_type = "texture_diffuse";

    if (m_xyzrgb_dragon->getMesh(0).getTexturesCount() == 0)
    {
        m_xyzrgb_dragon->getMesh(0).addTexture(default_diffuse_texture);
    }

    if (m_lucy->getMesh(0).getTexturesCount() == 0)
    {
        m_lucy->getMesh(0).addTexture(default_diffuse_texture);
    }

    RapidGL::Texture ground_texture;
    ground_texture.m_id = RapidGL::Util::loadGLTexture2D("grass_green_d.jpg", "textures", true);
    ground_texture.m_type = "texture_diffuse";

    m_ground_plane->getMesh(0).addTexture(ground_texture);


    constexpr uint8_t no_spheres = 1000;
    constexpr float max_sphere_radius = 0.4f;

    for (int i = 0; i < no_spheres; ++i)
    {
        float rand_radius = RapidGL::Util::randomDouble(0.1, max_sphere_radius);
        m_objects.emplace_back(std::make_shared<RapidGL::Model>());
        m_objects[3 + i]->genSphere(rand_radius, 20);
        m_objects[3 + i]->getMesh(0).addTexture(default_diffuse_texture);

        glm::vec3 random_position(0.0f);
        while(random_position.y - rand_radius < 0.0f)
            random_position = glm::sphericalRand(12.0f);

        glm::vec3 random_color = glm::linearRand(glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0));

        m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), random_position));
        m_color_tints.emplace_back(random_color);
    }

    /* Create shader. */
    std::string dir          = "../src/demos/08_enviro_mapping/";
    std::string dir_lighting = "../src/demos/03_lighting/";

    m_directional_light_shader = std::make_shared<RapidGL::Shader>(dir_lighting + "lighting.vert", dir + "lighting-directional.frag");
    m_directional_light_shader->link();

    /* Create skybox. */
    m_current_skybox_name = m_skybox_names_list[0];
    m_skybox = std::make_shared<Skybox>(m_current_skybox_name,
                                        m_current_skybox_name + "_lf.jpg",
                                        m_current_skybox_name + "_rt.jpg",
                                        m_current_skybox_name + "_up.jpg",
                                        m_current_skybox_name + "_dn.jpg",
                                        m_current_skybox_name + "_ft.jpg",
                                        m_current_skybox_name + "_bk.jpg");
}

void EnvironmentMapping::input()
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
        std::string filename = "08_enviro_mapping";
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

void EnvironmentMapping::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void EnvironmentMapping::render()
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

    for (unsigned i = 0; i < m_objects_model_matrices.size(); ++i)
    {
        m_directional_light_shader->setUniform("model",         m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp",           view_projection * m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("color_tint",    m_color_tints[i]);

        m_objects[i]->render(m_directional_light_shader);
    }

    /* Render skybox */
    m_skybox->render(m_camera->m_projection, m_camera->m_view);
}

void EnvironmentMapping::render_gui()
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
        ImGui::SliderFloat("Ambient color", &m_ambient_factor, 0.0, 1.0,  "%.2f");
        ImGui::SliderFloat("Gamma",         &m_gamma,          0.0, 10.0, "%.1f");

        ImGui::SliderFloat("Alpha cutout threshold", &m_alpha_cutout_threshold, 0.0, 1.0, "%.2f");

        if (ImGui::BeginCombo("Skybox texture", m_current_skybox_name.c_str()))
        {
            for (auto& sf : m_skybox_names_list)
            {
                bool is_selected = (m_current_skybox_name == sf);
                if (ImGui::Selectable(sf.c_str(), is_selected))
                {
                    m_current_skybox_name = sf;

                    m_skybox = std::make_shared<Skybox>(m_current_skybox_name,
                                                        m_current_skybox_name + "_lf.jpg",
                                                        m_current_skybox_name + "_rt.jpg",
                                                        m_current_skybox_name + "_up.jpg",
                                                        m_current_skybox_name + "_dn.jpg",
                                                        m_current_skybox_name + "_ft.jpg",
                                                        m_current_skybox_name + "_bk.jpg");
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
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
