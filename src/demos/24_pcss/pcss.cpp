#include "pcss.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

PCSS::PCSS()
      : m_dir_light_angles         (-35.0f, 65.0f),
        m_spot_light_angles        (90.0f, -25.0f),
        m_exposure                 (0.3f),
        m_gamma                    (3.6f),
        m_background_lod_level     (1.2),
        m_skybox_vao               (0),
        m_skybox_vbo               (0),
        m_dir_shadow_map           (0),
        m_shadow_fbo               (0),
        m_dir_shadow_frustum_size  (20.0f),
        m_dir_shadow_frustum_planes(120, 250),
        m_light_radius_uv          (0.5f)
{
}

PCSS::~PCSS()
{
    if (m_skybox_vao != 0)
    {
        glDeleteVertexArrays(1, &m_skybox_vao);
        m_skybox_vao = 0;
    }

    if (m_skybox_vbo != 0)
    {
        glDeleteBuffers(1, &m_skybox_vbo);
        m_skybox_vbo = 0;
    }

    if (m_dir_shadow_map != 0)
    {
        glDeleteTextures(1, &m_dir_shadow_map);
        m_dir_shadow_map = 0;
    }

    if (m_random_angles_tex3d_id != 0)
    {
        glDeleteTextures(1, &m_random_angles_tex3d_id);
        m_random_angles_tex3d_id = 0;
    }

    if (m_shadow_fbo != 0)
    {
        glDeleteFramebuffers(1, &m_shadow_fbo);
        m_shadow_fbo = 0;
    }
}

