#include "clustered_shading.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>

#define IMAGE_UNIT_WRITE 0

using namespace RGL;

// Convert HSV to RGB:
// Source: https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
// Retrieved: 28/04/2016
// @param H Hue in the range [0, 360)
// @param S Saturation in the range [0, 1]
// @param V Value in the range [0, 1]
glm::vec3 hsv2rgb(float H, float S, float V)
{
    float C = V * S;
    float m = V - C;
    float H2 = H / 60.0f;
    float X = C * (1.0f - fabsf(fmodf(H2, 2.0f) - 1.0f));

    glm::vec3 RGB;

    switch (static_cast<int>(H2))
    {
    case 0:
        RGB = { C, X, 0 };
        break;
    case 1:
        RGB = { X, C, 0 };
        break;
    case 2:
        RGB = { 0, C, X };
        break;
    case 3:
        RGB = { 0, X, C };
        break;
    case 4:
        RGB = { X, 0, C };
        break;
    case 5:
        RGB = { C, 0, X };
        break;
    }

    return RGB + m;
}

ClusteredShading::ClusteredShading()
      : m_exposure            (3.0f),
        m_gamma               (2.2f),
        m_background_lod_level(1.2),
        m_skybox_vao          (0),
        m_skybox_vbo          (0),
        m_threshold           (1.5),
        m_knee                (0.1),
        m_bloom_intensity     (1.0),
        m_bloom_dirt_intensity(1.0),
        m_bloom_enabled       (true)
{
}

ClusteredShading::~ClusteredShading()
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

    glDeleteBuffers(1, &m_clusters_ssbo_id);
}

