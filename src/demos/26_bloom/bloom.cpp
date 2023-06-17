#include "bloom.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

#define IMAGE_UNIT_WRITE 0

Bloom::Bloom()
      : m_exposure            (1.0f),
        m_gamma               (3.6f),
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

Bloom::~Bloom()
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
}

void Bloom::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-8.32222, 1.9269, -0.768721);
    m_camera->setOrientation(glm::quat(0.634325, 0.0407623, 0.772209, 0.0543523));
   
    /* Initialize lights' properties */
    m_point_lights_properties[0].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_lights_properties[0].intensity = 5.0f;
    m_point_lights_properties[0].position  = glm::vec3(0.0, 0.5, 1.5);
    m_point_lights_properties[0].radius    = 10.0f; 

    m_point_lights_properties[1].color     = glm::vec3(1.0, 0.01, 0.01);
    m_point_lights_properties[1].intensity = 10.0f;
    m_point_lights_properties[1].position  = glm::vec3(-4.0, 0.5, -3.0);
    m_point_lights_properties[1].radius     = 10.0f; 

    m_point_lights_properties[2].color     = glm::vec3(0.006, 0.006, 1.0);
    m_point_lights_properties[2].intensity = 15.0f;
    m_point_lights_properties[2].position  = glm::vec3(3.0, 0.5, 1.0);
    m_point_lights_properties[2].radius     = 10.0f; 

    m_point_lights_properties[3].color     = glm::vec3(0.02, 1.0, 0.02);
    m_point_lights_properties[3].intensity = 5.0f;
    m_point_lights_properties[3].position  = glm::vec3(-0.8, 2.4, -1.0);
    m_point_lights_properties[3].radius     = 10.0f; 

    /* Create materials */
    Material crate_material;
    crate_material.m_has_albedo_map    = true;
    crate_material.m_has_metallic_map  = true;
    crate_material.m_has_roughness_map = true;
    crate_material.m_has_normal_map    = true;
    crate_material.m_ao                = 0.98f;

    Material floor_material;
    floor_material.m_has_albedo_map    = true;
    floor_material.m_has_ao_map        = true;
    floor_material.m_has_normal_map    = true;
    floor_material.m_has_roughness_map = true;

    /* Create models. */
    auto crate_model = std::make_shared<RGL::StaticModel>();
    crate_model->Load(RGL::FileSystem::getResourcesPath() / "models/old_crate/old_crate.obj");

    auto plane_model = std::make_shared<RGL::StaticModel>();
    plane_model->GenCube(1.0f, 4.0f);

    auto box_model = std::make_shared<RGL::StaticModel>();
    box_model->GenCube(1.0f);

    auto crate_model_scale = glm::scale(glm::mat4(1.0f), glm::vec3(crate_model->GetUnitScaleFactor()));
    auto crate_scale_factor = 2.0f * crate_model->GetUnitScaleFactor();

    /* Create world transform for  the plane. */
    m_static_objects.reserve(15); // Reserving enough space, to keep track of the Static Objects' references

    glm::mat4 world_trans = glm::mat4(1.0f);
              world_trans = glm::translate(world_trans, glm::vec3(0.0f, -1.0f, 0.0));
              world_trans = glm::scale(world_trans, glm::vec3(12.5f, 0.5f, 12.5f));
    m_static_objects.push_back(StaticObject(plane_model, world_trans, floor_material));

    /* Create world transforms for the boxes. */
    world_trans = glm::mat4(1.0f);
    world_trans = glm::translate(world_trans, glm::vec3(0.0f, 1.5f, 0.0));
    world_trans = glm::scale(world_trans, glm::vec3(0.5f) * crate_scale_factor);
    m_static_objects.push_back(StaticObject(crate_model, world_trans, crate_material));

    world_trans = glm::mat4(1.0f);
    world_trans = glm::translate(world_trans, glm::vec3(2.0f, 0.0f, 1.0));
    world_trans = glm::scale(world_trans, glm::vec3(0.5f) * crate_scale_factor);
    m_static_objects.push_back(StaticObject(crate_model, world_trans, crate_material));

    world_trans = glm::mat4(1.0f);
    world_trans = glm::translate(world_trans, glm::vec3(-1.0f, -1.0f, 2.0));
    world_trans = glm::rotate(world_trans, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    world_trans = glm::scale(world_trans, glm::vec3(crate_scale_factor));
    m_static_objects.push_back(StaticObject(crate_model, world_trans, crate_material));

    world_trans = glm::mat4(1.0f);
    world_trans = glm::translate(world_trans, glm::vec3(0.0f, 2.7f, 4.0));
    world_trans = glm::rotate(world_trans, glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    world_trans = glm::scale(world_trans, glm::vec3(1.25) * crate_scale_factor);
    m_static_objects.push_back(StaticObject(crate_model, world_trans, crate_material));

    world_trans = glm::mat4(1.0f);
    world_trans = glm::translate(world_trans, glm::vec3(-2.0f, 1.0f, -3.0));
    world_trans = glm::rotate(world_trans, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    world_trans = glm::scale(world_trans, glm::vec3(crate_scale_factor));
    m_static_objects.push_back(StaticObject(crate_model, world_trans, crate_material));

    world_trans = glm::mat4(1.0f);
    world_trans = glm::translate(world_trans, glm::vec3(-3.0f, 0.0f, 0.0));
    world_trans = glm::scale(world_trans, glm::vec3(0.5f) * crate_scale_factor);
    m_static_objects.push_back(StaticObject(crate_model, world_trans, crate_material));

    /* Create world transforms for the boxes that will visualize the point lights. */
    for (unsigned int i = 0; i < std::size(m_point_lights_properties); i++)
    {
        world_trans = glm::mat4(1.0f);
        world_trans = glm::translate(world_trans, m_point_lights_properties[i].position);
        world_trans = glm::scale(world_trans, glm::vec3(0.25f));
        m_static_objects.push_back(StaticObject(box_model, world_trans));

        /* Assign light color to the emission color. */
        m_static_objects[m_static_objects.size() - 1].m_material.m_emission = m_point_lights_properties[i].color * m_point_lights_properties[i].intensity;
        m_light_boxes[i] = &m_static_objects[m_static_objects.size() - 1];
    }

    /* Add textures to the objects. */
    auto old_crate_albedo_map    = std::make_shared<RGL::Texture2D>(); old_crate_albedo_map   ->Load(RGL::FileSystem::getResourcesPath() / "models/old_crate/Flat_Crate_Diff.png", true);
    auto old_crate_normal_map    = std::make_shared<RGL::Texture2D>(); old_crate_normal_map   ->Load(RGL::FileSystem::getResourcesPath() / "models/old_crate/Flat_Crate_NORMAL.png");
    auto old_crate_metallic_map  = std::make_shared<RGL::Texture2D>(); old_crate_metallic_map ->Load(RGL::FileSystem::getResourcesPath() / "models/old_crate/Flat_Crate_Texture_METAL.png");
    auto old_crate_roughness_map = std::make_shared<RGL::Texture2D>(); old_crate_roughness_map->Load(RGL::FileSystem::getResourcesPath() / "models/old_crate/Flat_Crate_Texture_ROUGH.png");

    auto wooden_floor_albedo_map    = std::make_shared<RGL::Texture2D>(); wooden_floor_albedo_map   ->Load(RGL::FileSystem::getResourcesPath() / "textures/pbr/weathered_brown_planks_1k/weathered_brown_planks_diff_1k.png", true);
    auto wooden_floor_normal_map    = std::make_shared<RGL::Texture2D>(); wooden_floor_normal_map   ->Load(RGL::FileSystem::getResourcesPath() / "textures/pbr/weathered_brown_planks_1k/weathered_brown_planks_nor_1k.png");
    auto wooden_floor_roughness_map = std::make_shared<RGL::Texture2D>(); wooden_floor_roughness_map->Load(RGL::FileSystem::getResourcesPath() / "textures/pbr/weathered_brown_planks_1k/weathered_brown_planks_rough_1k.png");
    auto wooden_floor_ao_map        = std::make_shared<RGL::Texture2D>(); wooden_floor_ao_map       ->Load(RGL::FileSystem::getResourcesPath() / "textures/pbr/weathered_brown_planks_1k/weathered_brown_planks_ao_1k.png");

    wooden_floor_albedo_map   ->SetAnisotropy(16.0f);
    wooden_floor_normal_map   ->SetAnisotropy(16.0f);
    wooden_floor_roughness_map->SetAnisotropy(16.0f);
    wooden_floor_ao_map       ->SetAnisotropy(16.0f);

    wooden_floor_albedo_map   ->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT); wooden_floor_albedo_map   ->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
    wooden_floor_normal_map   ->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT); wooden_floor_normal_map   ->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
    wooden_floor_roughness_map->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT); wooden_floor_roughness_map->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
    wooden_floor_ao_map       ->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT); wooden_floor_ao_map       ->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);

    crate_model->AddTexture(old_crate_albedo_map,    RGL::Material::TextureType::ALBEDO);
    crate_model->AddTexture(old_crate_normal_map,    RGL::Material::TextureType::NORMAL);
    crate_model->AddTexture(old_crate_metallic_map,  RGL::Material::TextureType::METALLIC);
    crate_model->AddTexture(old_crate_roughness_map, RGL::Material::TextureType::ROUGHNESS);

    plane_model->AddTexture(wooden_floor_albedo_map,    RGL::Material::TextureType::ALBEDO);
    plane_model->AddTexture(wooden_floor_normal_map,    RGL::Material::TextureType::NORMAL);
    plane_model->AddTexture(wooden_floor_roughness_map, RGL::Material::TextureType::ROUGHNESS);
    plane_model->AddTexture(wooden_floor_ao_map,        RGL::Material::TextureType::AO);

    /* Create shader. */
    std::string dir = "src/demos/22_pbr/";
    m_ambient_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-ambient.frag");
    m_ambient_light_shader->link();

    m_point_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-point.frag");
    m_point_light_shader->link();

    m_equirectangular_to_cubemap_shader = std::make_shared<RGL::Shader>(dir + "cubemap.vert", dir + "equirectangular_to_cubemap.frag");
    m_equirectangular_to_cubemap_shader->link();

    m_irradiance_convolution_shader = std::make_shared<RGL::Shader>(dir + "cubemap.vert", dir + "irradiance_convolution.frag");
    m_irradiance_convolution_shader->link();

    m_prefilter_env_map_shader = std::make_shared<RGL::Shader>(dir + "cubemap.vert", dir + "prefilter_cubemap.frag");
    m_prefilter_env_map_shader->link();

    m_precompute_brdf = std::make_shared<RGL::Shader>("src/demos/10_postprocessing_filters/FSQ.vert", dir + "precompute_brdf.frag");
    m_precompute_brdf->link();

    m_background_shader = std::make_shared<RGL::Shader>(dir + "background.vert", dir + "background.frag");
    m_background_shader->link();

    m_tmo_ps = std::make_shared<PostprocessFilter>(RGL::Window::getWidth(), RGL::Window::getHeight());

    /* Bloom shaders. */
    dir = "src/demos/26_bloom/";
    m_downscale_shader = std::make_shared<RGL::Shader>(dir + "downscale.comp");
    m_downscale_shader->link();

    m_upscale_shader = std::make_shared<RGL::Shader>(dir + "upscale.comp");
    m_upscale_shader->link();

    m_bloom_dirt_texture = std::make_shared<RGL::Texture2D>(); 
    m_bloom_dirt_texture->Load(RGL::FileSystem::getResourcesPath() / "textures/bloom_dirt_mask.png");

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

    PrecomputeIndirectLight(RGL::FileSystem::getResourcesPath() / "textures/skyboxes/IBL" / m_hdr_maps_names[m_current_hdr_map_idx]);
    PrecomputeBRDF(m_brdf_lut_rt);
}

