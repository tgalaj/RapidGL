#include "cascaded_pcss.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

CascadedPCSS::CascadedPCSS()
      : m_dir_light_angles         (-35.0f, 65.0f),
        m_spot_light_angles        (90.0f, -25.0f),
        m_exposure                 (0.3f),
        m_gamma                    (3.6f),
        m_background_lod_level     (1.2),
        m_skybox_vao               (0),
        m_skybox_vbo               (0),
        m_shadow_fbo               (0),
        m_dir_shadow_maps          (0),
        m_dir_shadow_frustum_size  (20.0f),
        m_dir_shadow_frustum_planes(120, 250)
{
}

CascadedPCSS::~CascadedPCSS()
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

    if (m_dir_shadow_maps != 0)
    {
        glDeleteTextures(1, &m_dir_shadow_maps);
        m_dir_shadow_maps = 0;
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

void CascadedPCSS::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 50.0);
    m_camera->setPosition(-10.3, 7.6, -5.42);
    m_camera->setOrientation(glm::quat(-0.3, -0.052, -0.931, -0.165));

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 10.0f;
    //m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);
    m_dir_light_properties.direction = glm::vec3(1.0, -1.0, 0.0);

    /* Create models. */
    m_textured_models[0].GenCube();
    m_textured_models[1].Load(RGL::FileSystem::getPath("models/hk/hk.obj"));

    m_textured_models_model_matrices[0] = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(24.0, 0.1, 24.0));
    
    // TODO: clean this stuff
    float hk_unit_scale_factor = m_textured_models[1].GetUnitScaleFactor() * 2.0f;
    float radius = hk_unit_scale_factor * 3.0f;
    glm::vec3 offset = glm::vec3(1, 0, 1);
    glm::vec3 start_pos = -((radius + offset) * (10 - 1.0f)) / 2.0f;

    for (uint32_t y = 0; y < 10; ++y)
    {
        for (uint32_t x = 0; x < 10; ++x)
        {
            glm::vec3 position = start_pos + glm::vec3(x, 0.0, y) * (radius + offset);
            position.y = 0.0f;
            m_textured_models_model_matrices[1 + x + y * 10] = glm::translate(glm::mat4(1.0), position) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(hk_unit_scale_factor));
        }
    }

    /* Add textures to the objects. */
    auto concrete_albedo_map    = std::make_shared<RGL::Texture2D>(); concrete_albedo_map   ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_color.png"), true);
    auto concrete_normal_map    = std::make_shared<RGL::Texture2D>(); concrete_normal_map   ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_normal.png"));
    auto concrete_metallic_map  = std::make_shared<RGL::Texture2D>(); concrete_metallic_map ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_metallic.png"));
    auto concrete_roughness_map = std::make_shared<RGL::Texture2D>(); concrete_roughness_map->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_roughness.png"));
    auto concrete_ao_map        = std::make_shared<RGL::Texture2D>(); concrete_ao_map       ->Load(RGL::FileSystem::getPath("textures/pbr/concrete034_1k/concrete034_1K_ao.png"));

    m_textured_models[0].AddTexture(concrete_albedo_map,    0);
    m_textured_models[0].AddTexture(concrete_normal_map,    1);
    m_textured_models[0].AddTexture(concrete_metallic_map,  2);
    m_textured_models[0].AddTexture(concrete_roughness_map, 3);
    m_textured_models[0].AddTexture(concrete_ao_map,        4);

    auto hk_albedo_map = std::make_shared<RGL::Texture2D>(); hk_albedo_map->Load(RGL::FileSystem::getPath("models/hk/albedo.png"), true);
    auto hk_normal_map = std::make_shared<RGL::Texture2D>(); hk_normal_map->Load(RGL::FileSystem::getPath("models/hk/normal.png"));

    m_textured_models[1].AddTexture(hk_albedo_map, 0);
    m_textured_models[1].AddTexture(hk_normal_map, 1);

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
    dir = "../src/demos/25_cascaded_pcss/";
    m_generate_shadow_map_shader = std::make_shared<RGL::Shader>(dir + "generate_csm.vert", dir + "generate_csm.frag", dir + "generate_csm.geom");
    m_generate_shadow_map_shader->link();

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting-shadow.vert", dir + "pbr-directional-shadow.frag");
    m_directional_light_shader->link();

    m_visualize_shadow_map_shader = std::make_shared<RGL::Shader>("../src/demos/10_postprocessing_filters/FSQ.vert", dir + "visualize_csm_depth.frag");
    m_visualize_shadow_map_shader->link();

    // TODO: use log splits from the shadows book
    m_dir_light_view_projection_matrices.resize(NUM_CASCADES);
    m_dir_light_view_matrices.resize(NUM_CASCADES);
    m_cascade_splits.resize(NUM_CASCADES + 1);
    m_cascade_splits = { m_camera->NearPlane(), 15.0, 30.0, m_camera->FarPlane() };

    for (uint32_t i = 0; i < NUM_CASCADES; ++i)
    {
        auto split_view_space = glm::vec4(0.0, 0.0, -m_cascade_splits[i + 1], 1.0);
        auto split_clip_space = m_camera->m_projection * split_view_space;

        // printf("<%.2f, %.2f, %.2f, %.2f>\n", split_clip_space.x, split_clip_space.y, split_clip_space.z, split_clip_space.w);
        m_directional_light_shader->setUniform("u_cascade_end_clip_space[" + std::to_string(i) + "]", split_clip_space.z);
    }

    m_dir_light_shadow_map_res = glm::uvec2(1024 * 4);
    CreateShadowFBO(m_dir_light_shadow_map_res.x, m_dir_light_shadow_map_res.y);

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

    glEnable(GL_CULL_FACE);  

    glCreateBuffers(1, &m_csm_frusta_vbo);
    glNamedBufferStorage(m_csm_frusta_vbo, NUM_CASCADES * 12 * 2 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_STORAGE_BIT);
    
    glCreateVertexArrays(1, &m_csm_frusta_vao);
    glVertexArrayVertexBuffer(m_csm_frusta_vao, 0, m_csm_frusta_vbo, 0, sizeof(glm::vec3));

    glEnableVertexArrayAttrib(m_csm_frusta_vao, 0);
    glVertexArrayAttribFormat(m_csm_frusta_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_csm_frusta_vao, 0, 0);
}

