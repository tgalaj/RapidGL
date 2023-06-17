#include "enviro_mapping.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>

EnvironmentMapping::EnvironmentMapping()
    : m_specular_power               (120.0f),
      m_specular_intenstiy           (0.0f),
      m_ambient_factor               (0.18f),
      m_gamma                        (2.2),
      m_dir_light_angles             (45.0f, 50.0f),
      m_ior                          (1.52f),
      m_dynamic_enviro_mapping_toggle(false)
{
    m_enviro_cubemap_size = glm::vec2(2048);

    float half_size = m_enviro_cubemap_size.x * 0.5f;
    m_enviro_projection = glm::perspective(2.0f * glm::atan(half_size / (half_size - 0.5f)), 1.0f, 0.01f, 200.0f);
}

EnvironmentMapping::~EnvironmentMapping()
{
    for(auto & rt : m_cubemap_rts)
    {
        rt.cleanup();
    }
}

void EnvironmentMapping::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.76, 0.913, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(0.0, 5.0, 9.0);
    m_camera->setOrientation(glm::vec3(0.0, 3.0, -9.0));

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 3.5f;
    m_dir_light_properties.setDirection(m_dir_light_angles);

    /* Create models. */
    m_objects.emplace_back(std::make_shared<RGL::StaticModel>());
    m_objects[0]->Load(RGL::FileSystem::getResourcesPath() / "models/xyzrgb_dragon.obj");
    m_xyzrgb_dragon = m_objects[0];

    m_objects.emplace_back(std::make_shared<RGL::StaticModel>());
    m_objects[1]->Load(RGL::FileSystem::getResourcesPath() / "models/lucy.obj");
    m_lucy = m_objects[1];

    constexpr auto kRadius    = 2.5f; 
    constexpr float area_size = 15.0f;

    m_objects.emplace_back(std::make_shared<RGL::StaticModel>());
    m_objects[2]->GenPlane(area_size * 2.0 + kRadius, area_size * 2.0 + kRadius, area_size * 2.0, area_size * 2.0);
    m_ground_plane = m_objects[2];

    /* Set model matrices for each model. */
    /* xyzrgb dragon */
    glm::vec3 xyzrgb_dragon_position = glm::vec3(-4.0f, 1.11f, -1.0f);
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), xyzrgb_dragon_position) * glm::rotate(glm::mat4(1.0f), glm::radians(-45.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.04)));

    /* lucy */
    glm::vec3 lucy_position = glm::vec3(4.0f, 1.81f, 0.0f);
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), lucy_position) * glm::rotate(glm::mat4(1.0f), glm::radians(135.0f), glm::vec3(0.0, 1.0, 0.0)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(0.004)));

    /* plane */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(0.0, -0.5, 0.0)));

    /* Set the color tints for xyzrgb dragon, lucy and the plane respectively. */
    m_color_tints.emplace_back(glm::vec3(1.0));
    m_color_tints.emplace_back(glm::vec3(1.0));
    m_color_tints.emplace_back(glm::vec3(1.0));

    /* Add textures to the objects. */
    auto default_diffuse_texture = std::make_shared<RGL::Texture2D>();
    default_diffuse_texture->Load(RGL::FileSystem::getResourcesPath() / "textures/default_diffuse.png", true);

    m_xyzrgb_dragon->AddTexture(default_diffuse_texture);
    m_lucy->AddTexture(default_diffuse_texture);

    auto ground_texture = std::make_shared<RGL::Texture2D>();
    ground_texture->Load(RGL::FileSystem::getResourcesPath() / "textures/grass_green_d.jpg", true);
    ground_texture->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT);
    ground_texture->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
    ground_texture->SetAnisotropy(16);

    m_ground_plane->AddTexture(ground_texture);

    constexpr uint8_t no_spheres = 1000;
    constexpr float max_sphere_radius = 0.4f;

    for (int i = 0; i < no_spheres; ++i)
    {
        float rand_radius = RGL::Util::RandomDouble(0.1, max_sphere_radius);
        m_objects.emplace_back(std::make_shared<RGL::StaticModel>());
        m_objects[3 + i]->GenSphere(rand_radius, 20);
        m_objects[3 + i]->AddTexture(default_diffuse_texture);

        glm::vec3 random_position = glm::sphericalRand(16.0f);
        if (random_position.y < -0.5f)
        {
            auto offset = glm::abs(-0.5f - random_position.y) + rand_radius;
            random_position.y += offset;
        }

        glm::vec3 random_color = glm::linearRand(glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0));

        m_spheres_positions.emplace_back(random_position);
        m_random_spheres_rotation_speeds.emplace_back(RGL::Util::RandomDouble(0.1, 0.7));

        m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), random_position));
        m_color_tints.emplace_back(random_color);
    }

    /* Create shader. */
    std::string dir          = "src/demos/08_enviro_mapping/";
    std::string dir_lighting = "src/demos/03_lighting/";

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir_lighting + "lighting.vert", dir + "lighting-directional.frag");
    m_directional_light_shader->link();

    m_enviro_mapping_shader = std::make_shared<RGL::Shader>(dir + "enviro_mapping.vert", dir + "enviro_mapping.frag");
    m_enviro_mapping_shader->link();

    /* Create skybox. */
    m_current_skybox_name = m_skybox_names_list[0];
    m_skybox = std::make_shared<Skybox>(m_current_skybox_name,
                                        m_current_skybox_name + "_lf.jpg",
                                        m_current_skybox_name + "_rt.jpg",
                                        m_current_skybox_name + "_up.jpg",
                                        m_current_skybox_name + "_dn.jpg",
                                        m_current_skybox_name + "_ft.jpg",
                                        m_current_skybox_name + "_bk.jpg");

    /* Generate cubemap render targets*/
    m_cubemap_rts[0] = generate_cubemap_rt();
    m_cubemap_rts[1] = generate_cubemap_rt();

    m_cubemap_rts[0].set_position(xyzrgb_dragon_position);
    m_cubemap_rts[1].set_position(lucy_position);
}