void PCSS::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-10.3, 7.6, -5.42);
    m_camera->setOrientation(glm::quat(-0.3, -0.052, -0.931, -0.165));
   
    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 10.0f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    /* Create models. */
    m_textured_models[0].GenCube();
    m_textured_models[1].GenCube();
    m_textured_models[2].GenCone(2.0, 1.0, 64, 64);
    m_textured_models[3].GenPQTorusKnot(256, 64, 2, 3, 0.75, 0.25);
    m_textured_models[4].GenSphere(1.0, 64);

    m_textured_models_model_matrices[0] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 0.0,   0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(6.0, 0.1, 6.0));
    m_textured_models_model_matrices[1] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 2.0,   6.1)) * glm::scale(glm::mat4(1.0), glm::vec3(6.0, 2.0, 0.1));
    m_textured_models_model_matrices[2] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 2.11, -3.5)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
    m_textured_models_model_matrices[3] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 1.21,  0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
    m_textured_models_model_matrices[4] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 1.11,  3.5)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));

    m_scene_bbox = { glm::vec3(-15), glm::vec3(15) }; //Note: in a real-life app, the scene's bounding box should be computed!
    UpdateLightMatrix();

    /* Add textures to the objects. */
    auto concrete_albedo_map    = std::make_shared<RGL::Texture2D>(); concrete_albedo_map   ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_color.png"), true);
    auto concrete_normal_map    = std::make_shared<RGL::Texture2D>(); concrete_normal_map   ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_normal.png"));
    auto concrete_metallic_map  = std::make_shared<RGL::Texture2D>(); concrete_metallic_map ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_metallic.png"));
    auto concrete_roughness_map = std::make_shared<RGL::Texture2D>(); concrete_roughness_map->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_roughness.png"));
    auto concrete_ao_map        = std::make_shared<RGL::Texture2D>(); concrete_ao_map       ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_ao.png"));

    auto plastic_albedo_map    = std::make_shared<RGL::Texture2D>(); plastic_albedo_map   ->Load(RGL::FileSystem::getPath("textures/pbr/plastic008_1k/plastic008_1K_color.png"), true);
    auto plastic_normal_map    = std::make_shared<RGL::Texture2D>(); plastic_normal_map   ->Load(RGL::FileSystem::getPath("textures/pbr/plastic008_1k/plastic008_1K_normal.png"));
    auto plastic_metallic_map  = std::make_shared<RGL::Texture2D>(); plastic_metallic_map ->Load(RGL::FileSystem::getPath("textures/pbr/plastic008_1k/plastic008_1K_metallic.png"));
    auto plastic_roughness_map = std::make_shared<RGL::Texture2D>(); plastic_roughness_map->Load(RGL::FileSystem::getPath("textures/pbr/plastic008_1k/plastic008_1K_roughness.png"));
    auto plastic_ao_map        = std::make_shared<RGL::Texture2D>(); plastic_ao_map       ->Load(RGL::FileSystem::getPath("textures/pbr/plastic008_1k/plastic008_1K_ao.png"));

    auto gold_albedo_map    = std::make_shared<RGL::Texture2D>(); gold_albedo_map   ->Load(RGL::FileSystem::getPath("textures/pbr/gold-scuffed-bl/gold-scuffed_albedo.png"), true);
    auto gold_normal_map    = std::make_shared<RGL::Texture2D>(); gold_normal_map   ->Load(RGL::FileSystem::getPath("textures/pbr/gold-scuffed-bl/gold-scuffed_normal-ogl.png"));
    auto gold_metallic_map  = std::make_shared<RGL::Texture2D>(); gold_metallic_map ->Load(RGL::FileSystem::getPath("textures/pbr/gold-scuffed-bl/gold-scuffed_metallic.png"));
    auto gold_roughness_map = std::make_shared<RGL::Texture2D>(); gold_roughness_map->Load(RGL::FileSystem::getPath("textures/pbr/gold-scuffed-bl/gold-scuffed_roughness.png"));
    auto gold_ao_map        = std::make_shared<RGL::Texture2D>(); gold_ao_map       ->Load(RGL::FileSystem::getPath("textures/pbr/gold-scuffed-bl/gold-scuffed_metallic.png"));

    auto granite_albedo_map    = std::make_shared<RGL::Texture2D>(); granite_albedo_map   ->Load(RGL::FileSystem::getPath("textures/pbr/fleshy-granite1-bl/fleshy_granite1_albedo.png"), true);
    auto granite_normal_map    = std::make_shared<RGL::Texture2D>(); granite_normal_map   ->Load(RGL::FileSystem::getPath("textures/pbr/fleshy-granite1-bl/fleshy_granite1_normal-ogl.png"));
    auto granite_metallic_map  = std::make_shared<RGL::Texture2D>(); granite_metallic_map ->Load(RGL::FileSystem::getPath("textures/pbr/fleshy-granite1-bl/fleshy_granite1_metallic.png"));
    auto granite_roughness_map = std::make_shared<RGL::Texture2D>(); granite_roughness_map->Load(RGL::FileSystem::getPath("textures/pbr/fleshy-granite1-bl/fleshy_granite1_roughness.png"));
    auto granite_ao_map        = std::make_shared<RGL::Texture2D>(); granite_ao_map       ->Load(RGL::FileSystem::getPath("textures/pbr/fleshy-granite1-bl/fleshy_granite1_ao.png"));

    m_textured_models[0].AddTexture(concrete_albedo_map,    0);
    m_textured_models[0].AddTexture(concrete_normal_map,    1);
    m_textured_models[0].AddTexture(concrete_metallic_map,  2);
    m_textured_models[0].AddTexture(concrete_roughness_map, 3);
    m_textured_models[0].AddTexture(concrete_ao_map,        4);

    m_textured_models[1].AddTexture(concrete_albedo_map,    0);
    m_textured_models[1].AddTexture(concrete_normal_map,    1);
    m_textured_models[1].AddTexture(concrete_metallic_map,  2);
    m_textured_models[1].AddTexture(concrete_roughness_map, 3);
    m_textured_models[1].AddTexture(concrete_ao_map,        4);

    m_textured_models[2].AddTexture(plastic_albedo_map,    0);
    m_textured_models[2].AddTexture(plastic_normal_map,    1);
    m_textured_models[2].AddTexture(plastic_metallic_map,  2);
    m_textured_models[2].AddTexture(plastic_roughness_map, 3);
    m_textured_models[2].AddTexture(plastic_ao_map,        4);
    
    m_textured_models[3].AddTexture(gold_albedo_map,    0);
    m_textured_models[3].AddTexture(gold_normal_map,    1);
    m_textured_models[3].AddTexture(gold_metallic_map,  2);
    m_textured_models[3].AddTexture(gold_roughness_map, 3);
    m_textured_models[3].AddTexture(gold_ao_map,        4);

    m_textured_models[4].AddTexture(granite_albedo_map,    0);
    m_textured_models[4].AddTexture(granite_normal_map,    1);
    m_textured_models[4].AddTexture(granite_metallic_map,  2);
    m_textured_models[4].AddTexture(granite_roughness_map, 3);
    m_textured_models[4].AddTexture(granite_ao_map,        4);

    /* Create shader. */
    std::string dir = "../src/demos/22_pbr/";
    m_ambient_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-ambient.frag");
    m_ambient_light_shader->link();

    m_equirectangular_to_cubemap_shader = std::make_shared<RGL::Shader>(dir + "cubemap.vert", dir + "equirectangular_to_cubemap.frag");
    m_equirectangular_to_cubemap_shader->link();

    m_irradiance_convolution_shader = std::make_shared<RGL::Shader>(dir + "cubemap.vert", dir + "irradiance_convolution.frag");
    m_irradiance_convolution_shader->link();

    m_prefilter_env_map_shader = std::make_shared<RGL::Shader>(dir + "cubemap.vert", dir + "prefilter_cubemap.frag");
    m_prefilter_env_map_shader->link();

    m_precompute_brdf = std::make_shared<RGL::Shader>("../src/demos/10_postprocessing_filters/FSQ.vert", dir + "precompute_brdf.frag");
    m_precompute_brdf->link();

    m_background_shader = std::make_shared<RGL::Shader>(dir + "background.vert", dir + "background.frag");
    m_background_shader->link();

    m_tmo_ps = std::make_shared<PostprocessFilter>(RGL::Window::getWidth(), RGL::Window::getHeight());

    // IBL precomputations
    GenSkyboxGeometry();

    m_env_cubemap_rt = std::make_shared<CubeMapRenderTarget>();
    m_env_cubemap_rt->set_position(glm::vec3(0.0));
    m_env_cubemap_rt->generate_rt(2048, 2048, true);

    m_irradiance_cubemap_rt = std::make_shared<CubeMapRenderTarget>();
    m_irradiance_cubemap_rt->set_position(glm::vec3(0.0));
    m_irradiance_cubemap_rt->generate_rt(32, 32);

    m_prefiltered_env_map_rt = std::make_shared<CubeMapRenderTarget>();
    m_prefiltered_env_map_rt->set_position(glm::vec3(0.0));
    m_prefiltered_env_map_rt->generate_rt(512, 512, true);

    m_brdf_lut_rt = std::make_shared<Texture2DRenderTarget>();
    m_brdf_lut_rt->generate_rt(512, 512);

    PrecomputeIndirectLight(RGL::FileSystem::getPath("textures/skyboxes/IBL/" + m_hdr_maps_names[m_current_hdr_map_idx]));
    PrecomputeBRDF(m_brdf_lut_rt);

    // Shadows
    m_dir_light_shadow_map_res = glm::uvec2(1024);
    CreateDirectionalShadowMap(m_dir_light_shadow_map_res.x, m_dir_light_shadow_map_res.y);
    CreateShadowFBO(m_dir_shadow_map);

    m_shadow_map_pcf_sampler.Create();
    m_shadow_map_pcf_sampler.SetFiltering(RGL::TextureFiltering::MIN, RGL::TextureFilteringParam::LINEAR);
    m_shadow_map_pcf_sampler.SetFiltering(RGL::TextureFiltering::MIN, RGL::TextureFilteringParam::LINEAR);
    m_shadow_map_pcf_sampler.SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::CLAMP_TO_BORDER);
    m_shadow_map_pcf_sampler.SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::CLAMP_TO_BORDER);
    m_shadow_map_pcf_sampler.SetBorderColor(1, 0, 0, 0);
    m_shadow_map_pcf_sampler.SetCompareMode(RGL::TextureCompareMode::REF);
    m_shadow_map_pcf_sampler.SetCompareFunc(RGL::TextureCompareFunc::LEQUAL);

    uint32_t random_angles_size = 128;
    m_random_angles_tex3d_id = GenerateRandomAnglesTexture3D(random_angles_size);

    dir = "../src/demos/24_pcss/";
    m_generate_shadow_map_shader = std::make_shared<RGL::Shader>(dir + "generate_shadow_map.vert", dir + "generate_shadow_map.frag");
    m_generate_shadow_map_shader->link();

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting-shadow.vert", dir + "pbr-directional-shadow.frag");
    m_directional_light_shader->link();

    glEnable(GL_CULL_FACE);  
}