void CascadedPCSS::input()
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
        std::string filename = "25_cascaded_pcss";
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

void CascadedPCSS::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void CascadedPCSS::HdrEquirectangularToCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt, const std::shared_ptr<RGL::Texture2D>& m_equirectangular_map)
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

void CascadedPCSS::IrradianceConvolution(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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

void CascadedPCSS::PrefilterCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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

void CascadedPCSS::PrecomputeIndirectLight(const std::filesystem::path& hdri_map_filepath)
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

void CascadedPCSS::PrecomputeBRDF(const std::shared_ptr<Texture2DRenderTarget>& rt)
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

void CascadedPCSS::GenSkyboxGeometry()
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

void CascadedPCSS::CreateShadowFBO(uint32_t width, uint32_t height)
{
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_dir_shadow_maps);
    glTextureStorage3D(m_dir_shadow_maps, 1, GL_DEPTH_COMPONENT32F, width, height, NUM_CASCADES);

    glTextureParameteri(m_dir_shadow_maps, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_dir_shadow_maps, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_dir_shadow_maps, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
    glTextureParameteri(m_dir_shadow_maps, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTextureParameterfv(m_dir_shadow_maps, GL_TEXTURE_BORDER_COLOR, bordercolor);

    glCreateFramebuffers(1, &m_shadow_fbo);
    glNamedFramebufferTexture(m_shadow_fbo, GL_DEPTH_ATTACHMENT, m_dir_shadow_maps, 0);

    GLenum draw_buffers[] = { GL_NONE };
    glNamedFramebufferDrawBuffers(m_shadow_fbo, 1, draw_buffers);
    glNamedFramebufferReadBuffer(m_shadow_fbo, GL_NONE);

    int status = glCheckNamedFramebufferStatus(m_shadow_fbo, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
    }
    else std::cout << "SUCCESS::FRAMEBUFFER:: Framebuffer is complete!\n";

}


void CascadedPCSS::GenerateShadowMap(uint32_t width, uint32_t height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_fbo);
    glViewport(0, 0, width, height);
    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);

    m_generate_shadow_map_shader->bind();

    calculate_frusta_matrices();
    // m_dir_light_view_projection_matrices = get_light_space_matrices();
    m_generate_shadow_map_shader->setUniform("u_light_view_projections", m_dir_light_view_projection_matrices.data(), m_dir_light_view_projection_matrices.size());

    for (uint32_t i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_generate_shadow_map_shader->setUniform("u_model", m_textured_models_model_matrices[i]);
    
        if (i > 1)
        {
            m_textured_models[1].Render();
        }
        else
        {
            m_textured_models[i].Render();
        }
    }

    glCullFace(GL_BACK);
}

