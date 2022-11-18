#include "oit.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

OIT::OIT() 
    : m_fsq_vao              (0), 
      m_linked_lists_buffer  (0), 
      m_list_info_buffer     (0), 
      m_head_pointers_image2d(0), 
      m_max_nodes            (0),
      m_grid_dimensions      (5, 5, 3), 
      m_transparency         (0.4f)
{
}

OIT::~OIT()
{
    glDeleteVertexArrays(1, &m_fsq_vao);
    m_fsq_vao = 0;

    glDeleteBuffers(1, &m_linked_lists_buffer);
    m_linked_lists_buffer = 0;
    
    glDeleteBuffers(1, &m_list_info_buffer);
    m_list_info_buffer = 0;

    glDeleteTextures(1, &m_head_pointers_image2d);
    m_head_pointers_image2d = 0;
}

void OIT::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glDisable  (GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable   (GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-15.0, 0.0, 20.0);
    m_camera->setOrientation(0, 37.0, 0.0);

    /* Create models. */
    float radius = 1.5f;
    m_sphere_model.GenSphere(radius, 36);
    
    /* Set model matrices for each model. */
    glm::vec3 offset    = glm::vec3(2, 2, 5);
    glm::vec3 start_pos = -((radius + offset) * (m_grid_dimensions - 1.0f)) / 2.0f;

    for (uint32_t i = 0; i < m_grid_dimensions.z; ++i)
    {
        for (uint32_t j = 0; j < m_grid_dimensions.y; ++j)
        {
            for (uint32_t k = 0; k < m_grid_dimensions.x; ++k)
            {
                glm::vec3 position = start_pos + glm::vec3(k, j, i) * (radius + offset);
                m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), position));
            }
        }
    }

    m_dragon_model.Load(RGL::FileSystem::getPath("models/dragon.obj"));
    m_dragon_model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(m_dragon_model.GetUnitScaleFactor() * 25.0f));
    m_dragon_color        = glm::vec3(0, 1, 0);

    /* Set colors for the consecutive layers of cubes. */
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.0, 1.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 1.0, 0.0));
    m_objects_colors.emplace_back(glm::vec3(1.0, 0.0, 0.0));

    /* Create shader. */
    std::string dir = "../src/demos/21_oit/";
    m_oit_linked_list_shader = std::make_shared<RGL::Shader>(dir + "oit.vert", dir + "oit_linked_list.frag");
    m_oit_linked_list_shader->link();

    std::string dir2 = "../src/demos/10_postprocessing_filters/";
    m_oit_render_shader = std::make_shared<RGL::Shader>(dir2 + "FSQ.vert", dir + "oit_render.frag");
    m_oit_render_shader->link();

    /* Prepare GL objects */
          m_max_nodes    = 20 * RGL::Window::getWidth() * RGL::Window::getHeight();
    GLint list_node_size = sizeof(ListNode) + sizeof(uint32_t);

    glCreateBuffers(1, &m_linked_lists_buffer);
    glNamedBufferStorage(m_linked_lists_buffer, m_max_nodes * list_node_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_linked_lists_buffer);

    uint32_t list_info_data[] = { 0, m_max_nodes };
    glCreateBuffers(1, &m_list_info_buffer);
    glNamedBufferStorage(m_list_info_buffer, sizeof(uint32_t) * 2, list_info_data, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_list_info_buffer);

    m_head_pointers_clear_data = std::vector<uint32_t>(RGL::Window::getWidth() * RGL::Window::getHeight(), 0xffffffff);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_head_pointers_image2d);
    glTextureStorage2D(m_head_pointers_image2d, 1, GL_R32UI, RGL::Window::getWidth(), RGL::Window::getHeight());
    
    glTextureParameteri(m_head_pointers_image2d, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_head_pointers_image2d, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameterf(m_head_pointers_image2d, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameterf(m_head_pointers_image2d, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glClearTexImage(m_head_pointers_image2d, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, m_head_pointers_clear_data.data());
    glBindImageTexture(0, m_head_pointers_image2d, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glCreateVertexArrays(1, &m_fsq_vao);
}

void OIT::input()
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
        std::string filename = "21_oit";
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

void OIT::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void OIT::render()
{
    /* Clear list info SSBO and head_pointers_image2d before rendering. */
    uint32_t zero = 0;
    glNamedBufferSubData(m_list_info_buffer, 0 /*offset*/, sizeof(uint32_t), &zero); // clear next_node_counter only
    glClearTexImage(m_head_pointers_image2d, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, m_head_pointers_clear_data.data());

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Pass 1 - create the linked lists. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_oit_linked_list_shader->bind();
    m_oit_linked_list_shader->setUniform("light.intensity", 1.0f);
    m_oit_linked_list_shader->setUniform("light.direction", glm::vec3(1, -1, 0));
    m_oit_linked_list_shader->setUniform("cam_pos",         m_camera->position());
    m_oit_linked_list_shader->setUniform("transparency",    m_transparency);

    if (m_current_model == 0)
    {
        for (int i = m_grid_dimensions.z - 1; i >= 0; --i)
        {
            for (uint32_t j = 0; j < m_grid_dimensions.y; ++j)
            {
                for (uint32_t k = 0; k < m_grid_dimensions.x; ++k)
                {
                    uint32_t index = k + j * m_grid_dimensions.x + i * m_grid_dimensions.x * m_grid_dimensions.y;
                    m_oit_linked_list_shader->setUniform("light.color",     m_objects_colors[i]);
                    m_oit_linked_list_shader->setUniform("mvp",             view_projection * m_objects_model_matrices[index]);
                    m_oit_linked_list_shader->setUniform("model_matrix",    m_objects_model_matrices[index]);
                    m_sphere_model.Render();
                }
            }
        }
    }
    else
    {
        m_oit_linked_list_shader->setUniform("light.color",  m_dragon_color);
        m_oit_linked_list_shader->setUniform("mvp",          view_projection * m_dragon_model_matrix);
        m_oit_linked_list_shader->setUniform("model_matrix", m_dragon_model_matrix);
        m_dragon_model.Render();
    }

    /* Make sure that GPU finished writing to SSBOs and the image. */
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    
    /* Pass 2 - render OIT. */    
    m_oit_render_shader->bind();
    glBindVertexArray(m_fsq_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void OIT::render_gui()
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

        ImGui::SliderFloat("Transparency", &m_transparency, 0.0, 1.0, "%.2f");

        if (ImGui::BeginCombo("Model", m_models_names_combo_box[m_current_model].c_str()))
        {
            for (int i = 0; i < std::size(m_models_names_combo_box); ++i)
            {
                bool is_selected = (m_current_model == i);
                if (ImGui::Selectable(m_models_names_combo_box[i].c_str(), is_selected))
                {
                    m_current_model = i;
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();
}
