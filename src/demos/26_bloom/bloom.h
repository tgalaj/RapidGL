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

struct PointLight : BaseLight
{
    glm::vec3 position;
    float radius;
};

struct Material
{
    glm::vec3 m_albedo   = glm::vec3(1.0f);
    glm::vec3 m_emission = glm::vec3(0.0f);
    float m_ao           = 1.0f;
    float m_roughness    = 0.0f;
    float m_metallic     = 0.0f;

    bool m_has_albedo_map    = false;
    bool m_has_normal_map    = false;
    bool m_has_metallic_map  = false;
    bool m_has_roughness_map = false;
    bool m_has_ao_map        = false;
};

struct StaticObject
{
    StaticObject(const std::shared_ptr<RGL::StaticModel> & model, 
                 const glm::mat4                         & transform) 
        : m_model    (model), 
          m_transform(transform) {}

    StaticObject(const std::shared_ptr<RGL::StaticModel> & model, 
                 const glm::mat4                         & transform, 
                 const Material                          & material) 
        : m_model    (model), 
          m_transform(transform),
          m_material (material) {}

    Material m_material;
    glm::mat4 m_transform;
    std::shared_ptr<RGL::StaticModel> m_model;
};

class Bloom : public RGL::CoreApp
{
public:
    Bloom();
    ~Bloom();

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
        GLenum m_internalformat;

        const uint8_t m_downscale_limit = 10;
        const uint8_t m_max_iterations = 16; // max mipmap levels
        uint8_t m_mip_levels = 1;

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

        void bindImageForRead(GLuint image_unit, GLuint mip_level)
        {
            glBindImageTexture(image_unit, m_texture_id, mip_level, GL_FALSE, 0, GL_READ_ONLY, m_internalformat);
        }

        void bindImageForWrite(GLuint image_unit, GLuint mip_level)
        {
            glBindImageTexture(image_unit, m_texture_id, mip_level, GL_FALSE, 0, GL_WRITE_ONLY, m_internalformat);
        }

        void bindImageForReadWrite(GLuint image_unit, GLuint mip_level)
        {
            glBindImageTexture(image_unit, m_texture_id, mip_level, GL_FALSE, 0, GL_READ_WRITE, m_internalformat);
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

        void create(uint32_t width, uint32_t height, GLint internalformat)
        {
            m_width          = width;
            m_height         = height;
            m_internalformat = internalformat;

            m_mip_levels = calculateMipmapLevels();

            glCreateFramebuffers(1, &m_fbo_id);

            glCreateTextures(GL_TEXTURE_2D, 1, &m_texture_id);
            glTextureStorage2D(m_texture_id, m_mip_levels, internalformat, width, height); // internalformat = GL_RGB32F

            glTextureParameteri(m_texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(m_texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_texture_id, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_texture_id, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

            glCreateRenderbuffers(1, &m_rbo_id);
            glNamedRenderbufferStorage(m_rbo_id, GL_DEPTH24_STENCIL8, width, height);

            glNamedFramebufferTexture(m_fbo_id, GL_COLOR_ATTACHMENT0, m_texture_id, 0);
            glNamedFramebufferRenderbuffer(m_fbo_id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_id);
        }

        uint8_t calculateMipmapLevels()
        {
            uint32_t width      = m_width  / 2;
            uint32_t height     = m_height / 2;
            uint8_t  mip_levels = 1;

            printf("Mip level %d: %d x %d\n", 0, m_width, m_height);
            printf("Mip level %d: %d x %d\n", mip_levels, width, height);

            for (uint8_t i = 0; i < m_max_iterations; ++i)
            {
                width  = width  / 2;
                height = height / 2;

                if (width < m_downscale_limit || height < m_downscale_limit) break;

                ++mip_levels;

                printf("Mip level %d: %d x %d\n", mip_levels, width, height);
            }

            return mip_levels + 1;
        }
    };

    struct PostprocessFilter
    {
        std::shared_ptr<RGL::Shader> m_shader;
        std::shared_ptr<Texture2DRenderTarget> rt;

        GLuint m_dummy_vao_id;

        PostprocessFilter(uint32_t width, uint32_t height)
        {
            m_shader = std::make_shared<RGL::Shader>("src/demos/10_postprocessing_filters/FSQ.vert", "src/demos/22_pbr/tmo.frag");
            m_shader->link();

            rt = std::make_shared<Texture2DRenderTarget>();
            rt->create(width, height, GL_RGBA32F);
            glTextureParameteri(rt->m_texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTextureParameteri(rt->m_texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(rt->m_texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glCreateVertexArrays(1, &m_dummy_vao_id);
        }

        ~PostprocessFilter()
        {
            if (m_dummy_vao_id != 0)
            {
                glDeleteVertexArrays(1, &m_dummy_vao_id);
            }
        }

        void bindTexture(GLuint unit = 0)
        {
            rt->bindTexture(unit);
        }

        void bindFilterFBO()
        {
            rt->bindRenderTarget();
        }

        void render(float exposure, float gamma)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_shader->bind();
            m_shader->setUniform("u_exposure", exposure);
            m_shader->setUniform("u_gamma", gamma);
            bindTexture();

            glBindVertexArray(m_dummy_vao_id);
            glDrawArrays(GL_TRIANGLES, 0, 3);
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

    void RenderScene();

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
    std::shared_ptr<RGL::Shader> m_point_light_shader;

    /* Bloom members */
    std::shared_ptr<RGL::Shader> m_downscale_shader;
    std::shared_ptr<RGL::Shader> m_upscale_shader;
    std::shared_ptr<RGL::Texture2D> m_bloom_dirt_texture;

    float m_threshold;
    float m_knee;
    float m_bloom_intensity;
    float m_bloom_dirt_intensity;
    bool  m_bloom_enabled;
    /* End bloom members */

    std::vector<StaticObject> m_static_objects;

    PointLight m_point_lights_properties[4];
    StaticObject* m_light_boxes[4];

    std::shared_ptr<PostprocessFilter> m_tmo_ps;
    float m_exposure; 
    float m_gamma;

    float m_background_lod_level;
    std::string m_hdr_maps_names[3] = { "colorful_studio_4k.hdr", "phalzer_forest_01_4k.hdr", "sunset_fairway_4k.hdr" };
    uint8_t m_current_hdr_map_idx   = 2;

    GLuint m_skybox_vao, m_skybox_vbo;
};