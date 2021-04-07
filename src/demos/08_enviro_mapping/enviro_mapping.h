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
    struct CubeMapRenderTarget
    {
        glm::mat4 m_view_transforms[6];

        GLuint    m_cubemap_texture_id = 0;
        GLuint    m_fbo_id             = 0;
        GLuint    m_rbo_id             = 0;
        glm::vec3 m_position           = glm::vec3(0.0f);

        void set_position(const glm::vec3 pos)
        {
            m_position = pos;
            m_view_transforms[0] = glm::lookAt(pos, pos + glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0));
            m_view_transforms[1] = glm::lookAt(pos, pos + glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0));
            m_view_transforms[2] = glm::lookAt(pos, pos + glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1));
            m_view_transforms[3] = glm::lookAt(pos, pos + glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1));
            m_view_transforms[4] = glm::lookAt(pos, pos + glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0));
            m_view_transforms[5] = glm::lookAt(pos, pos + glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0));
        }

        void bindTexture(GLuint unit = 0)
        {
            glBindTextureUnit(unit, m_cubemap_texture_id);
        }

        void cleanup()
        {
            if (m_cubemap_texture_id != 0)
            {
                glDeleteTextures(1, &m_cubemap_texture_id);
            }

            if (m_fbo_id != 0)
            {
                glDeleteFramebuffers(1, &m_fbo_id);
            }

            if (m_rbo_id != 0)
            {
                glDeleteRenderbuffers(1, &m_rbo_id);
            }
        }

    } m_cubemap_rts[2];

    void                render_objects(const glm::mat4& camera_view, const glm::mat4& camera_projection, const glm::vec3 & camera_position);
    CubeMapRenderTarget generate_cubemap_rt() const;
    void                render_to_cubemap_rt(CubeMapRenderTarget & rt);

    std::shared_ptr<RapidGL::Camera> m_camera;
    std::shared_ptr<RapidGL::Shader> m_directional_light_shader;

    std::vector<std::shared_ptr<RapidGL::Model>> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;
    std::vector<glm::vec3> m_color_tints;
    std::vector<glm::vec3> m_spheres_positions;
    std::vector<float> m_random_spheres_rotation_speeds;

    std::shared_ptr<RapidGL::Model> m_xyzrgb_dragon;
    std::shared_ptr<RapidGL::Model> m_lucy;
    std::shared_ptr<RapidGL::Model> m_ground_plane;

    /* Environment mapping */
    std::shared_ptr<Skybox> m_skybox;
    std::shared_ptr<RapidGL::Shader> m_enviro_mapping_shader;
    float m_ior; /* index of refraction */

    std::string m_current_skybox_name;
    std::string m_skybox_names_list[3] = { "calm_sea", "distant_sunset", "heaven" };

    /* Dynamic Environment mapping */
    glm::ivec2 m_enviro_cubemap_size;
    glm::mat4 m_enviro_projection;
    bool m_dynamic_enviro_mapping_toggle;

    /* Light properties */
    DirectionalLight m_dir_light_properties;
    
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */

    float m_ambient_factor;
    float m_gamma;
};