#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"

#include <memory>

class Tessellation1D : public RapidGL::CoreApp
{
public:
    Tessellation1D();
    ~Tessellation1D();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RapidGL::Camera> m_camera;
    std::shared_ptr<RapidGL::Shader> m_solid_points_color_shader;
    std::shared_ptr<RapidGL::Shader> m_curve_tessellation_shader;

    GLuint m_no_curve_points;
    GLuint m_curve_points_vao_id;
    GLuint m_curve_points_vbo_id;

    glm::vec3 m_points_color;
    glm::vec3 m_line_color;
    int m_no_segments;
    int m_no_strips;
};