GLuint CascadedPCSS::GenerateRandomAnglesTexture3D(uint32_t size)
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

void CascadedPCSS::calculate_frusta_matrices()
{
    glm::mat4 view_inv = glm::inverse(m_camera->m_view);
    // glm::mat4 light_view = glm::lookAt(glm::vec3(0, 0, 0) + m_dir_light_properties.direction * 20.0f, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    float ar = m_camera->AspectRatio();
    float inv_ar = 1.0 / ar;
    float tan_half_h_fov = glm::tan(glm::radians(m_camera->FOV() / 2.0f));
    float tan_half_v_fov = glm::tan(glm::radians(m_camera->FOV() * inv_ar / 2.0f));

    for (uint32_t i = 0; i < NUM_CASCADES; ++i)
    {
        float xn = m_cascade_splits[i] * tan_half_h_fov;
        float yn = m_cascade_splits[i] * tan_half_v_fov;
        
        float xf = m_cascade_splits[i + 1] * tan_half_h_fov;
        float yf = m_cascade_splits[i + 1] * tan_half_v_fov;

        glm::vec4 frustum_corners[NUM_FRUSTUM_CORNERS] = {
            // near face
            glm::vec4( xn,  yn, m_cascade_splits[i], 1.0),
            glm::vec4(-xn,  yn, m_cascade_splits[i], 1.0),
            glm::vec4( xn, -yn, m_cascade_splits[i], 1.0),
            glm::vec4(-xn, -yn, m_cascade_splits[i], 1.0),

            // far face
            glm::vec4( xf,  yf, m_cascade_splits[i + 1], 1.0),
            glm::vec4(-xf,  yf, m_cascade_splits[i + 1], 1.0),
            glm::vec4( xf, -yf, m_cascade_splits[i + 1], 1.0),
            glm::vec4(-xf, -yf, m_cascade_splits[i + 1], 1.0)
        };

        glm::vec3 center(0.0f);
        for (const auto& c : frustum_corners)
        {
            center += glm::vec3(c);
        }
        center /= float(std::size(frustum_corners));
        center = glm::vec3(view_inv * glm::vec4(center, 1.0));

        // TODO: Something is wrong with the light_view matrix
        // glm::mat4 light_view = glm::lookAt(center - m_dir_light_properties.direction, center, glm::vec3(0, 1, 0));
        // glm::mat4 light_view = m_camera->m_view;
        //glm::mat4 light_view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0) + m_dir_light_properties.direction, glm::vec3(0, 1, 0));
        glm::vec3 right = glm::vec3(1, 0, 0);
        glm::vec3 up = glm::normalize(glm::cross(-m_dir_light_properties.direction, right));
        glm::mat4 light_view = glm::lookAt(center, center + m_dir_light_properties.direction, up);

        // glm::vec3 p1 = glm::normalize(glm::cross(-m_dir_light_properties.direction, glm::vec3(0, 0, 1)));
        // glm::vec3 p2 = glm::normalize(glm::cross(-m_dir_light_properties.direction, p1));
        // glm::mat4 light_view = glm::mat4(1.0);
        // light_view[0] = glm::vec4(p1, 0.0);
        // light_view[1] = glm::vec4(p2, 0.0);
        // light_view[2] = glm::vec4(-m_dir_light_properties.direction, 0.0);

        glm::vec4 frustum_corners_light[NUM_FRUSTUM_CORNERS];

        float min_x =  std::numeric_limits<float>::infinity();
        float max_x = -std::numeric_limits<float>::infinity();
        float min_y =  std::numeric_limits<float>::infinity();
        float max_y = -std::numeric_limits<float>::infinity();
        float min_z =  std::numeric_limits<float>::infinity();
        float max_z = -std::numeric_limits<float>::infinity();

        float min_x_ws =  std::numeric_limits<float>::infinity();
        float max_x_ws = -std::numeric_limits<float>::infinity();
        float min_y_ws =  std::numeric_limits<float>::infinity();
        float max_y_ws = -std::numeric_limits<float>::infinity();
        float min_z_ws =  std::numeric_limits<float>::infinity();
        float max_z_ws = -std::numeric_limits<float>::infinity();

        for (uint32_t j = 0; j < NUM_FRUSTUM_CORNERS; ++j)
        {
            glm::vec4 v_w            = view_inv * frustum_corners[j]; // Transform to world space
            frustum_corners_light[j] = light_view * v_w; // Transform to light's view space

            min_x = glm::min(min_x, frustum_corners_light[j].x);
            max_x = glm::max(max_x, frustum_corners_light[j].x);
            min_y = glm::min(min_y, frustum_corners_light[j].y);
            max_y = glm::max(max_y, frustum_corners_light[j].y);
            min_z = glm::min(min_z, frustum_corners_light[j].z);
            max_z = glm::max(max_z, frustum_corners_light[j].z);

            min_x_ws = glm::min(min_x, v_w.x);
            max_x_ws = glm::max(max_x, v_w.x);
            min_y_ws = glm::min(min_y, v_w.y);
            max_y_ws = glm::max(max_y, v_w.y);
            min_z_ws = glm::min(min_z, v_w.z);
            max_z_ws = glm::max(max_z, v_w.z);
        }

        m_ortho_frusta[i] = { min_x_ws, max_x_ws, min_y_ws, max_y_ws, min_z_ws, max_z_ws };
        // m_ortho_frusta[i] = {-1, 1, -1, 1, -1, 1};

        // if (i == 0) m_camera->m_view = light_view;
        // if (i == 0) m_camera->m_projection = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z);
        glm::vec3 extents(max_x - min_x, max_y - min_y, max_z - min_z);
        glm::vec3 he = extents * 0.5f;
        m_dir_light_view_projection_matrices[i] = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z) * light_view;
    }
}

