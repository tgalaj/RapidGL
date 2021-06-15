#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"
#include "gui/gui.h"

#include <memory>

class InstancedParticlesCS : public RGL::CoreApp
{
public:
    InstancedParticlesCS();
    ~InstancedParticlesCS();

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

    void reset_particles_buffers();

    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_simple_shader, m_particles_render_shader, m_particles_compute_shader;
    std::shared_ptr<RGL::Model> m_instanced_model, m_grid_model;

    GLuint m_pos_vbo_id;
    GLuint m_velocity_vbo_id;
    GLuint m_age_vbo_id;
    GLuint m_rotation_vbo_id;

    glm::vec3 m_emitter_pos, m_emitter_dir;
    glm::vec3 m_acceleration;
    float m_particle_lifetime;
    float m_delta_time;
    glm::vec2 m_start_position_min_max;
    glm::vec2 m_start_velocity_min_max;
    glm::vec2 m_start_rotational_velocity_min_max;
    glm::vec3 m_direction_constraints;
    glm::vec3 m_particles_color;
    float m_cone_angle;
    GLuint m_total_particles;
};