void ClusteredShading::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /* Create virtual camera. */
    m_camera = std::make_shared<Camera>(60.0, Window::getAspectRatio(), 1.0, 300.0);
    m_camera->setPosition(-8.32222, 1.9269, -0.768721);
    m_camera->setOrientation(glm::quat(0.634325, 0.0407623, 0.772209, 0.0543523));
   
    /* Init clustered shading variables. */
    float z_near       = m_camera->NearPlane();
    float z_far        = m_camera->FarPlane();
    float far_near_log = std::log2f(z_far / z_near);
    
    m_slice_scale = (float)m_grid_size.z / far_near_log;
    m_slice_bias  = -((float)m_grid_size.z * std::log2f(z_near) / far_near_log);

    /* Randomly initialize lights */
    GeneratePointLights();

    /* Create Sponza static object */
    auto sponza_model = std::make_shared<StaticModel>();
    sponza_model->Load(RGL::FileSystem::getPath("models/sponza/Sponza.gltf"));

    glm::mat4 world_trans = glm::mat4(1.0f);
    world_trans = glm::scale(world_trans, glm::vec3(sponza_model->GetUnitScaleFactor() * 30.0f));
    m_sponza_static_object = StaticObject(sponza_model, world_trans);

    /* Prepare SSBOs */
    uint32_t clusters_count = m_grid_size.x * m_grid_size.y * m_grid_size.z;

    glCreateBuffers(1, &m_clusters_ssbo_id);
    glNamedBufferData(m_clusters_ssbo_id, sizeof(ClusterAABB) * clusters_count, nullptr, GL_STATIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_clusters_ssbo_id);

    /* Create shader. */
    std::string dir = "../src/demos/27_clustered_shading/";
    m_generate_clusters_shader = std::make_shared<Shader>(dir + "generate_clusters.comp");
    m_generate_clusters_shader->link();

    m_ambient_light_shader = std::make_shared<Shader>(dir + "pbr-lighting.vert", dir + "pbr-ambient.frag");
    m_ambient_light_shader->link();

    m_point_light_shader = std::make_shared<Shader>(dir + "pbr-lighting.vert", dir + "pbr-point.frag");
    m_point_light_shader->link();

    dir = "../src/demos/22_pbr/";
    m_equirectangular_to_cubemap_shader = std::make_shared<Shader>(dir + "cubemap.vert", dir + "equirectangular_to_cubemap.frag");
    m_equirectangular_to_cubemap_shader->link();

    m_irradiance_convolution_shader = std::make_shared<Shader>(dir + "cubemap.vert", dir + "irradiance_convolution.frag");
    m_irradiance_convolution_shader->link();

    m_prefilter_env_map_shader = std::make_shared<Shader>(dir + "cubemap.vert", dir + "prefilter_cubemap.frag");
    m_prefilter_env_map_shader->link();

    m_precompute_brdf = std::make_shared<Shader>("../src/demos/10_postprocessing_filters/FSQ.vert", dir + "precompute_brdf.frag");
    m_precompute_brdf->link();

    m_background_shader = std::make_shared<Shader>(dir + "background.vert", dir + "background.frag");
    m_background_shader->link();

    m_tmo_ps = std::make_shared<PostprocessFilter>(Window::getWidth(), Window::getHeight());

    /* Bloom shaders. */
    dir = "../src/demos/26_bloom/";
    m_downscale_shader = std::make_shared<Shader>(dir + "downscale.comp");
    m_downscale_shader->link();

    m_upscale_shader = std::make_shared<Shader>(dir + "upscale.comp");
    m_upscale_shader->link();

    m_bloom_dirt_texture = std::make_shared<Texture2D>(); 
    m_bloom_dirt_texture->Load(FileSystem::getPath("textures/bloom_dirt_mask.png"));

    /* IBL precomputations. */
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
    m_brdf_lut_rt->create(512, 512, GL_RG16F);

    PrecomputeIndirectLight(FileSystem::getPath("textures/skyboxes/IBL/" + m_hdr_maps_names[m_current_hdr_map_idx]));
    PrecomputeBRDF(m_brdf_lut_rt);

    /* Generate clusters' AABBs */
    glm::vec2 cluster_size    = glm::vec2(RGL::Window::getWidth(), RGL::Window::getHeight()) / glm::vec2(m_grid_size);
    glm::vec2 view_pixel_size = 1.0f / glm::vec2(RGL::Window::getWidth(), RGL::Window::getHeight());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_clusters_ssbo_id);
    m_generate_clusters_shader->bind();
    m_generate_clusters_shader->setUniform("zNear",             m_camera->NearPlane());
    m_generate_clusters_shader->setUniform("zFar",              m_camera->FarPlane());
    m_generate_clusters_shader->setUniform("clusterSize",       cluster_size);
    m_generate_clusters_shader->setUniform("viewPxSize",        view_pixel_size);
    m_generate_clusters_shader->setUniform("inverseProjection", glm::inverse(m_camera->m_projection));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ClusteredShading::input()
{
    /* Close the application when Esc is released. */
    if (Input::getKeyUp(KeyCode::Escape))
    {
        stop();
    }

    /* Toggle between wireframe and solid rendering */
    if (Input::getKeyUp(KeyCode::F2))
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
    if (Input::getKeyUp(KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "27_clustered_shading";
        if (take_screenshot_png(filename, Window::getWidth() / 2.0, Window::getHeight() / 2.0))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << FileSystem::getPath("../screenshots/") << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << FileSystem::getPath("../screenshots/") << std::endl;
        }
    }

    if (Input::getKeyUp(KeyCode::F3))
    {
        std::cout << "******** Camera properties : ********\n"
                  << " Position:    [" << m_camera->position().x << ", " << m_camera->position().y << ", " << m_camera->position().z << "]\n"
                  << " Orientation: [" << m_camera->orientation().w << ", "  << m_camera->orientation().x << ", " << m_camera->orientation().y << ", " << m_camera->orientation().z << "]\n"
                  << "*************************************n\n";
    }
}

void ClusteredShading::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);

    static float rotation_speed = 1.0f;
    static float time_accum     = 0.0f;
    
    if (m_animate_lights)
    {
        time_accum += delta_time * m_animation_speed;

        for (uint32_t i = 0; i < m_point_lights.size(); ++i)
        {
            m_point_lights[i].position.x = m_point_lights_ellipses_radii[i].x * glm::cos(time_accum * m_point_lights_ellipses_radii[i].z);
            m_point_lights[i].position.z = m_point_lights_ellipses_radii[i].y * glm::sin(time_accum * m_point_lights_ellipses_radii[i].z);
        }
    }
}

