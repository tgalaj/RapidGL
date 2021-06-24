#pragma once
#include "core_app.h"

#include "camera.h"
#include "shader.h"

#include <memory>

class Tessellation2D : public RGL::CoreApp
{
public:
    Tessellation2D();
    ~Tessellation2D();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_quad_tessellation_shader;

    GLuint m_no_quad_points;
    GLuint m_quad_points_vao_id;
    GLuint m_quad_points_vbo_id;

    glm::vec3 m_quad_color;
    glm::vec3 m_line_color;
    float m_line_width;
    int m_outer;
    int m_inner;
};