void EnvironmentMapping::input()
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
        std::string filename = "08_enviro_mapping";
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

void EnvironmentMapping::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);

    /* Update model matrices of the spheres */
    static float rotation_angle = 0.0f;

    rotation_angle += delta_time;

    for (int i = 3; i < m_objects_model_matrices.size(); ++i)
    {
        auto translate     = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f));
        auto inv_translate = glm::inverse(translate);
        auto transform     = translate * glm::rotate(glm::mat4(1.0f), rotation_angle * m_random_spheres_rotation_speeds[i - 3], glm::vec3(0.0, 1.0, 0.0)) * inv_translate;

        m_objects_model_matrices[i] = transform * glm::translate(glm::mat4(1.0f), m_spheres_positions[i - 3]);
    }
}

void EnvironmentMapping::render()
{
    /* First pass: render scene to cubemap render targets */
    if (m_dynamic_enviro_mapping_toggle)
    {
        glEnable(GL_CULL_FACE);
        render_to_cubemap_rt(m_cubemap_rts[0], 0);
        render_to_cubemap_rt(m_cubemap_rts[1], 1);
        glDisable(GL_CULL_FACE);
    }

    /* Second pass: render scene normally */
    glViewport(0, 0, RGL::Window::getWidth(), RGL::Window::getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render_objects(m_camera->m_view, m_camera->m_projection, m_camera->position());
}

void EnvironmentMapping::render_objects(const glm::mat4& camera_view, const glm::mat4& camera_projection, const glm::vec3& camera_position, int ignore_obj_id)
{
    auto view_projection = camera_projection * camera_view;

    /* Render directional light(s) */
    m_directional_light_shader->bind();

    m_directional_light_shader->setUniform("directional_light.base.color", m_dir_light_properties.color);
    m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("directional_light.direction", m_dir_light_properties.direction);

    m_directional_light_shader->setUniform("cam_pos", camera_position);
    m_directional_light_shader->setUniform("specular_intensity", m_specular_intenstiy.x);
    m_directional_light_shader->setUniform("specular_power", m_specular_power.x);
    m_directional_light_shader->setUniform("gamma", m_gamma);
    m_directional_light_shader->setUniform("ambient_factor", m_ambient_factor);

    for (unsigned i = 2; i < m_objects_model_matrices.size(); ++i)
    {
        m_directional_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("color_tint", m_color_tints[i]);

        m_objects[i]->Render();
    }

    /* Render reflective / refractive models */
    m_enviro_mapping_shader->bind();
    m_enviro_mapping_shader->setUniform("cam_pos", camera_position);

    if(!m_dynamic_enviro_mapping_toggle) m_skybox->bindSkyboxTexture(1);

    /* xyzrgb dragon */
    if (ignore_obj_id != 0)
    {
        if (m_dynamic_enviro_mapping_toggle) m_cubemap_rts[0].bindTexture(1);

        m_enviro_mapping_shader->setSubroutine(RGL::Shader::ShaderType::FRAGMENT, "reflection");
        m_enviro_mapping_shader->setUniform("model", m_objects_model_matrices[0]);
        m_enviro_mapping_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[0]))));
        m_enviro_mapping_shader->setUniform("mvp", view_projection * m_objects_model_matrices[0]);
        m_xyzrgb_dragon->Render();
    }

    /* lucy */
    if (ignore_obj_id != 1)
    {
        if (m_dynamic_enviro_mapping_toggle) m_cubemap_rts[1].bindTexture(1);

        m_enviro_mapping_shader->setSubroutine(RGL::Shader::ShaderType::FRAGMENT, "refraction");
        m_enviro_mapping_shader->setUniform("ior", m_ior);
        m_enviro_mapping_shader->setUniform("model", m_objects_model_matrices[1]);
        m_enviro_mapping_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[1]))));
        m_enviro_mapping_shader->setUniform("mvp", view_projection * m_objects_model_matrices[1]);
        m_lucy->Render();
    }

    /* Render skybox */
    m_skybox->render(camera_projection, camera_view);
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

        ImGui::SliderFloat("Index of Refraction", &m_ior, 1.0, 2.417, "%.3f");
        ImGui::Checkbox("Dynamic Environment Mapping", &m_dynamic_enviro_mapping_toggle);

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

