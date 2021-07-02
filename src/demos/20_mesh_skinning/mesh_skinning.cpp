#include "mesh_skinning.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"
#include <timer.h>

MeshSkinning::MeshSkinning()
    : m_current_animation_index(0),
      m_animation_speed        (40.0f),
      m_gamma                  (0.4f)
{
}

MeshSkinning::~MeshSkinning()
{
}

void MeshSkinning::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-2.0, 1.0, 2.0);
    m_camera->setOrientation(10.0, 45.0, 0.0);

    /* Create models. */
    m_animated_model.Load(RGL::FileSystem::getPath("models/fox.glb"));
    m_animations_names = m_animated_model.GetAnimationsNames();
    m_animated_model.SetAnimationSpeed(m_animation_speed);

    /* Set model matrices for each model. */
    m_object_model_matrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 0.0)) * glm::rotate(glm::mat4(1.0), glm::radians(0.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.01));

    /* Create shader. */
    std::string dir = "../src/demos/20_mesh_skinning/";
    m_simple_skinning_shader = std::make_shared<RGL::Shader>(dir + "skinning.vert", dir + "skinning.frag");
    m_simple_skinning_shader->link();
}

void MeshSkinning::input()
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
        std::string filename = "20_mesh_skinning";
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

void MeshSkinning::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);

    m_bone_transforms.clear();
    m_animated_model.BoneTransform(delta_time, m_bone_transforms);
}

void MeshSkinning::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_simple_skinning_shader->bind();

    auto view_projection = m_camera->m_projection * m_camera->m_view;
    
    m_simple_skinning_shader->setUniform("mvp",   view_projection * m_object_model_matrix);
    m_simple_skinning_shader->setUniform("model", m_object_model_matrix);
    m_simple_skinning_shader->setUniform("bones", m_bone_transforms.data(), m_bone_transforms.size());
    m_simple_skinning_shader->setUniform("gamma", m_gamma);
    m_animated_model.Render();
}

void MeshSkinning::render_gui()
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
                        "WASDQE - control camera movement\n"
                        "RMB    - press to rotate the camera\n"
                        "Esc    - close the app\n\n");
        }

        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
        ImGui::Spacing();

        ImGui::SliderFloat("Gamma", &m_gamma, 0.0, 2.5 , "%.1f");

        if (ImGui::SliderFloat("Animation speed", &m_animation_speed, 0.0, 500.0, "%.1f"))
        {
            m_animated_model.SetAnimationSpeed(m_animation_speed);
        }

        if (ImGui::BeginCombo("Animation", m_animations_names[m_current_animation_index].c_str()))
        {
            for (int i = 0; i < m_animations_names.size(); ++i)
            {
                bool is_selected = (m_current_animation_index == i);
                if (ImGui::Selectable(m_animations_names[i].c_str(), is_selected))
                {
                    m_current_animation_index = i;
                    m_animated_model.SetAnimation(i);
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
    }
    ImGui::End();
}