void Bloom::input()
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
        std::string filename = "26_bloom";
        if (take_screenshot_png(filename, RGL::Window::getWidth() / 2.0, RGL::Window::getHeight() / 2.0))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << RGL::FileSystem::getRootPath() / "screenshots/" << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RGL::FileSystem::getRootPath() / "screenshots/" << std::endl;
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

void Bloom::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void Bloom::HdrEquirectangularToCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt, const std::shared_ptr<RGL::Texture2D>& m_equirectangular_map)
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

void Bloom::IrradianceConvolution(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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

void Bloom::PrefilterCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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

void Bloom::PrecomputeIndirectLight(const std::filesystem::path& hdri_map_filepath)
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

void Bloom::PrecomputeBRDF(const std::shared_ptr<Texture2DRenderTarget>& rt)
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

void Bloom::GenSkyboxGeometry()
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

void Bloom::RenderScene()
{
    auto view_projection = m_camera->m_projection * m_camera->m_view;

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_cam_pos", m_camera->position());

    /* First, render the ambient color only for the opaque objects. */
    m_irradiance_cubemap_rt->bindTexture(6);
    m_prefiltered_env_map_rt->bindTexture(7);
    m_brdf_lut_rt->bindTexture(8);

    for (uint32_t i = 0; i < m_static_objects.size(); ++i)
    {
        glBindTextures(0, 5, nullptr);
        m_ambient_light_shader->setUniform("u_has_albedo_map",    m_static_objects[i].m_material.m_has_albedo_map);
        m_ambient_light_shader->setUniform("u_has_normal_map",    m_static_objects[i].m_material.m_has_normal_map);
        m_ambient_light_shader->setUniform("u_has_metallic_map",  m_static_objects[i].m_material.m_has_metallic_map);
        m_ambient_light_shader->setUniform("u_has_roughness_map", m_static_objects[i].m_material.m_has_roughness_map);
        m_ambient_light_shader->setUniform("u_has_ao_map",        m_static_objects[i].m_material.m_has_ao_map);
        
        m_ambient_light_shader->setUniform("u_albedo",    m_static_objects[i].m_material.m_albedo);
        m_ambient_light_shader->setUniform("u_metallic",  m_static_objects[i].m_material.m_metallic);
        m_ambient_light_shader->setUniform("u_roughness", m_static_objects[i].m_material.m_roughness);
        m_ambient_light_shader->setUniform("u_ao",        m_static_objects[i].m_material.m_ao);
        m_ambient_light_shader->setUniform("u_emission",  m_static_objects[i].m_material.m_emission);

        m_ambient_light_shader->setUniform("u_model",         m_static_objects[i].m_transform);
        m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_static_objects[i].m_transform))));
        m_ambient_light_shader->setUniform("u_mvp",           view_projection * m_static_objects[i].m_transform);

        m_static_objects[i].m_model->Render();
    }

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

    for (uint8_t p = 0; p < std::size(m_point_lights_properties); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",     m_point_lights_properties[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity", m_point_lights_properties[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",       m_point_lights_properties[p].position);
        m_point_light_shader->setUniform("u_point_light.radius",         m_point_lights_properties[p].radius);

        for (uint32_t i = 0; i < m_static_objects.size(); ++i)
        {
            glBindTextures(0, 5, nullptr);
            m_point_light_shader->setUniform("u_has_albedo_map",    m_static_objects[i].m_material.m_has_albedo_map);
            m_point_light_shader->setUniform("u_has_normal_map",    m_static_objects[i].m_material.m_has_normal_map);
            m_point_light_shader->setUniform("u_has_metallic_map",  m_static_objects[i].m_material.m_has_metallic_map);
            m_point_light_shader->setUniform("u_has_roughness_map", m_static_objects[i].m_material.m_has_roughness_map);
            
            m_point_light_shader->setUniform("u_albedo",    m_static_objects[i].m_material.m_albedo);
            m_point_light_shader->setUniform("u_metallic",  m_static_objects[i].m_material.m_metallic);
            m_point_light_shader->setUniform("u_roughness", m_static_objects[i].m_material.m_roughness);

            m_point_light_shader->setUniform("u_model",         m_static_objects[i].m_transform);
            m_point_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_static_objects[i].m_transform))));
            m_point_light_shader->setUniform("u_mvp",           view_projection * m_static_objects[i].m_transform);

            m_static_objects[i].m_model->Render();
        }
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void Bloom::render()
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

