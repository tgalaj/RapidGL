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

class SimpleParticlesSystem : public RapidGL::CoreApp
{
public:
    SimpleParticlesSystem();
    ~SimpleParticlesSystem();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    /**
     * Return a rotation matrix that rotates the y-axis to be parallel to dir.
     *
     * @param dir
     * @return
     */
    glm::mat3 make_arbitrary_basis(const glm::vec3& dir)
    {
        glm::mat3 basis;
        glm::vec3 u, v, n;
        v = dir;
        n = glm::cross(glm::vec3(1, 0, 0), v);
        if (glm::length(n) < 0.00001f) {
            n = glm::cross(glm::vec3(0, 1, 0), v);
        }
        u = glm::cross(v, n);
        basis[0] = glm::normalize(u);
        basis[1] = glm::normalize(v);
        basis[2] = glm::normalize(n);
        return basis;
    }

    std::shared_ptr<RapidGL::Camera> m_camera;
    std::shared_ptr<RapidGL::Shader> m_particles_shader;

    DirectionalLight m_dir_light_properties;

    GLuint m_tfo_ids[2]; // Transform Feedback Objects
    GLuint m_pos_vbo_ids[2];
    GLuint m_velocity_vbo_ids[2];
    GLuint m_age_vbo_ids[2];
    GLuint m_vao_ids[2];
    GLuint m_random_texture_1d;
    GLuint m_draw_buffer_idx;

    glm::vec3 m_emitter_pos, m_emitter_dir;
    glm::vec3 m_acceleration;
    uint32_t m_no_particles;
    float m_particle_lifetime;
    float m_particle_size;
    float m_delta_time;
    GLuint m_particle_texture;

    glm::vec3 m_ambient_color;
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */
};