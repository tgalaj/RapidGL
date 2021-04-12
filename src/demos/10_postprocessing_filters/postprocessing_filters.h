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

struct PostprocessFilter
{
    std::shared_ptr<RapidGL::Shader> m_shader;

    GLuint m_fbo_id;
    GLuint m_rbo_id;
    GLuint m_tex_id;
    GLuint m_dummy_vao_id;

    PostprocessFilter(uint32_t width, uint32_t height)
    {
        glGenFramebuffers(1, &m_fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);

        glGenTextures(1, &m_tex_id);
        glBindTexture(GL_TEXTURE_2D, m_tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenRenderbuffers(1, &m_rbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_id, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_id);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_shader = std::make_shared<RapidGL::Shader>("../src/demos/10_postprocessing_filters/FSQ.vert", "../src/demos/10_postprocessing_filters/PS_filters.frag");
        m_shader->link();

        glGenVertexArrays(1, &m_dummy_vao_id);
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

    void render(const std::string filter_name)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->bind();
        m_shader->setSubroutine(RapidGL::Shader::ShaderType::FRAGMENT, filter_name);
        bindTexture();

        glBindVertexArray(m_dummy_vao_id);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
};

class Lighting : public RapidGL::CoreApp
{
public:
    Lighting();
    ~Lighting();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()               override;

private:
    std::shared_ptr<RapidGL::Camera> m_camera;
    std::shared_ptr<RapidGL::Shader> m_ambient_light_shader;
    std::shared_ptr<RapidGL::Shader> m_directional_light_shader;

    std::shared_ptr<PostprocessFilter> m_postprocess_filter;
    std::string m_current_ps_filter_name;
    std::string m_ps_filter_names_list[4] = { "no_filter", "negative", "edge_detection", "gaussian_blur" };

    std::vector<std::shared_ptr<RapidGL::Model>> m_objects;
    std::vector<glm::mat4> m_objects_model_matrices;

    DirectionalLight m_dir_light_properties;
    
    glm::vec3 m_specular_power;     /* specular powers for directional, point and spot lights respectively */
    glm::vec3 m_specular_intenstiy; /* specular intensities for directional, point and spot lights respectively */
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */

    float m_ambient_factor;
};