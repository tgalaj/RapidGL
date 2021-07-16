#include "pbr.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

PBR::PBR()
      : m_dir_light_angles  (0.0f, 0.0f),
        m_spot_light_angles (0.0f, 0.0f),
        m_exposure (1.0f),
        m_gamma (2.2f),
        m_a (1.6f),
        m_d (0.977f),
        m_hdr_max (8.0f),
        m_mid_in (0.18f),
        m_mid_out (0.267f)
{
}

PBR::~PBR()
{
}

void PBR::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 10.0);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 0.2f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    m_point_light_properties[0].color       = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[0].intensity   = 1000.0f;
    m_point_light_properties[0].position    = glm::vec3(-10.0, 10.0, 10.0);

    m_point_light_properties[1].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[1].intensity = 1000.0f;
    m_point_light_properties[1].position  = glm::vec3(10.0, 10.0, 10.0);

    m_point_light_properties[2].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[2].intensity = 1000.0f;
    m_point_light_properties[2].position  = glm::vec3(-10.0, -10.0, 10.0);

    m_point_light_properties[3].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[3].intensity = 1000.0f;
    m_point_light_properties[3].position  = glm::vec3(10.0, -10.0, 10.0);

    m_spot_light_properties.color       = glm::vec3(0.0, 0.0, 1.0);
    m_spot_light_properties.intensity   = 5.0f;
    m_spot_light_properties.position    = glm::vec3(-7.5, 3.0, -5);
    m_spot_light_properties.range       = 35.0f;
    m_spot_light_properties.cutoff      = 45.0f;
    m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);

    /* Create models. */
    m_sphere_model.GenSphere(1.0, 64);

    /* Set model matrices for each model. */
    uint8_t num_rows = 7;
    uint8_t num_cols = 7;
    float spacing    = 2.5;

    for (int row = 0; row < num_rows; ++row)
    {
        //shader.setFloat("metallic", (float)row / (float)nrRows);
        for (int col = 0; col < num_cols; ++col)
        {
            // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            //shader.setFloat("roughness", glm::clamp((float)col / (float)nrColumns, 0.05f, 1.0f));

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((col - (num_cols / 2)) * spacing, (row - (num_rows/ 2)) * spacing, 0.0f));
            m_objects_model_matrices.emplace_back(model);
        }
    }

    /* Add textures to the objects. */
    auto envmap_hdr = std::make_shared<RGL::Texture2D>();
    envmap_hdr->LoadHdr(RGL::FileSystem::getPath("textures/skyboxes/IBL/Newport_Loft.hdr"));

    /* Create shader. */
    std::string dir = "../src/demos/22_pbr/";
    m_ambient_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-ambient.frag");
    m_ambient_light_shader->link();

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-directional.frag");
    m_directional_light_shader->link();

    m_point_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-point.frag");
    m_point_light_shader->link();

    m_spot_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-spot.frag");
    m_spot_light_shader->link();

    m_tmo_ps = std::make_shared<PostprocessFilter>(RGL::Window::getWidth(), RGL::Window::getHeight());
}

void PBR::input()
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
        std::string filename = "22_pbr";
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