void ClusteredShading::GeneratePointLights()
{
    m_point_lights.clear();
    m_point_lights.resize(m_point_lights_count);

    m_point_lights_ellipses_radii.clear();
    m_point_lights_ellipses_radii.resize(m_point_lights_count);

    const float range_x = 11.0f;
    const float range_z = 6.0f;
    
    for(uint32_t i = 0; i < m_point_lights.size(); ++i)
    {
        auto& p = m_point_lights[i];
        auto& e = m_point_lights_ellipses_radii[i];

        float rand_x = glm::linearRand(-range_x, range_x);
        float rand_z = glm::linearRand(-range_z, range_z);

        p.color      = hsv2rgb(glm::linearRand(1.0f, 360.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f));
        p.intensity  = m_point_lights_intensity;
        p.position.y = glm::linearRand(0.5f, 12.0f);
        p.radius     = glm::linearRand(min_max_point_light_radius.x, min_max_point_light_radius.y);
        e            = glm::vec3(rand_x, rand_z, glm::linearRand(0.5f, 2.0f)); // [x, y, z] => [ellipse a radius, ellipse b radius, light move speed]
    }

    bool is_animate_state = m_animate_lights;
    m_animate_lights = true;
    update(0.01f);
    m_animate_lights = is_animate_state;
}

void ClusteredShading::HdrEquirectangularToCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt, const std::shared_ptr<Texture2D>& m_equirectangular_map)
{
    /* Update all faces per frame */
    m_equirectangular_to_cubemap_shader->bind();
    m_equirectangular_to_cubemap_shader->setUniform("u_projection", cubemap_rt->m_projection);

    glViewport(0, 0, cubemap_rt->m_width, cubemap_rt->m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, cubemap_rt->m_fbo_id);
    m_equirectangular_map->Bind(1);

    glBindVertexArray(m_skybox_vao);
    for (uint8_t side = 0; side < 6; ++side)
    {
        m_equirectangular_to_cubemap_shader->setUniform("u_view", cubemap_rt->m_view_transforms[side]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, cubemap_rt->m_cubemap_texture_id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glViewport(0, 0, Window::getWidth(), Window::getHeight());
}

void ClusteredShading::IrradianceConvolution(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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
    glViewport(0, 0, Window::getWidth(), Window::getHeight());
}

void ClusteredShading::PrefilterCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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
        //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width, mip_height);
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
    glViewport(0, 0, Window::getWidth(), Window::getHeight());
}

void ClusteredShading::PrecomputeIndirectLight(const std::filesystem::path& hdri_map_filepath)
{
    auto envmap_hdr = std::make_shared<Texture2D>();
    envmap_hdr->LoadHdr(hdri_map_filepath);

    HdrEquirectangularToCubemap(m_env_cubemap_rt, envmap_hdr);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_env_cubemap_rt->m_cubemap_texture_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    IrradianceConvolution(m_irradiance_cubemap_rt);
    PrefilterCubemap(m_prefiltered_env_map_rt);
}

void ClusteredShading::PrecomputeBRDF(const std::shared_ptr<Texture2DRenderTarget>& rt)
{
    GLuint m_dummy_vao_id;
    glCreateVertexArrays(1, &m_dummy_vao_id);

    rt->bindRenderTarget();
    m_precompute_brdf->bind();

    glBindVertexArray(m_dummy_vao_id);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDeleteVertexArrays(1, &m_dummy_vao_id);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Window::getWidth(), Window::getHeight());
}

void ClusteredShading::GenSkyboxGeometry()
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

