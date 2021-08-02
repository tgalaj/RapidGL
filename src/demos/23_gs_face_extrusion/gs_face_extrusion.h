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
        glTextureParameteri(m_tex_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_tex_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    void render(float exposure, float gamma)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->bind();
        m_shader->setUniform("u_exposure", exposure);
        m_shader->setUniform("u_gamma",    gamma);
        bindTexture();

        glBindVertexArray(m_dummy_vao_id);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
};

class GSFaceExtrusion : public RGL::CoreApp
{
public:
    GSFaceExtrusion();
    ~GSFaceExtrusion();

    void init_app()                override;
    void input()                   override;
    void update(double delta_time) override;
    void render()                  override;
    void render_gui()              override;

private:
    struct Texture2DRenderTarget
    {
        GLuint m_texture_id = 0;
        GLuint m_fbo_id = 0;
        GLuint m_rbo_id = 0;
        GLuint m_width = 0, m_height = 0;

        ~Texture2DRenderTarget() { cleanup(); }

        void bindTexture(GLuint unit = 0)
        {
            glBindTextureUnit(unit, m_texture_id);
        }

        void bindRenderTarget()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
            glViewport(0, 0, m_width, m_height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        void cleanup()
        {
            if (m_texture_id != 0)
            {
                glDeleteTextures(1, &m_texture_id);
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

        void generate_rt(uint32_t width, uint32_t height)
        {
            m_width = width;
            m_height = height;

            glGenTextures(1, &m_texture_id);
            glBindTexture(GL_TEXTURE_2D, m_texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, m_width, m_height, 0, GL_RG, GL_FLOAT, 0);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenFramebuffers(1, &m_fbo_id);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0);

            glGenRenderbuffers(1, &m_rbo_id);
            glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo_id);

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    };

    struct CubeMapRenderTarget
    {
        glm::mat4 m_view_transforms[6];
        glm::mat4 m_projection;

        GLuint    m_cubemap_texture_id = 0;
        GLuint    m_fbo_id             = 0;
        GLuint    m_rbo_id             = 0;
        glm::vec3 m_position           = glm::vec3(0.0f);
        GLuint m_width, m_height;

        ~CubeMapRenderTarget() { cleanup(); }

        void set_position(const glm::vec3 pos)
        {
            m_position = pos;
            m_view_transforms[0] = glm::lookAt(pos, pos + glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0));
            m_view_transforms[1] = glm::lookAt(pos, pos + glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0));
            m_view_transforms[2] = glm::lookAt(pos, pos + glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1));
            m_view_transforms[3] = glm::lookAt(pos, pos + glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1));
            m_view_transforms[4] = glm::lookAt(pos, pos + glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0));
            m_view_transforms[5] = glm::lookAt(pos, pos + glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0));

            m_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
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

        void generate_rt(uint32_t width, uint32_t height, bool gen_mip_levels = false)
        {
            m_width  = width;
            m_height = height;

            glGenTextures(1, &m_cubemap_texture_id);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap_texture_id);

            for (uint8_t i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, 0);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, gen_mip_levels ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            if (gen_mip_levels) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

            glGenFramebuffers(1, &m_fbo_id);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_cubemap_texture_id, 0);

            glGenRenderbuffers(1, &m_rbo_id);
            glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo_id);

            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }; 

    void HdrEquirectangularToCubemap(const std::shared_ptr<CubeMapRenderTarget> & cubemap_rt, const std::shared_ptr<RGL::Texture2D> & m_equirectangular_map);
    void IrradianceConvolution      (const std::shared_ptr<CubeMapRenderTarget> & cubemap_rt);
    void PrefilterCubemap           (const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt);
    void PrecomputeIndirectLight    (const std::filesystem::path & hdri_map_filepath);
    void PrecomputeBRDF             (const std::shared_ptr<Texture2DRenderTarget>& rt);
    void GenSkyboxGeometry();

    std::shared_ptr<CubeMapRenderTarget> m_env_cubemap_rt;
    std::shared_ptr<CubeMapRenderTarget> m_irradiance_cubemap_rt;
    std::shared_ptr<CubeMapRenderTarget> m_prefiltered_env_map_rt;
    std::shared_ptr<Texture2DRenderTarget> m_brdf_lut_rt;

    std::shared_ptr<RGL::Shader> m_equirectangular_to_cubemap_shader;
    std::shared_ptr<RGL::Shader> m_irradiance_convolution_shader;
    std::shared_ptr<RGL::Shader> m_prefilter_env_map_shader;
    std::shared_ptr<RGL::Shader> m_precompute_brdf;
    std::shared_ptr<RGL::Shader> m_background_shader;

    std::shared_ptr<RGL::Camera> m_camera;
    std::shared_ptr<RGL::Shader> m_ambient_light_shader;
    std::shared_ptr<RGL::Shader> m_directional_light_shader;
    std::shared_ptr<RGL::Shader> m_point_light_shader;
    std::shared_ptr<RGL::Shader> m_spot_light_shader;

    RGL::StaticModel m_static_model;
    glm::mat4        m_static_model_transform;

    glm::vec3 m_albedo;
    float m_roughness;
    float m_metallic;
    float m_ao;

    float m_current_time;
    float m_animation_speed;
    float m_extrusion_amount;

    DirectionalLight m_dir_light_properties;
    
    glm::vec2 m_dir_light_angles;   /* azimuth and elevation angles */
    glm::vec2 m_spot_light_angles;  /* azimuth and elevation angles */

    std::shared_ptr<PostprocessFilter> m_tmo_ps;
    float m_exposure; 
    float m_gamma;

    float m_background_lod_level;
    std::string m_hdr_maps_names[2] = { "phalzer_forest_01_4k.hdr", "sunset_fairway_4k.hdr" };
    uint8_t m_current_hdr_map_idx   = 0;

    GLuint m_skybox_vao, m_skybox_vbo;
};