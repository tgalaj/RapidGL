#include "clustered_shading.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/random.hpp>

#define IMAGE_UNIT_WRITE 0

using namespace RGL;

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

    glDeleteBuffers(1, &m_clusters_ssbo);
    glDeleteBuffers(1, &m_cull_lights_dispatch_args_ssbo);
    glDeleteBuffers(1, &m_directional_lights_ssbo);
    glDeleteBuffers(1, &m_point_lights_ssbo);
    glDeleteBuffers(1, &m_spot_lights_ssbo);
    glDeleteBuffers(1, &m_point_lights_ellipses_radii_ssbo);
    glDeleteBuffers(1, &m_spot_lights_ellipses_radii_ssbo);
    glDeleteBuffers(1, &m_clusters_flags_ssbo);
    glDeleteBuffers(1, &m_point_light_index_list_ssbo);
    glDeleteBuffers(1, &m_spot_light_index_list_ssbo);
    glDeleteBuffers(1, &m_point_light_grid_ssbo);
    glDeleteBuffers(1, &m_spot_light_grid_ssbo);
    glDeleteBuffers(1, &m_unique_active_clusters_ssbo);

    glDeleteTextures(1, &m_depth_tex2D_id);
    glDeleteFramebuffers(1, &m_depth_pass_fbo_id);
}