void CascadedPCSS::draw_csm_frusta()
{
    std::vector<glm::vec3> frusta_lines_points(2 * 12 * NUM_CASCADES);

    for (uint32_t i = 0; i < NUM_CASCADES; ++i)
    {
        Frustum f = m_ortho_frusta[i];

        // Near plane
        frusta_lines_points[24 * i + 0] = glm::vec3(f.min_x, f.min_y, f.min_z);
        frusta_lines_points[24 * i + 1] = glm::vec3(f.max_x, f.min_y, f.min_z);
        frusta_lines_points[24 * i + 2] = glm::vec3(f.max_x, f.min_y, f.min_z);
        frusta_lines_points[24 * i + 3] = glm::vec3(f.max_x, f.max_y, f.min_z);
        frusta_lines_points[24 * i + 4] = glm::vec3(f.max_x, f.max_y, f.min_z);
        frusta_lines_points[24 * i + 5] = glm::vec3(f.min_x, f.max_y, f.min_z);
        frusta_lines_points[24 * i + 6] = glm::vec3(f.min_x, f.max_y, f.min_z);
        frusta_lines_points[24 * i + 7] = glm::vec3(f.min_x, f.min_y, f.min_z);

        // Far plane
        frusta_lines_points[24 * i + 8]  = glm::vec3(f.min_x, f.min_y, f.max_z);
        frusta_lines_points[24 * i + 9]  = glm::vec3(f.max_x, f.min_y, f.max_z);
        frusta_lines_points[24 * i + 10] = glm::vec3(f.max_x, f.min_y, f.max_z);
        frusta_lines_points[24 * i + 11] = glm::vec3(f.max_x, f.max_y, f.max_z);
        frusta_lines_points[24 * i + 12] = glm::vec3(f.max_x, f.max_y, f.max_z);
        frusta_lines_points[24 * i + 13] = glm::vec3(f.min_x, f.max_y, f.max_z);
        frusta_lines_points[24 * i + 14] = glm::vec3(f.min_x, f.max_y, f.max_z);
        frusta_lines_points[24 * i + 15] = glm::vec3(f.min_x, f.min_y, f.max_z);

        // Left plane
        frusta_lines_points[24 * i + 16] = glm::vec3(f.min_x, f.max_y, f.min_z);
        frusta_lines_points[24 * i + 17] = glm::vec3(f.min_x, f.max_y, f.max_z);
        frusta_lines_points[24 * i + 18] = glm::vec3(f.min_x, f.min_y, f.min_z);
        frusta_lines_points[24 * i + 19] = glm::vec3(f.min_x, f.min_y, f.max_z);

        // Right plane
        frusta_lines_points[24 * i + 20] = glm::vec3(f.max_x, f.max_y, f.min_z);
        frusta_lines_points[24 * i + 21] = glm::vec3(f.max_x, f.max_y, f.max_z);
        frusta_lines_points[24 * i + 22] = glm::vec3(f.max_x, f.min_y, f.min_z);
        frusta_lines_points[24 * i + 23] = glm::vec3(f.max_x, f.min_y, f.max_z);
    }

    glBindVertexArray(m_csm_frusta_vao);
    glNamedBufferSubData(m_csm_frusta_vbo, 0, sizeof(frusta_lines_points[0]) * frusta_lines_points.size(), frusta_lines_points.data());
    glDrawArrays(GL_LINES, 0, 12 * NUM_CASCADES);
}

