#include "projected_texture.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

ProjectedTexture::ProjectedTexture()
    : m_specular_power    (120.0f),
      m_specular_intenstiy(0.2f),
      m_ambient_factor    (0.18f),
      m_gamma             (0.8),
      m_spot_light_angles (0.0f, 0.0f),
      m_projector_move_speed   (0.75f)
{
}

ProjectedTexture::~ProjectedTexture()
{
}

void ProjectedTexture::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 10.0);

    /* Initialize lights' properties */
    m_spot_light_properties.color       = glm::vec3(0.8);
    m_spot_light_properties.intensity   = 5.0f;
    m_spot_light_properties.attenuation = { 1.0f, 0.1f, 0.01f };
    m_spot_light_properties.position    = glm::vec3(-7.5, 3.0, -5.0);
    m_spot_light_properties.range       = 35.0f;
    m_spot_light_properties.cutoff      = 45.0f;
    m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);

    /* Create models. */
    for (unsigned i = 0; i < 9; ++i)
    {
        m_objects.emplace_back(std::make_shared<RGL::StaticModel>());
    }

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0]->Load(RGL::FileSystem::getPath("models/bunny.obj"));
    m_objects[1]->GenCone(1.0, 0.5);
    m_objects[2]->GenCube();
    m_objects[3]->GenCylinder(1.0, 0.5);
    m_objects[4]->GenPlane();
    m_objects[5]->GenSphere(0.5);
    m_objects[6]->GenTorus(0.5, 1.0);
    m_objects[7]->GenQuad();
    m_objects[8]->GenPlane(50, 50);

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-7.5, -1.0, -5)));                                                                         // bunny
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-5.0,  0.5, -5)));                                                                         // cone
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-2.5,  0.0, -5)));                                                                         // cube
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0,  0.0, -5)));                                                                         // cylinder
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 2.5,  0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(45.0f), glm::vec3(1, 0, 0)));  // plane
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 5.0,  0.0, -5)));                                                                         // sphere
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 7.5,  0.0, -5)));                                                                         // torus
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(10.0,  0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(45.0f), glm::vec3(1, 0, 0)));  // quad
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0, -1.0, -5)));                                                                         // ground plane

    /* Add textures to the objects. */
    auto texture = std::make_shared<RGL::Texture2D>();
    texture->Load(RGL::FileSystem::getPath("textures/bricks.png"), true);

    auto default_diffuse_texture = std::make_shared<RGL::Texture2D>();
    default_diffuse_texture->Load(RGL::FileSystem::getPath("textures/default_diffuse.png"), true);

    m_objects[5]->AddTexture(texture);

    for (auto& model : m_objects)
    {
        if (&model != &m_objects[5])
        {
            model->AddTexture(default_diffuse_texture);
        }
    }

    /* Create shader. */
    std::string dir = "../src/demos/09_projected_texture/";
    m_ambient_light_shader = std::make_shared<RGL::Shader>(dir + "lighting.vert", dir + "lighting-ambient.frag");
    m_ambient_light_shader->link();

    m_spot_light_shader = std::make_shared<RGL::Shader>(dir + "lighting-spot.vert", dir + "lighting-spot.frag");
    m_spot_light_shader->link();

    /* Load texture to be projected and adjust its parameters */
    m_projector.m_texture.Load(RGL::FileSystem::getPath("textures/circles/" + m_current_projector_texture_name), true);
    m_projector.m_texture.SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::CLAMP_TO_BORDER);
    m_projector.m_texture.SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::CLAMP_TO_BORDER);

    /* Set up the projectors' matrix */
    m_projector.m_view_matrix       = glm::lookAt(m_spot_light_properties.position, m_spot_light_properties.position + m_spot_light_properties.direction, glm::cross(m_spot_light_properties.direction, glm::vec3(1.0, 0.0, 0.0)));
    m_projector.m_projection_matrix = glm::perspective(glm::radians(m_spot_light_properties.cutoff), 1.0f, 1.0f, 400.0f);
    m_projector.m_bias_matrix       = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f));
    m_projector.m_bias_matrix       = glm::scale(m_projector.m_bias_matrix, glm::vec3(0.25f));
}

void ProjectedTexture::input()
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
        std::string filename = "09_projected_texture";
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

void ProjectedTexture::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);

    static float accum = 0.0f;
    accum += delta_time * m_projector_move_speed;

    static constexpr float min = -7.5f;
    static constexpr float max = 10.0f;
    m_spot_light_properties.position.x = min + (max - min) * 0.5f * (1.0 + glm::sin(accum));
}

