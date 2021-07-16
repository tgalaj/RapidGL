#pragma once
#include "core_app.h"

#include "camera.h"
#include "static_model.h"
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

struct PointLight : BaseLight
{
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

struct PostprocessFilter
{
    std::shared_ptr<RGL::Shader> m_shader;

    GLuint m_fbo_id;
    GLuint m_rbo_id;
    GLuint m_tex_id;
    GLuint m_dummy_vao_id;

    PostprocessFilter(uint32_t width, uint32_t height)
    {
        glCreateFramebuffers(1, &m_fbo_id);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_tex_id);
        glTextureStorage2D(m_tex_id, 1, GL_RGB32F, width, height);
        glTextureParameteri(m_tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glCreateRenderbuffers(1, &m_rbo_id);
        glNamedRenderbufferStorage(m_rbo_id, GL_DEPTH24_STENCIL8, width, height);

        glNamedFramebufferTexture(m_fbo_id, GL_COLOR_ATTACHMENT0, m_tex_id, 0);
        glNamedFramebufferRenderbuffer(m_fbo_id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_id);

        m_shader = std::make_shared<RGL::Shader>("../src/demos/10_postprocessing_filters/FSQ.vert", "../src/demos/22_pbr/tmo.frag");
        m_shader->link();

        glCreateVertexArrays(1, &m_dummy_vao_id);
    }

    ~PostprocessFilter()
    {
        if(m_fbo_id != 0)
        {
            glDeleteFramebuffers(1, &m_fbo_id);
        }

        if(m_rbo_id != 0)
        {
            glDeleteRenderbuffers(1, &m_rbo_id);
        }

        if(m_tex_id != 0)
        {
            glDeleteTextures(1, &m_tex_id);
        }

        if (m_dummy_vao_id != 0)
        {
            glDeleteVertexArrays(1, &m_dummy_vao_id);
        }
    }

    void bindTexture(GLuint unit = 0)
    {
        glBindTextureUnit(unit, m_tex_id);
    }

    void bindFilterFBO()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void render(float exposure, float gamma, float a, float d, float hdr_max, float mid_in, float mid_out)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->bind();
        m_shader->setUniform("u_exposure", exposure);
        m_shader->setUniform("u_gamma",    gamma);
        m_shader->setUniform("u_a",        a);
        m_shader->setUniform("u_d",        d);
        m_shader->setUniform("u_hdr_max",  hdr_max);
        m_shader->setUniform("u_mid_in",   mid_in);
        m_shader->setUniform("u_mid_out",  mid_out);
        bindTexture();

        glBindVertexArray(m_dummy_vao_id);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
};

class PBR : public RGL::CoreApp
{
public:
    PBR();
    ~PBR();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_ambient_light_shader;
    std::shared_ptr<RGL::Shader> m_directional_light_shader;
    std::shared_ptr<RGL::Shader> m_point_light_shader;
    std::shared_ptr<RGL::Shader> m_spot_light_shader;

    RGL::StaticModel m_sphere_model;
    std::vector<glm::mat4> m_objects_model_matrices;

    DirectionalLight m_dir_light_properties;
    PointLight       m_point_light_properties[4];
    SpotLight        m_spot_light_properties;
    
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */
    glm::vec2 m_spot_light_angles;  /* azimuth and elevation angles */

    std::shared_ptr<PostprocessFilter> m_tmo_ps;
    float m_exposure; 
    float m_gamma; 
    float m_a;
    float m_d;
    float m_hdr_max;
    float m_mid_in;
    float m_mid_out;
};