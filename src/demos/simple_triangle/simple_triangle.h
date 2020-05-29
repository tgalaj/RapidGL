#pragma once
#include <memory>

#include "core_app.h"
#include "shader.h"

class SimpleTriangle : public RapidGL::CoreApp
{
public:
    SimpleTriangle();
    ~SimpleTriangle();

    virtual void init_app()                override;
    virtual void input()                   override;
    virtual void update(double delta_time) override;
    virtual void render()                  override;

private:
    GLuint m_vao_id;
    GLuint m_vbo_id;

    glm::vec3 m_triangle_color;
    std::shared_ptr<RapidGL::Shader> m_shader;
};