void ProjectedTexture::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("ambient_factor", m_ambient_factor);
    m_ambient_light_shader->setUniform("gamma",          m_gamma);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* First, render the ambient color only for the opaque objects. */
    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        //m_ambient_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_ambient_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->Render();
    }

    /*
     * Disable writing to the depth buffer and additively
     * shade only those pixels, that were shaded in the ambient step.
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);

    /* Render spot lights */
    m_spot_light_shader->bind();

    m_spot_light_shader->setUniform("spot_light.point.base.color",      m_spot_light_properties.color);
    m_spot_light_shader->setUniform("spot_light.point.base.intensity",  m_spot_light_properties.intensity);
    m_spot_light_shader->setUniform("spot_light.point.atten.constant",  m_spot_light_properties.attenuation.constant);
    m_spot_light_shader->setUniform("spot_light.point.atten.linear",    m_spot_light_properties.attenuation.linear);
    m_spot_light_shader->setUniform("spot_light.point.atten.quadratic", m_spot_light_properties.attenuation.quadratic);
    m_spot_light_shader->setUniform("spot_light.point.position",        m_spot_light_properties.position);
    m_spot_light_shader->setUniform("spot_light.point.range",           m_spot_light_properties.range);
    m_spot_light_shader->setUniform("spot_light.direction",             m_spot_light_properties.direction);
    m_spot_light_shader->setUniform("spot_light.cutoff",                glm::radians(90.0f - m_spot_light_properties.cutoff));

    m_spot_light_shader->setUniform("cam_pos",            m_camera->position());
    m_spot_light_shader->setUniform("specular_intensity", m_specular_intenstiy.z);
    m_spot_light_shader->setUniform("specular_power",     m_specular_power.z);
    m_spot_light_shader->setUniform("gamma",              m_gamma);

    /* Projector texture uniforms */
    m_projector.m_texture.Bind(1);
    m_projector.m_view_matrix = glm::lookAt(m_spot_light_properties.position, m_spot_light_properties.position + m_spot_light_properties.direction, glm::cross(m_spot_light_properties.direction, glm::vec3(1.0, 0.0, 0.0)));
    m_spot_light_shader->setUniform("projector_matrix", m_projector.transform());

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_spot_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_spot_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_spot_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->Render();
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

void ProjectedTexture::render_gui()
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

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
        ImGui::SliderFloat("Ambient color", &m_ambient_factor, 0.0, 1.0,  "%.2f");
        ImGui::SliderFloat("Gamma",         &m_gamma,          0.0, 10.0, "%.1f");

        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Spot and Projector"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3 ("Color",              &m_spot_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",    &m_spot_light_properties.intensity, 0.0, 100.0, "%.1f");
                    ImGui::SliderFloat("Specular power",     &m_specular_power.z,                1.0, 120.0, "%.0f");
                    ImGui::SliderFloat("Specular intensity", &m_specular_intenstiy.z,            0.0, 1.0,   "%.2f");

                    ImGui::SliderFloat("Constant attenuation",  &m_spot_light_properties.attenuation.constant,  0.01, 10.0,  "%.2f");
                    ImGui::SliderFloat("Linear attenuation",    &m_spot_light_properties.attenuation.linear,    0.01, 10.0,  "%.2f");
                    ImGui::SliderFloat("Quadratic attenuation", &m_spot_light_properties.attenuation.quadratic, 0.01, 10.0,  "%.2f");
                    ImGui::SliderFloat("Range",                 &m_spot_light_properties.range,                 0.01, 100.0, "%.2f");

                    if(ImGui::SliderFloat("Cut-off angle", &m_spot_light_properties.cutoff, 33.0, 90.0, "%.1f"))
                    {
                        m_projector.m_projection_matrix = glm::perspective(glm::radians(m_spot_light_properties.cutoff), 1.0f, 1.0f, 400.0f);
                    }

                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_spot_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);
                    }

                    ImGui::SliderFloat("Projector's move speed", &m_projector_move_speed, 0.0f, 5.0f, "%.1f");

                    if (ImGui::BeginCombo("Projector texture", m_current_projector_texture_name.c_str()))
                    {
                        for (auto& sf : m_projector_textures_names_list)
                        {
                            bool is_selected = (m_current_projector_texture_name == sf);
                            if (ImGui::Selectable(sf.c_str(), is_selected))
                            {
                                m_current_projector_texture_name = sf;

                                m_projector.m_texture.Load(RGL::FileSystem::getPath("textures/circles/" + m_current_projector_texture_name), true);
                                m_projector.m_texture.SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::CLAMP_TO_BORDER);
                                m_projector.m_texture.SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::CLAMP_TO_BORDER);
                            }

                            if (is_selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
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