void Bloom::render_gui()
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
                    PrecomputeIndirectLight(RGL::FileSystem::getResourcesPath() / "textures/skyboxes/IBL" / m_hdr_maps_names[m_current_hdr_map_idx]);
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();

        ImGui::Checkbox   ("Bloom enabled",        &m_bloom_enabled);
        ImGui::SliderFloat("Bloom threshold",      &m_threshold,            0.0f, 15.0f, "%.1f");
        ImGui::SliderFloat("Bloom knee",           &m_knee,                 0.0f, 1.0f,  "%.1f");
        ImGui::SliderFloat("Bloom intensity",      &m_bloom_intensity,      0.0f, 5.0f,  "%.1f");
        ImGui::SliderFloat("Bloom dirt intensity", &m_bloom_dirt_intensity, 0.0f, 10.0f, "%.1f");

        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            for(uint8_t i = 0; i < std::size(m_point_lights_properties); ++i)
            {
                if (ImGui::BeginTabItem(std::string("Point" + std::to_string(i+1)).c_str()))
                {
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                    {
                        if (ImGui::ColorEdit3("Color", &m_point_lights_properties[i].color[0]))
                        {
                            m_light_boxes[i]->m_material.m_emission = m_point_lights_properties[i].color * m_point_lights_properties[i].intensity;
                        }

                        if (ImGui::SliderFloat("Light intensity", &m_point_lights_properties[i].intensity, 0.0, 50.0, "%.2f"))
                        {
                            m_light_boxes[i]->m_material.m_emission = m_point_lights_properties[i].color * m_point_lights_properties[i].intensity;
                        }

                        ImGui::SliderFloat("Radius", &m_point_lights_properties[i].radius, 0.01, 100.0, "%.2f");

                        if (ImGui::SliderFloat3("Position", &m_point_lights_properties[i].position[0], -10.0, 10.0, "%.2f"))
                        {
                            m_light_boxes[i]->m_transform = glm::translate(glm::mat4(1.0f), m_point_lights_properties[i].position) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
                        }
                    }
                    ImGui::PopItemWidth();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