void PCSS::input()
{
    /* Close the application when Esc is released. */
    if (RGL::Input::getKeyUp(RGL::KeyCode::Escape))
    {
        stop();
    }

    /* Toggle between wireframe and solid rendering */
    if (RGL::Input::getKeyUp(RGL::KeyCode::F2))
    {
        static bool toggle_wireframe = false;

        toggle_wireframe = !toggle_wireframe;

        if (toggle_wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    /* It's also possible to take a screenshot. */
    if (RGL::Input::getKeyUp(RGL::KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "24_pcss";
        if (take_screenshot_png(filename, RGL::Window::getWidth() / 2.0, RGL::Window::getHeight() / 2.0))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << RGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
    }

    if (RGL::Input::getKeyUp(RGL::KeyCode::F3))
    {
        std::cout << "******** Camera properties : ********\n"
                  << " Position:    [" << m_camera->position().x << ", " << m_camera->position().y << ", " << m_camera->position().z << "]\n"
                  << " Orientation: [" << m_camera->orientation().w << ", "  << m_camera->orientation().x << ", " << m_camera->orientation().y << ", " << m_camera->orientation().z << "]\n"
                  << "*************************************n\n";
    }
}

void PCSS::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void PCSS::HdrEquirectangularToCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt, const std::shared_ptr<RGL::Texture2D>& m_equirectangular_map)
{
    /* Update all faces per frame */
    m_equirectangular_to_cubemap_shader->bind();
    m_equirectangular_to_cubemap_shader->setUniform("u_projection", cubemap_rt->m_projection);

    glViewport(0, 0, cubemap_rt->m_width, cubemap_rt->m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, cubemap_rt->m_fbo_id);
    m_equirectangular_map->Bind(1);

    for (uint8_t side = 0; side < 6; ++side)
    {
        m_equirectangular_to_cubemap_shader->setUniform("u_view", cubemap_rt->m_view_transforms[side]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, cubemap_rt->m_cubemap_texture_id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(m_skybox_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glViewport(0, 0, RGL::Window::getWidth(), RGL::Window::getHeight());
}

void PCSS::IrradianceConvolution(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
{
    /* Update all faces per frame */
    m_irradiance_convolution_shader->bind();
    m_irradiance_convolution_shader->setUniform("u_projection", cubemap_rt->m_projection);

    glViewport(0, 0, cubemap_rt->m_width, cubemap_rt->m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, cubemap_rt->m_fbo_id);
    m_env_cubemap_rt->bindTexture(1);

    for (uint8_t side = 0; side < 6; ++side)
    {
        m_irradiance_convolution_shader->setUniform("u_view", cubemap_rt->m_view_transforms[side]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, cubemap_rt->m_cubemap_texture_id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(m_skybox_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glViewport(0, 0, RGL::Window::getWidth(), RGL::Window::getHeight());
}

void PCSS::PrefilterCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
{
    m_prefilter_env_map_shader->bind();
    m_prefilter_env_map_shader->setUniform("u_projection", cubemap_rt->m_projection);
    
    m_env_cubemap_rt->bindTexture(1);

    glBindFramebuffer(GL_FRAMEBUFFER, cubemap_rt->m_fbo_id);

    uint8_t max_mip_levels = glm::log2(float(cubemap_rt->m_width));
    for (uint8_t mip = 0; mip < max_mip_levels; ++mip)
    {
        // resize the framebuffer according to mip-level size.
        uint32_t mip_width  = cubemap_rt->m_width  * std::pow(0.5, mip);
        uint32_t mip_height = cubemap_rt->m_height * std::pow(0.5, mip);

        glBindRenderbuffer(GL_RENDERBUFFER, cubemap_rt->m_rbo_id);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width, mip_height);
        glViewport(0, 0, mip_width, mip_height);

        float roughness = float(mip) / float(max_mip_levels - 1);
        m_prefilter_env_map_shader->setUniform("u_roughness", roughness);

        for (uint8_t side = 0; side < 6; ++side)
        {
            m_prefilter_env_map_shader->setUniform("u_view", cubemap_rt->m_view_transforms[side]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, cubemap_rt->m_cubemap_texture_id, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(m_skybox_vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, RGL::Window::getWidth(), RGL::Window::getHeight());
}

void PCSS::PrecomputeIndirectLight(const std::filesystem::path& hdri_map_filepath)
{
    auto envmap_hdr = std::make_shared<RGL::Texture2D>();
    envmap_hdr->LoadHdr(hdri_map_filepath);
    auto envmap_metadata = envmap_hdr->GetMetadata();

    HdrEquirectangularToCubemap(m_env_cubemap_rt, envmap_hdr);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_env_cubemap_rt->m_cubemap_texture_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    IrradianceConvolution(m_irradiance_cubemap_rt);
    PrefilterCubemap(m_prefiltered_env_map_rt);
}

void PCSS::PrecomputeBRDF(const std::shared_ptr<Texture2DRenderTarget>& rt)
{
    GLuint m_dummy_vao_id;
    glCreateVertexArrays(1, &m_dummy_vao_id);

    rt->bindRenderTarget();
    m_precompute_brdf->bind();

    glBindVertexArray(m_dummy_vao_id);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteVertexArrays(1, &m_dummy_vao_id);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, RGL::Window::getWidth(), RGL::Window::getHeight());
}

void PCSS::GenSkyboxGeometry()
{
    m_skybox_vao = 0;
    m_skybox_vbo = 0;

    glCreateVertexArrays(1, &m_skybox_vao);
    glCreateBuffers(1, &m_skybox_vbo);

    std::vector<float> skybox_positions = {
        // positions          
        -1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        // front face
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        // left face
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        // right face
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        // bottom face
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        // top face
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f , 1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
    };

    /* Set up buffer objects */
    glNamedBufferStorage(m_skybox_vbo, skybox_positions.size() * sizeof(skybox_positions[0]), skybox_positions.data(), 0 /*flags*/);

    /* Set up VAO */
    glEnableVertexArrayAttrib(m_skybox_vao, 0 /*index*/);

    /* Separate attribute format */
    glVertexArrayAttribFormat(m_skybox_vao, 0 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
    glVertexArrayAttribBinding(m_skybox_vao, 0 /*index*/, 0 /*bindingindex*/);
    glVertexArrayVertexBuffer(m_skybox_vao, 0 /*bindingindex*/, m_skybox_vbo, 0 /*offset*/, sizeof(glm::vec3) /*stride*/);
}

void PCSS::CreateDirectionalShadowMap(uint32_t width, uint32_t height)
{
    GLfloat border[] = { 1.0, 0.0, 0.0, 0.0 };

    glCreateTextures(GL_TEXTURE_2D, 1, &m_dir_shadow_map);
    glTextureStorage2D(m_dir_shadow_map, 1, GL_DEPTH_COMPONENT32F, width, height);

    glTextureParameteri(m_dir_shadow_map, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_dir_shadow_map, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_dir_shadow_map, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_dir_shadow_map, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTextureParameterfv(m_dir_shadow_map, GL_TEXTURE_BORDER_COLOR, border);
}

void PCSS::CreateShadowFBO(GLuint shadow_texture)
{
    glCreateFramebuffers(1, &m_shadow_fbo);
    glNamedFramebufferTexture(m_shadow_fbo, GL_DEPTH_ATTACHMENT, shadow_texture, 0);

    GLenum draw_buffers[] = { GL_NONE };
    glNamedFramebufferDrawBuffers(m_shadow_fbo, 1, draw_buffers);
}

void PCSS::GenerateShadowMap(uint32_t width, uint32_t height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);

    m_generate_shadow_map_shader->bind();
    m_generate_shadow_map_shader->setUniform("u_light_view_projection", m_dir_light_view_projection);

    for (uint32_t i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_generate_shadow_map_shader->setUniform("u_model", m_textured_models_model_matrices[i]);
        m_textured_models[i].Render();
    }
    glCullFace(GL_BACK);
}

GLuint PCSS::GenerateRandomAnglesTexture3D(uint32_t size)
{
    int buffer_size = size * size * size;
    auto data = std::make_unique<glm::vec2[]>(buffer_size);// glm::vec2[buffer_size];

    for (int z = 0; z < size; ++z)
    {
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                int index = x + y * size + z * size * size;

                float random_angle = RGL::Util::RandomDouble(0.0, glm::two_pi<double>()); // Random angles in range [0, 2PI);
                data[index] = glm::vec2(glm::cos(random_angle), glm::sin(random_angle)); // Map sine and cosine values to [0, 1] range
            }
        }
    }

    GLuint tex_id = 0;
    glCreateTextures   (GL_TEXTURE_3D, 1, &tex_id);
    glTextureStorage3D (tex_id, 1, GL_RG32F, size, size, size);
    glTextureSubImage3D(tex_id, 0, 0, 0, 0, size, size, size, GL_RG, GL_FLOAT, data.get());
    
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_R,     GL_REPEAT);
    glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return tex_id;
}

void PCSS::UpdateLightMatrix()
{
    glm::vec3 light_dir = m_dir_light_properties.direction;

    glm::vec3 max_extents = m_scene_bbox.max;
    glm::vec3 min_extents = m_scene_bbox.min;
    glm::vec3 scene_center = (max_extents + min_extents) * 0.5f;

    glm::mat4 light_view_matrix  = glm::lookAt(scene_center - light_dir * -min_extents.z, scene_center, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 light_ortho_matrix = glm::ortho(min_extents.x, max_extents.x, min_extents.y, max_extents.y, 0.0f, max_extents.z - min_extents.z);

    m_dir_shadow_frustum_size = max_extents.x - min_extents.x;

    m_dir_light_view            = light_view_matrix;
    m_dir_light_view_projection = light_ortho_matrix * light_view_matrix;
    m_dir_shadow_frustum_planes = glm::vec2(min_extents.z, max_extents.z);
}

void PCSS::RenderTexturedModels()
{
    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_ambient_light_shader->setUniform("u_has_albedo_map",    true);
    m_ambient_light_shader->setUniform("u_has_normal_map",    true);
    m_ambient_light_shader->setUniform("u_has_metallic_map",  true);
    m_ambient_light_shader->setUniform("u_has_roughness_map", true);
    m_ambient_light_shader->setUniform("u_has_ao_map",        true);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* First, render the ambient color only for the opaque objects. */
    m_irradiance_cubemap_rt->bindTexture(5);
    m_prefiltered_env_map_rt->bindTexture(6);
    m_brdf_lut_rt->bindTexture(7);

    for (uint32_t i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_ambient_light_shader->setUniform("u_model",         m_textured_models_model_matrices[i]);
        m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_textured_models_model_matrices[i]))));
        m_ambient_light_shader->setUniform("u_mvp",           view_projection * m_textured_models_model_matrices[i]);

        m_textured_models[i].Render();
    }

    /*
     * Disable writing to the depth buffer and additively
     * shade only those pixels, that were shaded in the ambient step.
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);

    /* Render directional light(s) */
    m_directional_light_shader->bind();
    m_directional_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_directional_light_shader->setUniform("u_has_albedo_map",    true);
    m_directional_light_shader->setUniform("u_has_normal_map",    true);
    m_directional_light_shader->setUniform("u_has_metallic_map",  true);
    m_directional_light_shader->setUniform("u_has_roughness_map", true);

    m_directional_light_shader->setUniform("u_directional_light.base.color",     m_dir_light_properties.color);
    m_directional_light_shader->setUniform("u_directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("u_directional_light.direction",      m_dir_light_properties.direction);
    m_directional_light_shader->setUniform("u_light_view_projection",            m_dir_light_view_projection);
    m_directional_light_shader->setUniform("u_light_view",                       m_dir_light_view);

    m_directional_light_shader->setUniform("u_blocker_search_samples", m_blocker_search_samples);
    m_directional_light_shader->setUniform("u_pcf_samples",            m_pcf_filter_samples);
    m_directional_light_shader->setUniform("u_light_radius_uv",        m_light_radius_uv / (m_dir_shadow_frustum_size * 2.0f));
    m_directional_light_shader->setUniform("u_light_near",             m_dir_shadow_frustum_planes.x);
    m_directional_light_shader->setUniform("u_light_far",              m_dir_shadow_frustum_planes.y);

    m_shadow_map_pcf_sampler.Bind(9);

    glBindTextureUnit(8, m_dir_shadow_map);
    glBindTextureUnit(9, m_dir_shadow_map); 
    glBindTextureUnit(10, m_random_angles_tex3d_id);
   
    for (unsigned i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_directional_light_shader->setUniform("u_model",         m_textured_models_model_matrices[i]);
        m_directional_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_textured_models_model_matrices[i]))));
        m_directional_light_shader->setUniform("u_mvp",           view_projection * m_textured_models_model_matrices[i]);

        m_textured_models[i].Render();
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void PCSS::render()
{
    // Generate shadow map
    GenerateShadowMap(m_dir_light_shadow_map_res.x, m_dir_light_shadow_map_res.y);

    /* Put render specific code here. Don't update variables here! */
    m_tmo_ps->bindFilterFBO();
    glViewport(0, 0, RGL::Window::getWidth(), RGL::Window::getHeight());

    RenderTexturedModels();

    m_background_shader->bind();
    m_background_shader->setUniform("u_projection", m_camera->m_projection);
    m_background_shader->setUniform("u_view", glm::mat4(glm::mat3(m_camera->m_view)));
    m_background_shader->setUniform("u_lod_level", m_background_lod_level);
    m_env_cubemap_rt->bindTexture();
    
    glCullFace(GL_FRONT);
    glBindVertexArray(m_skybox_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glCullFace(GL_BACK);

    m_tmo_ps->render(m_exposure, m_gamma);
}

void PCSS::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos       = ImVec2(RGL::Window::getWidth() - 10.0, 10.0);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 400, 0 });

    ImGui::Begin("Info");
    {
        if (ImGui::CollapsingHeader("Help"))
        {
            ImGui::Text("Controls info: \n\n"
                        "F1     - take a screenshot\n"
                        "F2     - toggle wireframe rendering\n"
                        "WASDQE - control camera movement\n"
                        "RMB    - press to rotate the camera\n"
                        "Esc    - close the app\n\n");
        }

        ImGui::Spacing();
        ImGui::Text("# General");

        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
        ImGui::SliderFloat("Exposure",             &m_exposure,             0.0, 10.0, "%.1f");
        ImGui::SliderFloat("Gamma",                &m_gamma,                0.0, 10.0, "%.1f");
        ImGui::SliderFloat("Background LOD level", &m_background_lod_level, 0.0, glm::log2(float(m_env_cubemap_rt->m_width)), "%.1f");

        if (ImGui::BeginCombo("HDR map", m_hdr_maps_names[m_current_hdr_map_idx].c_str()))
        {
            for (int i = 0; i < std::size(m_hdr_maps_names); ++i)
            {
                bool is_selected = (m_current_hdr_map_idx == i);
                if (ImGui::Selectable(m_hdr_maps_names[i].c_str(), is_selected))
                {
                    glDisable(GL_CULL_FACE);
                    m_current_hdr_map_idx = i;
                    PrecomputeIndirectLight(RGL::FileSystem::getPath("textures/skyboxes/IBL/" + m_hdr_maps_names[m_current_hdr_map_idx]));
                    glEnable(GL_CULL_FACE);
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Text("# PCSS settings");

        {
            const char* listbox_items[] = { "25", "32", "64", "100", "128" };
            const int   sample_counts[] = { 25, 32, 64, 100, 128 };

            if (ImGui::Combo("Blocker search samples", &m_blocker_search_samples_idx, listbox_items, std::size(listbox_items)))
            {
                m_blocker_search_samples = sample_counts[m_blocker_search_samples_idx];
            }

            if (ImGui::Combo("PCF filter samples", &m_pcf_filter_samples_idx, listbox_items, std::size(listbox_items)))
            {
                m_pcf_filter_samples = sample_counts[m_pcf_filter_samples_idx];
            }

            ImGui::SliderFloat("Light radius", &m_light_radius_uv, 0.0, 1.0, "%.2f");
        }

        ImGui::PopItemWidth();

        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3 ("Color",                 &m_dir_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",       &m_dir_light_properties.intensity, 0.0, 10.0,  "%.1f");
                    
                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_dir_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        if (glm::epsilonEqual(m_dir_light_angles.y, 0.0f, 1e-5f)) m_dir_light_angles.y = 1e-5f;

                        m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);
                        
                        UpdateLightMatrix();
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
