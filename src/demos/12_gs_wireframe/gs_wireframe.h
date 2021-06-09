#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"

#include <memory>
#include <vector>

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

class GSWireframe : public RGL::CoreApp
{
public:
    GSWireframe();
    ~GSWireframe();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_directional_light_shader;

    std::vector<std::shared_ptr<RGL::Model>> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;

    DirectionalLight m_dir_light_properties;

    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */

    glm::vec4 m_line_color;
    float m_line_width;

    glm::vec3 m_ambient_color;
};