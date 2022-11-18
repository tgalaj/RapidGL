#include "ts_curve.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/random.hpp>

Tessellation1D::Tessellation1D()
    : m_curve_points_vao_id(0),
      m_curve_points_vbo_id(0),
      m_no_curve_points(4),
      m_points_color(1.0, 0.0, 0.0),
      m_line_color(1.0, 1.0, 0.0),
      m_no_segments(50),
      m_no_strips(1)
{
}

Tessellation1D::~Tessellation1D()
{
    if(m_curve_points_vao_id != 0)
    {
        glDeleteVertexArrays(1, &m_curve_points_vao_id);
    }

    if(m_curve_points_vbo_id != 0)
    {
        glDeleteBuffers(1, &m_curve_points_vbo_id);
    }
}

void Tessellation1D::init_app()
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
    m_camera = std::make_shared<RGL::Camera>(-0.5f * c, 0.5f * c, -0.3f * c, 0.45f * c, 0.1f, 100.0f);
    m_camera->setPosition(0.0, 0.0, 1.5);

    /* Create shader. */
    std::string dir  = "../src/demos/13_ts_curve/";
    m_curve_tessellation_shader = std::make_shared<RGL::Shader>(dir + "ts_curve.vert", dir + "ts_curve.frag", dir + "ts_curve.tcs", dir + "ts_curve.tes");
    m_curve_tessellation_shader->link();

    m_solid_points_color_shader = std::make_shared<RGL::Shader>(dir + "solid.vert", dir + "solid.frag");
    m_solid_points_color_shader->link();

    /* Create VAO and VBO for curve points in 2D - NDC */
    std::vector<glm::vec2> curve_points = { glm::vec2(-1.0, -1.0),
                                            glm::vec2(-0.5,  1.0),
                                            glm::vec2( 0.5, -1.0),
                                            glm::vec2( 1.0,  1.0) };

    glCreateVertexArrays(1, &m_curve_points_vao_id);

    glCreateBuffers(1, &m_curve_points_vbo_id);
    glNamedBufferStorage(m_curve_points_vbo_id, curve_points.size() * sizeof(curve_points[0]), curve_points.data(), 0 /*flags*/);

    /* Set up VAO */
    glEnableVertexArrayAttrib(m_curve_points_vao_id, 0 /*index*/);

    /* Separate attribute format */
    glVertexArrayAttribFormat (m_curve_points_vao_id, 0 /*index*/, 2 /*size*/, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
    glVertexArrayAttribBinding(m_curve_points_vao_id, 0 /*index*/, 0 /*bindingindex*/);
    glVertexArrayVertexBuffer (m_curve_points_vao_id, 0 /*bindingindex*/, m_curve_points_vbo_id, 0 /*offset*/, sizeof(curve_points[0]) /*stride*/);
}

void Tessellation1D::input()
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
        std::string filename = "13_ts_curve";
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

void Tessellation1D::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void Tessellation1D::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(m_curve_points_vao_id);
    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* Draw curve */
    m_curve_tessellation_shader->bind();
    m_curve_tessellation_shader->setUniform("num_segments", m_no_segments);
    m_curve_tessellation_shader->setUniform("num_strips", m_no_strips);
    m_curve_tessellation_shader->setUniform("line_color", m_line_color);
    m_curve_tessellation_shader->setUniform("mvp", view_projection);
    glDrawArrays(GL_PATCHES, 0, m_no_curve_points);

    /* Draw control points */
    m_solid_points_color_shader->bind();
    m_solid_points_color_shader->setUniform("view_projection", view_projection);
    m_solid_points_color_shader->setUniform("point_color", m_points_color);

    glDrawArrays(GL_POINTS, 0, m_no_curve_points);
}

void Tessellation1D::render_gui()
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
        ImGui::ColorEdit4("Points color", &m_points_color[0]);
        ImGui::ColorEdit4("Line color",   &m_line_color[0]);
        ImGui::SliderInt("No. segments",  &m_no_segments, 1, 50);
        ImGui::PopItemWidth();
        ImGui::Spacing();
    }
    ImGui::End();
}