void CascadedPCSS::RenderTexturedModels()
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

        if (i > 1)
        {
            m_textured_models[1].Render();
        }
        else
        {
            m_textured_models[i].Render();
        }
    }

    m_ambient_light_shader->setUniform("u_has_albedo_map",    false);
    m_ambient_light_shader->setUniform("u_has_metallic_map",  false);
    m_ambient_light_shader->setUniform("u_has_roughness_map", false);
    m_ambient_light_shader->setUniform("u_has_ao_map",        false);
    m_ambient_light_shader->setUniform("u_metallic",          0.0f);
    m_ambient_light_shader->setUniform("u_roughness",         1.0f);
    m_ambient_light_shader->setUniform("u_ao",                1.0f);

    if (m_draw_debug_frusta)
    {
        m_ambient_light_shader->setUniform("u_albedo",        glm::vec3(100.0, 0.0, 0.0));
        m_ambient_light_shader->setUniform("u_model",         glm::mat4(1.0f));
        m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(1.0f));
        m_ambient_light_shader->setUniform("u_mvp",           view_projection);
        draw_csm_frusta();
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
    // m_directional_light_shader->setUniform("u_cam_pos",           m_camera->position());
    // m_directional_light_shader->setUniform("u_has_albedo_map",    true);
    // m_directional_light_shader->setUniform("u_has_normal_map",    true);
    // m_directional_light_shader->setUniform("u_has_metallic_map",  true);
    // m_directional_light_shader->setUniform("u_has_roughness_map", true);

    // m_directional_light_shader->setUniform("u_directional_light.base.color",     m_dir_light_properties.color);
    // m_directional_light_shader->setUniform("u_directional_light.base.intensity", m_dir_light_properties.intensity);
    // m_directional_light_shader->setUniform("u_directional_light.direction",      m_dir_light_properties.direction);
    m_directional_light_shader->setUniform("u_light_view_projections",           m_dir_light_view_projection_matrices.data(), m_dir_light_view_projection_matrices.size());
    m_directional_light_shader->setUniform("u_light_views",                      m_dir_light_view_matrices.data(), m_dir_light_view_matrices.size());

    // m_directional_light_shader->setUniform("u_blocker_search_samples", m_blocker_search_samples);
    // m_directional_light_shader->setUniform("u_pcf_samples",            m_pcf_filter_samples);
    // m_directional_light_shader->setUniform("u_light_radius_uv",        m_light_radius_uv); 
    // m_directional_light_shader->setUniform("u_light_near",             m_dir_shadow_frustum_planes.x);
    // m_directional_light_shader->setUniform("u_light_far",              m_dir_shadow_frustum_planes.y);
 
    glBindTextureUnit(8, m_dir_shadow_maps);
    glBindTextureUnit(9, m_dir_shadow_maps);
    m_shadow_map_pcf_sampler.Bind(9);

    glBindTextureUnit(10, m_random_angles_tex3d_id);

    for (unsigned i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_directional_light_shader->setUniform("u_model",         m_textured_models_model_matrices[i]);
        m_directional_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_textured_models_model_matrices[i]))));
        m_directional_light_shader->setUniform("u_mvp",           view_projection * m_textured_models_model_matrices[i]);
        //m_directional_light_shader->setUniform("u_mv",            m_camera->m_view * m_textured_models_model_matrices[i]);

        if (i > 1)
        {
            m_textured_models[1].Render();
        }
        else
        {
            m_textured_models[i].Render();
        }
    }

    // m_directional_light_shader->setUniform("u_has_metallic_map",  false);
    // m_directional_light_shader->setUniform("u_has_roughness_map", false);
    // m_directional_light_shader->setUniform("u_metallic",          0.0f);
    // m_directional_light_shader->setUniform("u_roughness",         1.0f);

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void CascadedPCSS::render()
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

    // visualize shadow maps
    if (m_draw_debug_visualize_shadow_maps)
    {
        glBindVertexArray(m_tmo_ps->m_dummy_vao_id);
        glBindTextureUnit(0, m_dir_shadow_maps);
        m_visualize_shadow_map_shader->bind();

        for(uint32_t i = 0; i < NUM_CASCADES; ++i)
        {
            static uint32_t width = RGL::Window::getWidth() * 0.2;
            glViewport(width * i, 0, width, width);
            m_visualize_shadow_map_shader->setUniform("u_layer", int(i));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }
}

