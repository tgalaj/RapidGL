#pragma once
#include "core_app.h"

#include "camera.h"
#include "model.h"
#include "shader.h"
#include "gui/gui.h"

#include <memory>
#include <imfilebrowser.h>

class SimpleParticlesSystem : public RGL::CoreApp
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

    void reset_particles_buffers();

    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_simple_shader, m_particles_shader;
    std::shared_ptr<RGL::Model> m_grid_model;

    GLuint m_tfo_ids[2]; // Transform Feedback Objects
    GLuint m_pos_vbo_ids[2];
    GLuint m_velocity_vbo_ids[2];
    GLuint m_age_vbo_ids[2];
    GLuint m_vao_ids[2];
    GLuint m_random_texture_1d;
    GLuint m_draw_buffer_idx;

    glm::vec3 m_emitter_pos, m_emitter_dir;
    glm::vec3 m_acceleration;
    int m_no_particles;
    float m_particle_lifetime;
    glm::vec2 m_particle_size_min_max;
    float m_particle_angle;
    float m_delta_time;
    GLuint m_particle_texture;
    bool m_should_fade_out_with_time;
    glm::vec2 m_start_position_min_max;
    glm::vec2 m_start_velocity_min_max;
    glm::vec3 m_direction_constraints;
    float m_cone_angle;

    glm::vec3 m_ambient_color;
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */

    ImGui::FileBrowser m_file_dialog;
    std::string m_current_texture_filename;
};