void ClusteredShading::RenderScene()
{
    auto view_projection = m_camera->m_projection * m_camera->m_view;

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_cam_pos", m_camera->position());

    /* First, render the ambient color only for the opaque objects. */
    m_irradiance_cubemap_rt->bindTexture(6);
    m_prefiltered_env_map_rt->bindTexture(7);
    m_brdf_lut_rt->bindTexture(8);

    glBindTextures(0, 5, nullptr);
    //m_ambient_light_shader->setUniform("u_has_albedo_map",    m_static_objects[i].m_material->GetBool("u_has_albedo_map"));
    //m_ambient_light_shader->setUniform("u_has_normal_map",    m_static_objects[i].m_material->GetBool("u_has_normal_map"));
    //m_ambient_light_shader->setUniform("u_has_metallic_map",  m_static_objects[i].m_material->GetBool("u_has_metallic_map"));
    //m_ambient_light_shader->setUniform("u_has_roughness_map", m_static_objects[i].m_material->GetBool("u_has_roughness_map"));
    //m_ambient_light_shader->setUniform("u_has_ao_map",        m_static_objects[i].m_material->GetBool("u_has_ao_map"));
    //
    //m_ambient_light_shader->setUniform("u_albedo",    m_static_objects[i].m_material->GetVector3("u_albedo"));
    //m_ambient_light_shader->setUniform("u_metallic",  m_static_objects[i].m_material->GetFloat  ("u_metallic"));
    //m_ambient_light_shader->setUniform("u_roughness", m_static_objects[i].m_material->GetFloat  ("u_roughness"));
    //m_ambient_light_shader->setUniform("u_ao",        m_static_objects[i].m_material->GetFloat  ("u_ao"));
    //m_ambient_light_shader->setUniform("u_emission",  m_static_objects[i].m_material->GetVector3("u_emission"));

    m_ambient_light_shader->setUniform("u_z_near",       m_camera->NearPlane());
    m_ambient_light_shader->setUniform("u_z_far",        m_camera->FarPlane());
    m_ambient_light_shader->setUniform("u_slice_scale",  m_slice_scale);
    m_ambient_light_shader->setUniform("u_slice_bias",   m_slice_bias);
    m_ambient_light_shader->setUniform("u_debug_slices", m_debug_slices);

    m_ambient_light_shader->setUniform("u_model",         m_sponza_static_object.m_transform);
    m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_sponza_static_object.m_transform))));
    m_ambient_light_shader->setUniform("u_mvp",           view_projection * m_sponza_static_object.m_transform);

    m_sponza_static_object.m_model->Render(m_ambient_light_shader);

    /*
     * Disable writing to the depth buffer and additively
     * shade only those pixels, that were shaded in the ambient step.
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);

    /* Render point lights */
    m_point_light_shader->bind();
    m_point_light_shader->setUniform("u_cam_pos", m_camera->position());

    for (uint8_t p = 0; p < m_point_lights.size(); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",     m_point_lights[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity", m_point_lights[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",       m_point_lights[p].position);
        m_point_light_shader->setUniform("u_point_light.radius",         m_point_lights[p].radius);

        glBindTextures(0, 5, nullptr);
        //m_point_light_shader->setUniform("u_has_albedo_map",    m_static_objects[i].m_material->GetBool("u_has_albedo_map"));
        //m_point_light_shader->setUniform("u_has_normal_map",    m_static_objects[i].m_material->GetBool("u_has_normal_map"));
        //m_point_light_shader->setUniform("u_has_metallic_map",  m_static_objects[i].m_material->GetBool("u_has_metallic_map"));
        //m_point_light_shader->setUniform("u_has_roughness_map", m_static_objects[i].m_material->GetBool("u_has_roughness_map"));
        //
        //m_point_light_shader->setUniform("u_albedo",    m_static_objects[i].m_material->GetVector3("u_albedo"));
        //m_point_light_shader->setUniform("u_metallic",  m_static_objects[i].m_material->GetFloat  ("u_metallic"));
        //m_point_light_shader->setUniform("u_roughness", m_static_objects[i].m_material->GetFloat  ("u_roughness"));

        m_point_light_shader->setUniform("u_model",         m_sponza_static_object.m_transform);
        m_point_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_sponza_static_object.m_transform))));
        m_point_light_shader->setUniform("u_mvp",           view_projection * m_sponza_static_object.m_transform);

        m_sponza_static_object.m_model->Render(m_point_light_shader);
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void ClusteredShading::render()
{
    /* Put render specific code here. Don't update variables here! */
    m_tmo_ps->bindFilterFBO();

    RenderScene();

    m_background_shader->bind();
    m_background_shader->setUniform("u_projection", m_camera->m_projection);
    m_background_shader->setUniform("u_view", glm::mat4(glm::mat3(m_camera->m_view)));
    m_background_shader->setUniform("u_lod_level", m_background_lod_level);
    m_env_cubemap_rt->bindTexture();

    glBindVertexArray(m_skybox_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    /* Bloom: downscale */
    if(m_bloom_enabled)
    {
        m_downscale_shader->bind();
        m_downscale_shader->setUniform("u_threshold", glm::vec4(m_threshold, m_threshold - m_knee, 2.0f * m_knee, 0.25f * m_knee));
        m_tmo_ps->rt->bindTexture();

        glm::uvec2 mip_size = glm::uvec2(m_tmo_ps->rt->m_width / 2, m_tmo_ps->rt->m_height / 2);

        for (uint8_t i = 0; i < m_tmo_ps->rt->m_mip_levels - 1; ++i)
        {
            m_downscale_shader->setUniform("u_texel_size",    1.0f / glm::vec2(mip_size));
            m_downscale_shader->setUniform("u_mip_level",     i);
            m_downscale_shader->setUniform("u_use_threshold", i == 0);

            m_tmo_ps->rt->bindImageForWrite(IMAGE_UNIT_WRITE, i + 1);

            glDispatchCompute(glm::ceil(float(mip_size.x) / 8), glm::ceil(float(mip_size.y) / 8), 1);

            mip_size = mip_size / 2u;

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        /* Bloom: upscale */
        m_upscale_shader->bind();
        m_upscale_shader->setUniform("u_bloom_intensity", m_bloom_intensity);
        m_upscale_shader->setUniform("u_dirt_intensity",  m_bloom_dirt_intensity);
        m_tmo_ps->rt->bindTexture();
        m_bloom_dirt_texture->Bind(1);

        for (uint8_t i = m_tmo_ps->rt->m_mip_levels - 1; i >= 1; --i)
        {
            mip_size.x = glm::max(1.0, glm::floor(float(m_tmo_ps->rt->m_width)  / glm::pow(2.0, i - 1)));
            mip_size.y = glm::max(1.0, glm::floor(float(m_tmo_ps->rt->m_height) / glm::pow(2.0, i - 1)));

            m_upscale_shader->setUniform("u_texel_size", 1.0f / glm::vec2(mip_size));
            m_upscale_shader->setUniform("u_mip_level",  i);

            m_tmo_ps->rt->bindImageForReadWrite(IMAGE_UNIT_WRITE, i - 1);

            glDispatchCompute(glm::ceil(float(mip_size.x) / 8), glm::ceil(float(mip_size.y) / 8), 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }
    }
    m_tmo_ps->render(m_exposure, m_gamma);
}

void ClusteredShading::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos       = ImVec2(Window::getWidth() - 10.0, 10.0);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 400, 0 });

    ImGui::Begin("Settings");
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

        if (ImGui::CollapsingHeader("Camera Info"))
        {
            glm::vec3 cam_pos = m_camera->position();
            glm::vec3 cam_dir = m_camera->direction();
            float     cam_fov = m_camera->FOV();

            ImGui::Text("Position  : [%.2f, %.2f, %.2f]\n"
                        "Direction : [%.2f, %.2f, %.2f]\n"
                        "FoV       : % .2f", 
                         cam_pos.x, cam_pos.y, cam_pos.z, 
                         cam_dir.x, cam_dir.y, cam_dir.z, 
                         cam_fov);
        }

        if (ImGui::CollapsingHeader("Lights Generator", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

            ImGui::Checkbox   ("Show Debug Z Tiles",                         &m_debug_slices);
            ImGui::Checkbox   ("Animate Lights",                             &m_animate_lights);
            ImGui::SliderFloat("Animation Speed",                            &m_animation_speed, 0.0f, 2.0f, "%.2f");
            ImGui::InputScalar("Point Lights Count",      ImGuiDataType_U32, &m_point_lights_count);

            if (ImGui::InputFloat("Min Point Lights Radius", &min_max_point_light_radius.x, 0.0f, 0.0f, "%.0f"))
            {
                if (min_max_point_light_radius.x < 0.0)
                {
                    min_max_point_light_radius.x = 0.0f;
                }
            }

            if (ImGui::InputFloat ("Max Point Lights Radius", &min_max_point_light_radius.y, 0.0f, 0.0f, "%.0f"))
            {
                if (min_max_point_light_radius.y < 0.0)
                {
                    min_max_point_light_radius.y = 0.0f;
                }
            }

            ImGui::SliderFloat("Point Lights Intensity", &m_point_lights_intensity, 0.0f, 10.0f, "%.2f");

            if (ImGui::Button("Generate Lights"))
            {
                GeneratePointLights();
            }

            ImGui::PopItemWidth();
        }

        if (ImGui::CollapsingHeader("Tonemapper"))
        {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
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
                        m_current_hdr_map_idx = i;
                        PrecomputeIndirectLight(FileSystem::getPath("textures/skyboxes/IBL/" + m_hdr_maps_names[m_current_hdr_map_idx]));
                    }

                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }

        if (ImGui::CollapsingHeader("Bloom"))
        {
            ImGui::Checkbox   ("Bloom enabled",        &m_bloom_enabled);
            ImGui::SliderFloat("Bloom threshold",      &m_threshold,            0.0f, 15.0f, "%.1f");
            ImGui::SliderFloat("Bloom knee",           &m_knee,                 0.0f, 1.0f,  "%.1f");
            ImGui::SliderFloat("Bloom intensity",      &m_bloom_intensity,      0.0f, 5.0f,  "%.1f");
            ImGui::SliderFloat("Bloom dirt intensity", &m_bloom_dirt_intensity, 0.0f, 10.0f, "%.1f");
        }

    }
    ImGui::End();
}
