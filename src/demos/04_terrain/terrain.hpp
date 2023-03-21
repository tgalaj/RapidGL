#pragma once
#include "core_app.h"

#include "camera.h"
#include "static_model.h"
#include "shader.h"

#include <memory>
#include <vector>

#include "terrain_model.hpp"

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

class Terrain : public RGL::CoreApp
{
public:
    Terrain();
    ~Terrain();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    void render_terrain(const glm::mat4 & view_projection);

    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_ambient_light_shader;
    std::shared_ptr<RGL::Shader> m_directional_light_shader;
    std::shared_ptr<RGL::Shader> m_point_light_shader;
    std::shared_ptr<RGL::Shader> m_spot_light_shader;

    std::shared_ptr<TerrainModel> m_terrain_model;
    glm::mat4 m_terrain_model_matrix;
    glm::vec3 m_terrain_position;
    float m_terrain_size;
    float m_terrain_max_height;
    float m_texcoord_tiling_factor;
    float m_grass_slope_threshold;
    float m_slope_rock_threshold;

    std::vector<std::string>                     m_terrain_textures_filenames;
    std::vector<std::string>                     m_terrain_heightmaps_filenames;
    std::vector<std::shared_ptr<RGL::Texture2D>> m_terrain_textures;

    std::shared_ptr<RGL::Shader> m_terrain_ambient_light_shader;
    std::shared_ptr<RGL::Shader> m_terrain_directional_light_shader;
    std::shared_ptr<RGL::Shader> m_terrain_point_light_shader;
    std::shared_ptr<RGL::Shader> m_terrain_spot_light_shader;

    std::vector<RGL::StaticModel> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;

    DirectionalLight m_dir_light_properties;
    PointLight       m_point_light_properties;
    SpotLight        m_spot_light_properties;
    
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */
    glm::vec2 m_spot_light_angles;  /* azimuth and elevation angles */

    float m_ambient_factor;
    float m_gamma;

    bool m_snap_camera_to_ground;
};