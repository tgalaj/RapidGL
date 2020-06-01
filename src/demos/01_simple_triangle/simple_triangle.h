#pragma once
#include <memory>

#include "core_app.h"
#include "shader.h"

class TemplateProject : public RapidGL::CoreApp
{
public:
    TemplateProject();
    ~TemplateProject();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void renderGUI()               override;

private:
    GLuint m_vao_id;
    GLuint m_vbo_id;

    glm::vec3 m_triangle_color;
    glm::vec2 m_triangle_translation;
    std::shared_ptr<RapidGL::Shader> m_shader;
};