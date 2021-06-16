#include "ts_quad.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/random.hpp>

Tessellation2D::Tessellation2D()
    : m_quad_points_vao_id(0),
      m_quad_points_vbo_id(0),
      m_no_quad_points(4),
      m_quad_color(180 / 255.0f),
      m_line_color(0.0, 0.0, 0.0),
      m_line_width(0.5),
      m_outer(2),
      m_inner(2)
{
}

Tessellation2D::~Tessellation2D()
{
    if(m_quad_points_vao_id != 0)
    {
        glDeleteVertexArrays(1, &m_quad_points_vao_id);
    }

    if(m_quad_points_vbo_id != 0)
    {
        glDeleteBuffers(1, &m_quad_points_vbo_id);
    }
}

void Tessellation2D::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glPointSize(10.0f);
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    GLint maxVerts;
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxVerts);
    printf("Max patch vertices: %d\n", maxVerts);

    /* Create virtual camera. */
    const float c = 3.5f;
    m_camera = std::make_shared<RGL::Camera>(-0.4f * c, 0.7f * c, -0.35f * c, 0.4f * c, 0.1f, 100.0f);
    m_camera->setPosition(0.0, 0.0, 1.5);

    /* Create shader. */
    std::string dir  = "../src/demos/14_ts_quad/";
    m_quad_tessellation_shader = std::make_shared<RGL::Shader>(dir + "ts_quad.vert", dir + "ts_quad.frag", dir + "ts_quad.geom", dir + "ts_quad.tcs", dir + "ts_quad.tes");
    m_quad_tessellation_shader->link();

    /* Create VAO and VBO for curve points in 2D - NDC */
    std::vector<glm::vec2> curve_points = { glm::vec2(-1.0, -1.0),
                                            glm::vec2( 1.0,  -1.0),
                                            glm::vec2( 1.0,  1.0),
                                            glm::vec2(-1.0,  1.0) };

    glCreateVertexArrays(1, &m_quad_points_vao_id);

    glCreateBuffers(1, &m_quad_points_vbo_id);
    glNamedBufferStorage(m_quad_points_vbo_id, curve_points.size() * sizeof(curve_points[0]), curve_points.data(), 0 /*flags*/);

    /* Set up VAO */
    glEnableVertexArrayAttrib(m_quad_points_vao_id, 0 /*index*/);

    /* Separate attribute format */
    glVertexArrayAttribFormat (m_quad_points_vao_id, 0 /*index*/, 2 /*size*/, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
    glVertexArrayAttribBinding(m_quad_points_vao_id, 0 /*index*/, 0 /*bindingindex*/);
    glVertexArrayVertexBuffer (m_quad_points_vao_id, 0 /*bindingindex*/, m_quad_points_vbo_id, 0 /*offset*/, sizeof(curve_points[0]) /*stride*/);
}

void Tessellation2D::input()
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
        std::string filename = "14_ts_quad";
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

void Tessellation2D::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void Tessellation2D::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(m_quad_points_vao_id);
    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Draw curve */
    m_quad_tessellation_shader->bind();
    m_quad_tessellation_shader->setUniform("outer", m_outer);
    m_quad_tessellation_shader->setUniform("inner", m_inner);
    m_quad_tessellation_shader->setUniform("quad_color", m_quad_color);
    m_quad_tessellation_shader->setUniform("line_color", m_line_color);
    m_quad_tessellation_shader->setUniform("line_width", m_line_width * 0.5f);
    m_quad_tessellation_shader->setUniform("mvp", view_projection);
    m_quad_tessellation_shader->setUniform("viewport_matrix", RGL::Window::getViewportMatrix());

    glDrawArrays(GL_PATCHES, 0, m_no_quad_points);
}

void Tessellation2D::render_gui()
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
        ImGui::ColorEdit4("Quad color", &m_quad_color[0]);
        ImGui::ColorEdit4("Line color", &m_line_color[0]);
        ImGui::SliderFloat("Line width", &m_line_width, 0.0f, 10.0f, "%.1f");
        ImGui::SliderInt("Outer", &m_outer, 1, 32);
        ImGui::SliderInt("Inner", &m_inner, 2, 32);
        ImGui::PopItemWidth();
        ImGui::Spacing();
    }
    ImGui::End();
}
