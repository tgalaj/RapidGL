#include "mesh_skinning.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"
#include <timer.h>

MeshSkinning::MeshSkinning()
    : m_skinning_method        (SkinningMethod::LBS),
      m_current_animation_index(0),
      m_animation_speed        (1.0f),
      m_gamma                  (0.2f)
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
    m_camera->setPosition(-1.0, 0.5, 1.0);
    m_camera->setOrientation(10.0, 45.0, 0.0);

    /* Create models. */
    m_grid_model.GenPlaneGrid(20, 20, 20, 20);

    m_animated_model.Load(RGL::FileSystem::getResourcesPath() / "models/fox.glb");
    m_animations_names = m_animated_model.GetAnimationsNames();
    m_animated_model.SetAnimationSpeed(m_animation_speed);

    /* Set model matrices for each model. */
    auto scale_factor = m_animated_model.GetUnitScaleFactor();
    m_object_model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));

    /* Create shader. */
    std::string dir = "src/demos/20_mesh_skinning/";

    /* Linear Blend Skinning shader. */
    m_lbs_skinning_shader = std::make_shared<RGL::Shader>(dir + "skinning_lbs.vert", dir + "skinning.frag");
    m_lbs_skinning_shader->link();

    /* Dual Quaternion Blend Skinning shader. */
    m_dqs_skinning_shader = std::make_shared<RGL::Shader>(dir + "skinning_dqs.vert", dir + "skinning.frag");
    m_dqs_skinning_shader->link();

    dir = "src/demos/02_simple_3d/";
    m_simple_shader = std::make_shared<RGL::Shader>(dir + "simple_3d.vert", dir + "simple_3d.frag");
    m_simple_shader->link();
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
            std::cout << "Saved " << filename << ".png to " << RGL::FileSystem::getRootPath() / "screenshots/" << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RGL::FileSystem::getRootPath() / "screenshots/" << std::endl;
        }
    }
}

void MeshSkinning::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);

    switch(m_skinning_method)
    {
        case SkinningMethod::LBS:
            m_bone_transforms.clear();
            m_animated_model.BoneTransform(delta_time, m_bone_transforms);
            break;
        case SkinningMethod::DQS:
            m_bone_transforms_dq.clear();
            m_animated_model.BoneTransform(delta_time, m_bone_transforms_dq);
            break;
    }
}

void MeshSkinning::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Draw the grid. */
    m_simple_shader->bind();
    m_simple_shader->setUniform("mvp",        view_projection);
    m_simple_shader->setUniform("color",      glm::vec3(0.4));
    m_simple_shader->setUniform("mix_factor", 1.0f);
    m_grid_model.Render();

    /* Draw the animated model. */
    if (m_skinning_method == SkinningMethod::LBS)
    {
        m_lbs_skinning_shader->bind();
        m_lbs_skinning_shader->setUniform("mvp",   view_projection * m_object_model_matrix);
        m_lbs_skinning_shader->setUniform("model", m_object_model_matrix);
        m_lbs_skinning_shader->setUniform("bones", m_bone_transforms.data(), m_bone_transforms.size());
        m_lbs_skinning_shader->setUniform("gamma", m_gamma);
    }

    if (m_skinning_method == SkinningMethod::DQS)
    {
        m_dqs_skinning_shader->bind();
        m_dqs_skinning_shader->setUniform("mvp",   view_projection * m_object_model_matrix);
        m_dqs_skinning_shader->setUniform("model", m_object_model_matrix);
        m_dqs_skinning_shader->setUniform("bones", m_bone_transforms_dq.data(), m_bone_transforms_dq.size());
        m_dqs_skinning_shader->setUniform("gamma", m_gamma);
    }

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

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
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

        if (ImGui::BeginCombo("Skinning method", m_skinning_methods_names[int(m_skinning_method)].c_str()))
        {
            for (int i = 0; i < std::size(m_skinning_methods_names); ++i)
            {
                bool is_selected = (int(m_skinning_method) == i);
                if (ImGui::Selectable(m_skinning_methods_names[i].c_str(), is_selected))
                {
                    m_skinning_method = SkinningMethod(i);
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