EnvironmentMapping::CubeMapRenderTarget EnvironmentMapping::generate_cubemap_rt() const
{
    CubeMapRenderTarget rt;

    glGenTextures(1, &rt.m_cubemap_texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, rt.m_cubemap_texture_id);

    for (uint8_t i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, m_enviro_cubemap_size.x, m_enviro_cubemap_size.y, 0, GL_RGB, GL_FLOAT, 0);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &rt.m_fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, rt.m_fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, rt.m_cubemap_texture_id, 0);

    glGenRenderbuffers(1, &rt.m_rbo_id);
    glBindRenderbuffer(GL_RENDERBUFFER, rt.m_rbo_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_enviro_cubemap_size.x, m_enviro_cubemap_size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rt.m_rbo_id);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return rt;
}

void EnvironmentMapping::render_to_cubemap_rt(CubeMapRenderTarget & rt, int ignore_obj_id)
{
    /* Update all faces per frame */
    glViewport(0, 0, m_enviro_cubemap_size.x, m_enviro_cubemap_size.y);
    glBindFramebuffer(GL_FRAMEBUFFER, rt.m_fbo_id);
        for(uint8_t side = 0; side < 6; ++side)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, rt.m_cubemap_texture_id, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render_objects(rt.m_view_transforms[side], m_enviro_projection, rt.m_position, ignore_obj_id);
        }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
