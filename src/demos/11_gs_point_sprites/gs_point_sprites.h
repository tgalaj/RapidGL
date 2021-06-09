#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"

#include <memory>

class GSPointSprites : public RGL::CoreApp
{
public:
    GSPointSprites();
    ~GSPointSprites();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_point_sprites_shader;
    std::shared_ptr<RGL::Texture> m_sprite_tex;

    float m_half_quad_width;
    uint32_t m_no_sprites;
    GLuint m_sprites_vao_id;
    GLuint m_sprites_vbo_id;
};