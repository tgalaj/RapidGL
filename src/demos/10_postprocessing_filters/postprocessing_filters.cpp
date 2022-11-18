#include "postprocessing_filters.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

PostprocessingFilters::PostprocessingFilters()
    : m_specular_power    (120.0f),
      m_specular_intenstiy(0.0f),
      m_ambient_factor    (0.5f),
      m_dir_light_angles  (60.0f, 40.0f)
{
}

PostprocessingFilters::~PostprocessingFilters()
{
}

void PostprocessingFilters::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-6, 5.0, 10.0);
    m_camera->setOrientation(20.0f, 30.0f, 0.0f);

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 1.5f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    /* Create models. */
    for (unsigned i = 0; i < 5; ++i)
    {
        m_objects.emplace_back(std::make_shared<RGL::StaticModel>());
    }

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0]->GenCube(2);
    m_objects[1]->GenCube(1.5);
    m_objects[2]->GenCube(1);
    m_objects[3]->GenCube(3);
    m_objects[4]->GenPlane(30, 30);

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-6, 0.0, -5))); // crate 0
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(2.0, 0.28, 0)) * 
                                          glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 0, 1)) * 
                                          glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1, 0, 0))); // crate 1
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-2.0,  -0.5, 2))); // crate 2
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(6.5, 0.5, -7)));  // crate 3
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0, -1.0, -5))); // ground plane

    /* Add textures to the objects. */
    auto crate_texture = std::make_shared<RGL::Texture2D>();
    crate_texture->Load(RGL::FileSystem::getPath("textures/crate/crate0_diffuse.png"), true);

    auto ground_texture = std::make_shared<RGL::Texture2D>();
    ground_texture->Load(RGL::FileSystem::getPath("textures/ground.png"), true);
    ground_texture->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT);
    ground_texture->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
    ground_texture->SetAnisotropy(16);

    m_objects[0]->AddTexture(crate_texture);
    m_objects[1]->AddTexture(crate_texture);
    m_objects[2]->AddTexture(crate_texture);
    m_objects[3]->AddTexture(crate_texture);
    m_objects[4]->AddTexture(ground_texture);

    /* Create shader. */
    std::string dir  = "../src/demos/03_lighting/";
    std::string dir2 = "../src/demos/10_postprocessing_filters/";
    m_ambient_light_shader = std::make_shared<RGL::Shader>(dir + "lighting.vert", dir2 + "lighting-ambient.frag");
    m_ambient_light_shader->link();

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir + "lighting.vert", dir2 + "lighting-directional.frag");
    m_directional_light_shader->link();

    m_postprocess_filter = std::make_shared<PostprocessFilter>(RGL::Window::getWidth(), RGL::Window::getHeight());
    m_current_ps_filter_name = m_ps_filter_names_list[0];
}

void PostprocessingFilters::input()
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
        std::string filename = "10_postprocessing_filters";
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

void PostprocessingFilters::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void PostprocessingFilters::render()
{
    /* First pass - render to offscreen FBO */
    m_postprocess_filter->bindFilterFBO();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("ambient_factor", m_ambient_factor);

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

    /* Render directional light(s) */
    m_directional_light_shader->bind();

    m_directional_light_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    
    m_directional_light_shader->setUniform("cam_pos",            m_camera->position());
    m_directional_light_shader->setUniform("specular_intensity", m_specular_intenstiy.x);
    m_directional_light_shader->setUniform("specular_power",     m_specular_power.x);

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_directional_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->Render();
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    /* Render FSQ with postprocess filter */
    m_postprocess_filter->render(m_current_ps_filter_name);
}

void PostprocessingFilters::render_gui()
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

        if (ImGui::BeginCombo("Postprocess filter", m_current_ps_filter_name.c_str()))
        {
            for (auto& sf : m_ps_filter_names_list)
            {
                bool is_selected = (m_current_ps_filter_name == sf);
                if (ImGui::Selectable(sf.c_str(), is_selected))
                {
                    m_current_ps_filter_name = sf;
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
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                {
                    ImGui::ColorEdit3 ("Color",                 &m_dir_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",       &m_dir_light_properties.intensity, 0.0, 10.0,  "%.1f");
                    ImGui::SliderFloat("Specular power",        &m_specular_power.x,               1.0, 120.0, "%.0f");
                    ImGui::SliderFloat("Specular intensity",    &m_specular_intenstiy.x,           0.0, 1.0,   "%.2f");
                    
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