void ClusteredShading::init_app()
{
    /// Initialize all the variables, buffers, etc. here.
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /* Create virtual camera. */
    m_camera = std::make_shared<Camera>(45.0, Window::getAspectRatio(), 0.01, 300.0);
    m_camera->setPosition(-8.32222, 1.9269, -0.768721);
    m_camera->setOrientation(glm::quat(0.634325, 0.0407623, 0.772209, 0.0543523));
   
    /// Init clustered shading variables.
    float z_near       = m_camera->NearPlane();
    float z_far        = m_camera->FarPlane();
    float half_fov     = glm::radians(m_camera->FOV() * 0.5f);

    m_cluster_grid_dim.x = uint32_t(glm::ceil(RGL::Window::getWidth()  / float(m_cluster_grid_block_size)));
    m_cluster_grid_dim.y = uint32_t(glm::ceil(RGL::Window::getHeight() / float(m_cluster_grid_block_size)));

    // The depth of the cluster grid during clustered rendering is dependent on the 
    // number of clusters subdivisions in the screen Y direction.
    // Source: Clustered Deferred and Forward Shading (2012) (Ola Olsson, Markus Billeter, Ulf Assarsson).
    float sD         = 2.0f * glm::tan(half_fov) / float(m_cluster_grid_dim.y);
          m_near_k   = 1.0f + sD;
    m_log_grid_dim_y = 1.0f / glm::log(m_near_k);

    float log_depth      = glm::log(z_far / z_near);
    m_cluster_grid_dim.z = uint32_t(glm::floor(log_depth * m_log_grid_dim_y));

    m_clusters_count = m_cluster_grid_dim.x * m_cluster_grid_dim.y * m_cluster_grid_dim.z;

    /// Randomly initialize lights
    srand(3281991);
    GeneratePointLights();
    GenerateSpotLights();

    /// Create Sponza static object
    auto sponza_model = std::make_shared<StaticModel>();
    sponza_model->Load(RGL::FileSystem::getResourcesPath() / "models/sponza/Sponza.gltf");

    glm::mat4 world_trans  = glm::mat4(1.0f);
              world_trans  = glm::scale(world_trans, glm::vec3(sponza_model->GetUnitScaleFactor() * 30.0f));
    m_sponza_static_object = StaticObject(sponza_model, world_trans);

    /// Prepare lights' SSBOs.
    glCreateBuffers  (1, &m_directional_lights_ssbo);
    glNamedBufferData(m_directional_lights_ssbo, sizeof(DirectionalLight) * m_directional_lights.size(), m_directional_lights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, DIRECTIONAL_LIGHTS_SSBO_BINDING_INDEX, m_directional_lights_ssbo);

    glCreateBuffers  (1, &m_point_lights_ssbo);
    glNamedBufferData(m_point_lights_ssbo, sizeof(PointLight) * m_point_lights.size(), m_point_lights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, POINT_LIGHTS_SSBO_BINDING_INDEX, m_point_lights_ssbo);

    glCreateBuffers  (1, &m_spot_lights_ssbo);
    glNamedBufferData(m_spot_lights_ssbo, sizeof(SpotLight) * m_spot_lights.size(), m_spot_lights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, SPOT_LIGHTS_SSBO_BINDING_INDEX, m_spot_lights_ssbo);

    glCreateBuffers  (1, &m_point_lights_ellipses_radii_ssbo);
    glNamedBufferData(m_point_lights_ellipses_radii_ssbo, sizeof(m_point_lights_ellipses_radii[0]) * m_point_lights_ellipses_radii.size(), m_point_lights_ellipses_radii.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, POINT_LIGHTS_ELLIPSES_RADII_SSBO_BINDING_INDEX, m_point_lights_ellipses_radii_ssbo);

    glCreateBuffers  (1, &m_spot_lights_ellipses_radii_ssbo);
    glNamedBufferData(m_spot_lights_ellipses_radii_ssbo, sizeof(m_spot_lights_ellipses_radii[0]) * m_spot_lights_ellipses_radii.size(), m_spot_lights_ellipses_radii.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, SPOT_LIGHTS_ELLIPSES_RADII_SSBO_BINDING_INDEX, m_spot_lights_ellipses_radii_ssbo);

    /// Prepare SSBOs related to the clustering (light-culling) algorithm.
    // Stores the screen-space clusters 
    glCreateBuffers  (1, &m_clusters_ssbo);
    glNamedBufferData(m_clusters_ssbo, sizeof(ClusterAABB) * m_clusters_count, nullptr, GL_STATIC_READ);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, CLUSTERS_SSBO_BINDING_INDEX, m_clusters_ssbo);

    // Create a buffer to hold (boolean) flags in the cluster grid that contain samples.
    glCreateBuffers  (1, &m_clusters_flags_ssbo);
    glNamedBufferData(m_clusters_flags_ssbo, sizeof(uint32_t) * m_clusters_count, nullptr, GL_STATIC_READ);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, CLUSTERS_FLAGS_SSBO_BINDING_INDEX, m_clusters_flags_ssbo);

    // A buffer (and internal counter) that holds a list of the unique clusters (the clusters that are visible and actually contain a sample).
    glCreateBuffers  (1, &m_unique_active_clusters_ssbo);
    glNamedBufferData(m_unique_active_clusters_ssbo, sizeof(uint32_t) * m_clusters_count + sizeof(uint32_t), nullptr, GL_STATIC_READ);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, UNIQUE_ACTIVE_CLUSTERS_SSBO_BINDING_INDEX, m_unique_active_clusters_ssbo);

    // A buffer that stores number of work groups to be dispatched by cull lights shader
    glCreateBuffers  (1, &m_cull_lights_dispatch_args_ssbo);
    glNamedBufferData(m_cull_lights_dispatch_args_ssbo, sizeof(uint32_t) * 3, nullptr, GL_STATIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, CULL_LIGHTS_DISPATCH_ARGS_SSBO_BINDING_INDEX, m_cull_lights_dispatch_args_ssbo);

    // A list of indices to the lights that are active and intersect with a cluster
    glCreateBuffers  (1, &m_point_light_index_list_ssbo);
    glNamedBufferData(m_point_light_index_list_ssbo, sizeof(uint32_t) * m_clusters_count * AVERAGE_OVERLAPPING_LIGHTS_PER_CLUSTER, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, POINT_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX, m_point_light_index_list_ssbo);

    glCreateBuffers  (1, &m_spot_light_index_list_ssbo);
    glNamedBufferData(m_spot_light_index_list_ssbo, sizeof(uint32_t) * m_clusters_count * AVERAGE_OVERLAPPING_LIGHTS_PER_CLUSTER, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, SPOT_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX, m_spot_light_index_list_ssbo);

    // Every tile takes LightGrid struct that has two unsigned ints one to represent the number of lights in that grid
    // Another to represent the offset to the light index list from where to begin reading light indexes from
    // In this SSBO, atomic counter is also being stored (uint global_index_count)
    // This implementation is straight up from Olsson paper
    glCreateBuffers  (1, &m_point_light_grid_ssbo);
    glNamedBufferData(m_point_light_grid_ssbo, sizeof(uint32_t) + sizeof(LightGrid) * m_clusters_count, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, POINT_LIGHT_GRID_SSBO_BINDING_INDEX, m_point_light_grid_ssbo);

    glCreateBuffers  (1, &m_spot_light_grid_ssbo);
    glNamedBufferData(m_spot_light_grid_ssbo, sizeof(uint32_t) + sizeof(LightGrid) * m_clusters_count, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase (GL_SHADER_STORAGE_BUFFER, SPOT_LIGHT_GRID_SSBO_BINDING_INDEX, m_spot_light_grid_ssbo);

    // Create depth pre-pass texture and FBO
    glCreateTextures  (GL_TEXTURE_2D, 1, &m_depth_tex2D_id);
    glTextureStorage2D(m_depth_tex2D_id, 1, GL_DEPTH_COMPONENT32F, RGL::Window::getWidth(), RGL::Window::getHeight());

    glTextureParameteri(m_depth_tex2D_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_depth_tex2D_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_depth_tex2D_id, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depth_tex2D_id, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glCreateFramebuffers     (1, &m_depth_pass_fbo_id);
    glNamedFramebufferTexture(m_depth_pass_fbo_id, GL_DEPTH_ATTACHMENT, m_depth_tex2D_id, 0);

    GLenum draw_buffers[] = { GL_NONE };
    glNamedFramebufferDrawBuffers(m_depth_pass_fbo_id, 1, draw_buffers);

    /// Create shaders.
    std::string dir = "src/demos/27_clustered_shading/";
    m_depth_prepass_shader = std::make_shared<Shader>(dir + "depth_pass.vert", dir + "depth_pass.frag");
    m_depth_prepass_shader->link();

    m_generate_clusters_shader = std::make_shared<Shader>(dir + "generate_clusters.comp");
    m_generate_clusters_shader->link();

    m_find_visible_clusters_shader = std::make_shared<Shader>(dir + "find_visible_clusters.comp");
    m_find_visible_clusters_shader->link();

    m_find_unique_clusters_shader = std::make_shared<Shader>(dir + "find_unique_clusters.comp");
    m_find_unique_clusters_shader->link();

    m_update_cull_lights_indirect_args_shader = std::make_shared<Shader>(dir + "update_cull_lights_indirect_args.comp");
    m_update_cull_lights_indirect_args_shader->link();

    m_cull_lights_shader = std::make_shared<Shader>(dir + "cull_lights.comp");
    m_cull_lights_shader->link();

    m_clustered_pbr_shader = std::make_shared<Shader>(dir + "pbr_lighting.vert", dir + "pbr_clustered.frag");
    m_clustered_pbr_shader->link();

    m_update_lights_shader = std::make_shared<Shader>(dir + "update_lights.comp");
    m_update_lights_shader->link();

    dir = "src/demos/22_pbr/";
    m_equirectangular_to_cubemap_shader = std::make_shared<Shader>(dir + "cubemap.vert", dir + "equirectangular_to_cubemap.frag");
    m_equirectangular_to_cubemap_shader->link();

    m_irradiance_convolution_shader = std::make_shared<Shader>(dir + "cubemap.vert", dir + "irradiance_convolution.frag");
    m_irradiance_convolution_shader->link();
    
    m_prefilter_env_map_shader = std::make_shared<Shader>(dir + "cubemap.vert", dir + "prefilter_cubemap.frag");
    m_prefilter_env_map_shader->link();

    m_precompute_brdf = std::make_shared<Shader>("src/demos/10_postprocessing_filters/FSQ.vert", dir + "precompute_brdf.frag");
    m_precompute_brdf->link();

    m_background_shader = std::make_shared<Shader>(dir + "background.vert", dir + "background.frag");
    m_background_shader->link();

    m_tmo_ps = std::make_shared<PostprocessFilter>(Window::getWidth(), Window::getHeight());

    // Bloom shaders.
    dir = "src/demos/26_bloom/";
    m_downscale_shader = std::make_shared<Shader>(dir + "downscale.comp");
    m_downscale_shader->link();

    m_upscale_shader = std::make_shared<Shader>(dir + "upscale.comp");
    m_upscale_shader->link();

    m_bloom_dirt_texture = std::make_shared<Texture2D>(); 
    m_bloom_dirt_texture->Load(FileSystem::getResourcesPath() / "textures/bloom_dirt_mask.png");

    // IBL precomputations.
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

    PrecomputeIndirectLight(FileSystem::getResourcesPath() / "textures/skyboxes/IBL" / m_hdr_maps_names[m_current_hdr_map_idx]);
    PrecomputeBRDF(m_brdf_lut_rt);

    auto proj = m_camera->m_projection;
    auto inv_proj = glm::inverse(proj);

    /// Generate clusters' AABBs
    // This can be done once as long as the camera parameters doesn't change (projection matrix related variables)
    m_generate_clusters_shader->bind();
    m_generate_clusters_shader->setUniform("u_grid_dim",           m_cluster_grid_dim);
    m_generate_clusters_shader->setUniform("u_cluster_size_ss",    glm::uvec2(m_cluster_grid_block_size));
    m_generate_clusters_shader->setUniform("u_near_k",             m_near_k);
    m_generate_clusters_shader->setUniform("u_near_z",             m_camera->NearPlane());
    m_generate_clusters_shader->setUniform("u_inverse_projection", glm::inverse(m_camera->m_projection));
    m_generate_clusters_shader->setUniform("u_pixel_size",         1.0f / glm::vec2(RGL::Window::getWidth(), RGL::Window::getHeight()));
    glDispatchCompute(glm::ceil(float(m_clusters_count) / 1024.0f), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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
            std::cout << "Saved " << filename << ".png to " << (FileSystem::getRootPath() / "screenshots/") << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << (FileSystem::getRootPath() / "screenshots/") << std::endl;
        }
    }

    if (Input::getKeyUp(KeyCode::F3))
    {
        std::cout << "******** Camera properties : ********\n"
                  << " Position:    [" << m_camera->position().x << ", " << m_camera->position().y << ", " << m_camera->position().z << "]\n"
                  << " Orientation: [" << m_camera->orientation().w << ", "  << m_camera->orientation().x << ", " << m_camera->orientation().y << ", " << m_camera->orientation().z << "]\n"
                  << "*************************************n\n";
    }

    if (Input::getKeyUp(KeyCode::Space))
    {
        m_animate_lights = !m_animate_lights;
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

        m_update_lights_shader->bind();
        m_update_lights_shader->setUniform("u_time", time_accum);

        uint32_t max_lights_count = glm::max(m_point_lights_count, glm::max(m_spot_lights_count, m_directional_lights_count));
        glDispatchCompute(std::ceilf(max_lights_count / 1024.0f), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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

        p.base.color     = hsv2rgb(glm::linearRand(1.0f, 360.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f));
        p.base.intensity = m_point_lights_intensity;
        p.position.y     = glm::linearRand(0.5f, 12.0f);
        p.radius         = glm::linearRand(min_max_point_light_radius.x, min_max_point_light_radius.y);
        e                = glm::vec4(rand_x, rand_z, glm::linearRand(0.5f, 2.0f), 0.0f); // [x, y, z] => [ellipse a radius, ellipse b radius, light move speed]

        p.position.x = e.x * glm::cos(1.618f * e.z);
        p.position.z = e.y * glm::sin(1.618f * e.z);
    }
}

void ClusteredShading::GenerateSpotLights()
{
    m_spot_lights.clear();
    m_spot_lights.resize(m_spot_lights_count);

    m_spot_lights_ellipses_radii.clear();
    m_spot_lights_ellipses_radii.resize(m_spot_lights_count);

    const float range_x = 11.0f;
    const float range_z = 6.0f;
    
    for(uint32_t i = 0; i < m_spot_lights.size(); ++i)
    {
        auto& p = m_spot_lights[i];
        auto& e = m_spot_lights_ellipses_radii[i];

        float rand_x = glm::linearRand(-range_x, range_x);
        float rand_z = glm::linearRand(-range_z, range_z);

        setLightDirection(p.direction, glm::linearRand(0.0f, 360.0f), glm::linearRand(0.0f, 70.0f));
        p.outer_angle          = glm::radians(15.0f);
        p.inner_angle          = glm::radians(10.0f);
        p.point.base.color     = hsv2rgb(glm::linearRand(1.0f, 360.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f));
        p.point.base.intensity = m_spot_lights_intensity;
        p.point.position.y     = glm::linearRand(0.5f, 12.0f);
        p.point.radius         = glm::linearRand(min_max_spot_light_radius.x, min_max_spot_light_radius.y);
        e                      = glm::vec4(rand_x, rand_z, glm::linearRand(0.5f, 2.0f), 0.0f); // [x, y, z] => [ellipse a radius, ellipse b radius, light move speed]

        p.point.position.x = e.x * glm::cos(1.618f * e.z);
        p.point.position.z = e.y * glm::sin(1.618f * e.z);
    }
}

void ClusteredShading::UpdateLightsSSBOs()
{
    glNamedBufferData(m_directional_lights_ssbo,          sizeof(DirectionalLight)                 * m_directional_lights.size(),          m_directional_lights.data(),          GL_DYNAMIC_DRAW);
    glNamedBufferData(m_point_lights_ssbo,                sizeof(PointLight)                       * m_point_lights.size(),                m_point_lights.data(),                GL_DYNAMIC_DRAW);
    glNamedBufferData(m_spot_lights_ssbo,                 sizeof(SpotLight)                        * m_spot_lights.size(),                 m_spot_lights.data(),                 GL_DYNAMIC_DRAW);
    glNamedBufferData(m_point_lights_ellipses_radii_ssbo, sizeof(m_point_lights_ellipses_radii[0]) * m_point_lights_ellipses_radii.size(), m_point_lights_ellipses_radii.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(m_spot_lights_ellipses_radii_ssbo,  sizeof(m_spot_lights_ellipses_radii[0])  * m_spot_lights_ellipses_radii.size(),  m_spot_lights_ellipses_radii.data(),  GL_DYNAMIC_DRAW);
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

void ClusteredShading::render()
{
    // 1. Depth(Z) pre-pass
    renderDepthPass();

    // 2. Blit depth info to tmo_ps framebuffer
    glBlitNamedFramebuffer(m_depth_pass_fbo_id, m_tmo_ps->rt->m_fbo_id, 
                           0, 0, Window::getWidth(), Window::getHeight(),
                           0, 0, Window::getWidth(), Window::getHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    
    static const uint32_t clear_val = 0;
    
    // 3. Find visible clusters
    glClearNamedBufferData(m_clusters_flags_ssbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_val);

    m_find_visible_clusters_shader->bind();
    m_find_visible_clusters_shader->setUniform("u_near_z",          m_camera->NearPlane());
    m_find_visible_clusters_shader->setUniform("u_far_z",           m_camera->FarPlane());
    m_find_visible_clusters_shader->setUniform("u_log_grid_dim_y",  m_log_grid_dim_y);
    m_find_visible_clusters_shader->setUniform("u_cluster_size_ss", glm::uvec2(m_cluster_grid_block_size));
    m_find_visible_clusters_shader->setUniform("u_grid_dim",        m_cluster_grid_dim);
    
    glBindTextureUnit(0, m_depth_tex2D_id);
    glDispatchCompute(glm::ceil(RGL::Window::getWidth() / 32.0f), glm::ceil(RGL::Window::getHeight() / 32.0f), 1);
    glMemoryBarrier  (GL_SHADER_STORAGE_BARRIER_BIT);

    // 4. Find unique clusters and update the indirect dispatch arguments buffer
    glClearNamedBufferData(m_unique_active_clusters_ssbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_val);

    m_find_unique_clusters_shader->bind();
    glDispatchCompute(glm::ceil(m_clusters_count / 1024.0f), 1, 1);
    glMemoryBarrier  (GL_SHADER_STORAGE_BARRIER_BIT);

    m_update_cull_lights_indirect_args_shader->bind();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier  (GL_SHADER_STORAGE_BARRIER_BIT);

    // 5. Assign lights to clusters (cull lights)
    glClearNamedBufferData(m_point_light_grid_ssbo,       GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_val);
    glClearNamedBufferData(m_point_light_index_list_ssbo, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_val);
    glClearNamedBufferData(m_spot_light_grid_ssbo,        GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_val);
    glClearNamedBufferData(m_spot_light_index_list_ssbo,  GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &clear_val);

    m_cull_lights_shader->bind();
    m_cull_lights_shader->setUniform("u_view_matrix", m_camera->m_view);

    glBindBuffer             (GL_DISPATCH_INDIRECT_BUFFER, m_cull_lights_dispatch_args_ssbo);
    glDispatchComputeIndirect(NULL);
    glMemoryBarrier          (GL_SHADER_STORAGE_BARRIER_BIT);

    // 6. Render lighting
    m_tmo_ps->bindFilterFBO(GL_COLOR_BUFFER_BIT);
    renderLighting();

    // 7. Render skybox
    m_background_shader->bind();
    m_background_shader->setUniform("u_projection", m_camera->m_projection);
    m_background_shader->setUniform("u_view",       glm::mat4(glm::mat3(m_camera->m_view)));
    m_background_shader->setUniform("u_lod_level",  m_background_lod_level);
    m_env_cubemap_rt->bindTexture();

    glBindVertexArray(m_skybox_vao);
    glDrawArrays     (GL_TRIANGLES, 0, 36);

    // 8. Bloom: downscale
    if (m_bloom_enabled)
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

    // 9. Apply tone mapping
    m_tmo_ps->render(m_exposure, m_gamma);
}

void ClusteredShading::renderDepthPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_depth_pass_fbo_id);
    glClear          (GL_DEPTH_BUFFER_BIT);

    glDepthMask(1);
    glColorMask(0, 0, 0, 0);
    glDepthFunc(GL_LESS);

    m_depth_prepass_shader->bind();
    m_depth_prepass_shader->setUniform("mvp", m_camera->m_projection * m_camera->m_view * m_sponza_static_object.m_transform);
    m_sponza_static_object.m_model->Render();
}

void ClusteredShading::renderLighting()
{
    glDepthMask(0);
    glColorMask(1, 1, 1, 1);
    glDepthFunc(GL_EQUAL);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    m_clustered_pbr_shader->bind();
    m_clustered_pbr_shader->setUniform("u_cam_pos",         m_camera->position());
    m_clustered_pbr_shader->setUniform("u_near_z",          m_camera->NearPlane());
    m_clustered_pbr_shader->setUniform("u_grid_dim",        m_cluster_grid_dim);
    m_clustered_pbr_shader->setUniform("u_cluster_size_ss", glm::uvec2(m_cluster_grid_block_size));
    m_clustered_pbr_shader->setUniform("u_log_grid_dim_y",  m_log_grid_dim_y);
    m_clustered_pbr_shader->setUniform("u_debug_slices",    m_debug_slices);

    m_clustered_pbr_shader->setUniform("u_model",         m_sponza_static_object.m_transform);
    m_clustered_pbr_shader->setUniform("u_view",          m_camera->m_view);
    m_clustered_pbr_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_sponza_static_object.m_transform))));
    m_clustered_pbr_shader->setUniform("u_mvp",           view_projection * m_sponza_static_object.m_transform);

    m_irradiance_cubemap_rt->bindTexture(6);
    m_prefiltered_env_map_rt->bindTexture(7);
    m_brdf_lut_rt->bindTexture(8);

    m_sponza_static_object.m_model->Render(m_clustered_pbr_shader);

    /* Enable writing to the depth buffer. */
    glDepthMask(1);
    glDepthFunc(GL_LEQUAL);
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
            ImGui::SliderFloat("Animation Speed",                            &m_animation_speed, 0.0f, 15.0f, "%.1f");
            ImGui::InputScalar("Point Lights Count",      ImGuiDataType_U32, &m_point_lights_count);

            if (ImGui::InputFloat("Min Point Lights Radius", &min_max_point_light_radius.x, 0.0f, 0.0f, "%.2f"))
            {
                if (min_max_point_light_radius.x < 0.0)
                {
                    min_max_point_light_radius.x = 0.0f;
                }
            }

            if (ImGui::InputFloat ("Max Point Lights Radius", &min_max_point_light_radius.y, 0.0f, 0.0f, "%.2f"))
            {
                if (min_max_point_light_radius.y < 0.0)
                {
                    min_max_point_light_radius.y = 0.0f;
                }
            }
            ImGui::SliderFloat("Point Lights Intensity", &m_point_lights_intensity, 0.0f, 10.0f, "%.2f");

            ImGui::Separator();
            ImGui::InputScalar("Spot Lights Count", ImGuiDataType_U32, &m_spot_lights_count);

            if (ImGui::InputFloat("Min Spot Lights Radius", &min_max_spot_light_radius.x, 0.0f, 0.0f, "%.2f"))
            {
                if (min_max_spot_light_radius.x < 0.0)
                {
                    min_max_spot_light_radius.x = 0.0f;
                }
            }

            if (ImGui::InputFloat("Max Spot Lights Radius", &min_max_spot_light_radius.y, 0.0f, 0.0f, "%.2f"))
            {
                if (min_max_spot_light_radius.y < 0.0)
                {
                    min_max_spot_light_radius.y = 0.0f;
                }
            }

            ImGui::SliderFloat("Spot Lights Intensity", &m_spot_lights_intensity, 0.0f, 10.0f, "%.2f");

            if (ImGui::Button("Generate Lights"))
            {
                GeneratePointLights();
                GenerateSpotLights();
                UpdateLightsSSBOs();
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
                        PrecomputeIndirectLight(FileSystem::getResourcesPath() / "textures/skyboxes/IBL" / m_hdr_maps_names[m_current_hdr_map_idx]);
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
