#include "gs_point_sprites.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/random.hpp>

GSPointSprites::GSPointSprites()
    : m_sprites_vao_id(0),
      m_sprites_vbo_id(0),
      m_no_sprites(1000),
      m_half_quad_width(0.2f)
{
}

GSPointSprites::~GSPointSprites()
{
    if(m_sprites_vao_id != 0)
    {
        glDeleteVertexArrays(1, &m_sprites_vao_id);
    }

    if(m_sprites_vbo_id != 0)
    {
        glDeleteBuffers(1, &m_sprites_vbo_id);
    }
}

void GSPointSprites::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-6, 5.0, 10.0);
    m_camera->setOrientation(20.0f, 30.0f, 0.0f);

    /* Add textures to the objects. */
    m_sprite_tex = std::make_shared<RGL::Texture2D>();
    m_sprite_tex->Load(RGL::FileSystem::getPath("textures/sprites/pear-tree.png"), false);

    /* Create shader. */
    std::string dir  = "../src/demos/11_gs_point_sprites/";
    m_point_sprites_shader = std::make_shared<RGL::Shader>(dir + "gs_point_sprites.vert", dir + "gs_point_sprites.frag", dir + "gs_point_sprites.geom");
    m_point_sprites_shader->link();

    /* Create VAO and VBO for point sprites */
    std::vector<glm::vec3> m_sprites_positions(m_no_sprites);

    for (uint32_t i = 0; i < m_no_sprites; ++i)
    {
        m_sprites_positions[i] = glm::sphericalRand(10.0f);
    }

    glCreateVertexArrays(1, &m_sprites_vao_id);

    glCreateBuffers(1, &m_sprites_vbo_id);
    glNamedBufferStorage(m_sprites_vbo_id, m_sprites_positions.size() * sizeof(m_sprites_positions[0]), m_sprites_positions.data(), 0 /*flags*/);

    /* Set up VAO */
    glEnableVertexArrayAttrib(m_sprites_vao_id, 0 /*index*/);

    /* Separate attribute format */
    glVertexArrayAttribFormat (m_sprites_vao_id, 0 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
    glVertexArrayAttribBinding(m_sprites_vao_id, 0 /*index*/, 0 /*bindingindex*/);
    glVertexArrayVertexBuffer (m_sprites_vao_id, 0 /*bindingindex*/, m_sprites_vbo_id, 0 /*offset*/, sizeof(m_sprites_positions[0]) /*stride*/);
}

void GSPointSprites::input()
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
        std::string filename = "11_gs_point_sprites";
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

void GSPointSprites::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void GSPointSprites::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_point_sprites_shader->bind();
    m_point_sprites_shader->setUniform("model_view_matrix", m_camera->m_view);
    m_point_sprites_shader->setUniform("half_quad_width", m_half_quad_width);
    m_point_sprites_shader->setUniform("projection_matrix", m_camera->m_projection);

    m_sprite_tex->Bind(0);
    glBindVertexArray(m_sprites_vao_id);
    glDrawArrays(GL_POINTS, 0, m_no_sprites);
}

void GSPointSprites::render_gui()
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
        ImGui::SliderFloat("Half quad width", &m_half_quad_width, 0.1f, 1.0f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::Spacing();
    }
    ImGui::End();
}
