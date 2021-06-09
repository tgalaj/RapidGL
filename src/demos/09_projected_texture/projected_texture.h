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

struct Attenuation
{
    float constant;
    float linear;
    float quadratic;
};

struct PointLight : BaseLight
{
    Attenuation attenuation;
    glm::vec3 position;
    float range;
};

struct SpotLight : PointLight
{
    glm::vec3 direction;
    float cutoff;

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

struct Projector
{
    glm::mat4 m_view_matrix;
    glm::mat4 m_projection_matrix;
    glm::mat4 m_bias_matrix;
    RGL::Texture m_texture;

    glm::mat4 transform() const
    {
        return m_bias_matrix * m_projection_matrix * m_view_matrix;
    }
};

class ProjectedTexture : public RGL::CoreApp
{
public:
    ProjectedTexture();
    ~ProjectedTexture();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_ambient_light_shader;
    std::shared_ptr<RGL::Shader> m_spot_light_shader;

    std::vector<std::shared_ptr<RGL::Model>> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;

    SpotLight m_spot_light_properties;
    Projector m_projector;
    float m_projector_move_speed;

    std::string m_current_projector_texture_name = "circle4a.png";
    std::vector<std::string> m_projector_textures_names_list = { "circle4a.png", "circle5a.png", "circle6a.png", "circle7a.png" };

    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_spot_light_angles;  /* azimuth and elevation angles */

    float m_ambient_factor;
    float m_gamma;
};