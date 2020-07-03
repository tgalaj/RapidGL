#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"

#include <memory>
#include <vector>

class Lighting : public RapidGL::CoreApp
{
public:
    Lighting();
    ~Lighting();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RapidGL::Camera> m_camera;
    std::shared_ptr<RapidGL::Shader> m_ambient_light_shader;
    std::shared_ptr<RapidGL::Shader> m_directional_light_shader;
    std::shared_ptr<RapidGL::Shader> m_point_light_shader;
    std::shared_ptr<RapidGL::Shader> m_spot_light_shader;

    std::vector<std::shared_ptr<RapidGL::Model>> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;

    float m_mix_factor;
};