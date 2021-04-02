#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"
#include "skybox.hpp"

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

    void setDirection(const glm::vec2 & azimuth_elevation_angles)
    {
        float az = glm::radians(azimuth_elevation_angles.x);
        float el = glm::radians(azimuth_elevation_angles.y);

        direction.x = glm::sin(el) * glm::cos(az);
        direction.y = glm::cos(el);
        direction.z = glm::sin(el) * glm::sin(az);

        direction = glm::normalize(-direction);
    }
};

class EnvironmentMapping : public RapidGL::CoreApp
{
public:
    EnvironmentMapping();
    ~EnvironmentMapping();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    std::shared_ptr<RapidGL::Camera> m_camera;
    std::shared_ptr<RapidGL::Shader> m_directional_light_shader;

    std::vector<std::shared_ptr<RapidGL::Model>> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;
    std::vector<glm::vec3> m_color_tints;

    std::shared_ptr<RapidGL::Model> m_xyzrgb_dragon;
    std::shared_ptr<RapidGL::Model> m_lucy;
    std::shared_ptr<RapidGL::Model> m_ground_plane;

    /* Environment mapping */
    std::shared_ptr<Skybox> m_skybox;

    std::string m_current_skybox_name;
    std::string m_skybox_names_list[3] = { "calm_sea", "distant_sunset", "heaven" };
    
    /* Light properties */
    DirectionalLight m_dir_light_properties;
    
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */

    float m_ambient_factor;
    float m_gamma;
    float m_alpha_cutout_threshold;
};