void PBR::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void PBR::render()
{
    /* Put render specific code here. Don't update variables here! */
    m_tmo_ps->bindFilterFBO();

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_albedo", glm::vec3(0.5, 0.0, 0.0f));
    m_ambient_light_shader->setUniform("u_ao",     0.9f);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* First, render the ambient color only for the opaque objects. */
    for (unsigned i = 0; i < 7 * 7; ++i)
    {
        //m_ambient_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_ambient_light_shader->setUniform("u_mvp", view_projection * m_objects_model_matrices[i]);

        m_sphere_model.Render();
    }

    /*
     * Disable writing to the depth buffer and additively
     * shade only those pixels, that were shaded in the ambient step.
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);

    /* Render directional light(s) */
    //m_directional_light_shader->bind();

    //m_directional_light_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    //m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    //m_directional_light_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    //
    //m_directional_light_shader->setUniform("cam_pos",            m_camera->position());
    //m_directional_light_shader->setUniform("specular_intensity", m_specular_intenstiy.x);
    //m_directional_light_shader->setUniform("specular_power",     m_specular_power.x);
    //m_directional_light_shader->setUniform("gamma",              m_gamma);

    //for (unsigned i = 0; i < m_objects.size(); ++i)
    //{
    //    m_directional_light_shader->setUniform("model", m_objects_model_matrices[i]);
    //    m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
    //    m_directional_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

    //    m_objects[i].Render();
    //}

    /* Render point lights */
    m_point_light_shader->bind();
    m_point_light_shader->setUniform("u_albedo", glm::vec3(0.5, 0.0, 0.0f));
    m_point_light_shader->setUniform("u_cam_pos", m_camera->position());

    for(uint8_t p = 0; p < std::size(m_point_light_properties); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",      m_point_light_properties[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity",  m_point_light_properties[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",        m_point_light_properties[p].position);
        //m_point_light_shader->setUniform("u_point_light.range",           m_point_light_properties.range);

        for (unsigned row = 0; row < 7; ++row)
        {
            m_point_light_shader->setUniform("u_metallic", float(row)/7.0f);
            for (unsigned col = 0; col < 7; ++col)
            {
                m_point_light_shader->setUniform("u_roughness",     glm::clamp(float(col) / 7.0f, 0.05f, 1.0f));

                uint32_t idx = col + row * 7;
                m_point_light_shader->setUniform("u_model",         m_objects_model_matrices[idx]);
                m_point_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[idx]))));
                m_point_light_shader->setUniform("u_mvp",           view_projection * m_objects_model_matrices[idx]);

                m_sphere_model.Render();
            }
        }
    }
    /* Render spot lights */
    //m_spot_light_shader->bind();

    //m_spot_light_shader->setUniform("spot_light.point.base.color",      m_spot_light_properties.color);
    //m_spot_light_shader->setUniform("spot_light.point.base.intensity",  m_spot_light_properties.intensity);
    //m_spot_light_shader->setUniform("spot_light.point.atten.constant",  m_spot_light_properties.attenuation.constant);
    //m_spot_light_shader->setUniform("spot_light.point.atten.linear",    m_spot_light_properties.attenuation.linear);
    //m_spot_light_shader->setUniform("spot_light.point.atten.quadratic", m_spot_light_properties.attenuation.quadratic);
    //m_spot_light_shader->setUniform("spot_light.point.position",        m_spot_light_properties.position);
    //m_spot_light_shader->setUniform("spot_light.point.range",           m_spot_light_properties.range);
    //m_spot_light_shader->setUniform("spot_light.direction",             m_spot_light_properties.direction);
    //m_spot_light_shader->setUniform("spot_light.cutoff",                glm::radians(90.0f - m_spot_light_properties.cutoff));

    //m_spot_light_shader->setUniform("cam_pos",            m_camera->position());
    //m_spot_light_shader->setUniform("specular_intensity", m_specular_intenstiy.z);
    //m_spot_light_shader->setUniform("specular_power",     m_specular_power.z);
    //m_spot_light_shader->setUniform("gamma",              m_gamma);

    //for (unsigned i = 0; i < m_objects.size(); ++i)
    //{
    //    m_spot_light_shader->setUniform("model", m_objects_model_matrices[i]);
    //    m_spot_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
    //    m_spot_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

    //    m_objects[i].Render();
    //}

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    m_tmo_ps->render(m_exposure, m_gamma, m_a, m_d, m_hdr_max, m_mid_in, m_mid_out);
}

void PBR::render_gui()
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

        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
        ImGui::SliderFloat("Exposure", &m_exposure, 0.0, 10.0, "%.1f");
        ImGui::SliderFloat("Gamma",    &m_gamma,    0.0, 10.0, "%.1f");
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
                    
                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_dir_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            for(uint8_t i = 0; i < std::size(m_point_light_properties); ++i)
            {
                if (ImGui::BeginTabItem(std::string("Point" + std::to_string(i+1)).c_str()))
                {
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                    {
                        ImGui::ColorEdit3 ("Color",              &m_point_light_properties[i].color[0]);
                        ImGui::SliderFloat("Light intensity",    &m_point_light_properties[i].intensity, 0.0, 5000.0,  "%1.f");

                        //ImGui::SliderFloat ("Range",                 &m_point_light_properties[i].range,                 0.01, 100.0, "%.2f");
                        ImGui::SliderFloat3("Position",              &m_point_light_properties[i].position[0],          -10.0, 10.0,  "%.1f");
                    }
                    ImGui::PopItemWidth();
                    ImGui::EndTabItem();
                }
            }
            if (ImGui::BeginTabItem("Spot"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3 ("Color",              &m_spot_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",    &m_spot_light_properties.intensity, 0.0, 100.0, "%.1f");

                    ImGui::SliderFloat("Range",                 &m_spot_light_properties.range,                 0.01, 100.0, "%.2f");
                    ImGui::SliderFloat("Cut-off angle",         &m_spot_light_properties.cutoff,                33.0, 90.0,  "%.1f");
                    ImGui::SliderFloat3("Position",             &m_spot_light_properties.position[0],          -10.0, 10.0,  "%.1f");

                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_spot_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);
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