void CascadedPCSS::render_gui()
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

        ImGui::Checkbox("Draw debug lights' frusta", &m_draw_debug_frusta);
        ImGui::Checkbox("Show shadow maps", &m_draw_debug_visualize_shadow_maps);

        ImGui::Spacing();

        {
            const char* listbox_items[] = { "25", "32", "64", "100", "128" };
            const int   sample_counts[] = { 25, 32, 64, 100, 128 };

            if (ImGui::ListBox("Blocker search samples", &m_blocker_search_samples_idx, listbox_items, std::size(listbox_items), 5))
            {
                m_blocker_search_samples = sample_counts[m_blocker_search_samples_idx];
            }

            if (ImGui::ListBox("PCF filter samples", &m_pcf_filter_samples_idx, listbox_items, std::size(listbox_items), 5))
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
                        m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);
                        
                        auto light_target = glm::vec3(0.0);
                        auto light_cam_pos = light_target - m_dir_light_properties.direction * 200.0f;
                        auto view = glm::lookAt(light_cam_pos, light_target, glm::vec3(0, 1, 0));
                        auto proj = glm::ortho(-m_dir_shadow_frustum_size, m_dir_shadow_frustum_size, -m_dir_shadow_frustum_size, m_dir_shadow_frustum_size, m_dir_shadow_frustum_planes.x, m_dir_shadow_frustum_planes.y);

                        // TODO:
                        // m_dir_light_view = view;
                        // m_dir_light_view_projection = proj * view;
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
