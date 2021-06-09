#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"

#include <memory>

struct BaseLight
{
    glm::vec3 color;
    float intensity;
};

struct DirectionalLight : BaseLight
{
    glm::vec3 direction;

    void setDirection(float azimuth, float elevation)
    {
        float az = glm::radians(azimuth);
        float el = glm::radians(elevation);

        direction.x = glm::sin(el) * glm::cos(az);
        direction.y = glm::cos(el);
        direction.z = glm::sin(el) * glm::sin(az);

        direction = glm::normalize(-direction);
    }
};

class VertexDisplacement : public RGL::CoreApp
{
public:
    VertexDisplacement();
    ~VertexDisplacement();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_vs_disp_shader;
    std::shared_ptr<RGL::Model> m_model;
    glm::mat4 m_world_matrix;

    DirectionalLight m_dir_light_properties;

    glm::vec3 m_ambient_color;
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */

    float m_time;
    float m_amplitude;
    float m_velocity;
    float m_frequency;
};