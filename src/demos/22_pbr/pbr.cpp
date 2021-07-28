#include "pbr.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

PBR::PBR()
      : m_dir_light_angles  (0.0f, 0.0f),
        m_spot_light_angles (90.0f, -25.0f),
        m_exposure (0.3f),
        m_gamma (3.6f),
        m_background_lod_level(1.2),
        m_skybox_vao(0),
        m_skybox_vbo(0)
{
}

PBR::~PBR()
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

void PBR::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.05, 0.05, 0.05, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-10.3, 7.6, -5.42);
    m_camera->setOrientation(glm::quat(-0.3, -0.052, -0.931, -0.165));
   

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 1.0f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    m_point_light_properties[0].color     = glm::vec3(1.0, 0.48, 0.0);
    m_point_light_properties[0].intensity = 100.0f;
    m_point_light_properties[0].position  = glm::vec3(-8.0, 5.0, -1.0);
    m_point_light_properties[0].radius    = 10.0f; 

    m_point_light_properties[1].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[1].intensity = 100.0f;
    m_point_light_properties[1].position  = glm::vec3(10.0, 10.0, -10.0);
    m_point_light_properties[1].radius     = 10.0f; 

    m_point_light_properties[2].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[2].intensity = 100.0f;
    m_point_light_properties[2].position  = glm::vec3(-10.0, -10.0, -10.0);
    m_point_light_properties[2].radius     = 10.0f; 

    m_point_light_properties[3].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[3].intensity = 100.0f;
    m_point_light_properties[3].position  = glm::vec3(10.0, -10.0, -10.0);
    m_point_light_properties[3].radius     = 10.0f; 

    m_spot_light_properties.color       = glm::vec3(0.0, 1.0, 0.0);
    m_spot_light_properties.intensity   = 100.0f;
    m_spot_light_properties.position    = glm::vec3(-6, 8.0, 3.5);
    m_spot_light_properties.radius      = 35.0f;
    m_spot_light_properties.inner_angle = 30.0f;
    m_spot_light_properties.outer_angle = 45.0f;
    m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);

    /* Create models. */
    m_sphere_model.GenSphere(1.0, 64);

    m_textured_models[0].GenCube();
    m_textured_models[1].GenCube();
    m_textured_models[2].GenCone(2.0, 1.0, 64, 64);
    m_textured_models[3].GenPQTorusKnot(256, 64, 2, 3, 0.75, 0.25);
    m_textured_models[4].GenSphere(1.0, 64);

    m_textured_models_model_matrices[0] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 0.0,   0.0))             * glm::scale(glm::mat4(1.0), glm::vec3(6.0, 0.1, 6.0));
    m_textured_models_model_matrices[1] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 2.0,   6.1)) * glm::scale(glm::mat4(1.0), glm::vec3(6.0, 2.0, 0.1));
    m_textured_models_model_matrices[2] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 2.11, -3.5)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
    m_textured_models_model_matrices[3] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 1.21,  0.0)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));
    m_textured_models_model_matrices[4] = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 4 + 1.11,  3.5)) * glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0));

    m_cerberus_model.Load(RGL::FileSystem::getPath("models/cerberus/Cerberus_LP.FBX"));
    m_cerberus_model_matrix = glm::translate(glm::mat4(1.0), glm::vec3(-6.0, 6.0, -3.0)) * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(10 * m_cerberus_model.GetUnitScaleFactor()));

    /* Set model matrices for each model. */
    uint8_t num_rows = 7;
    uint8_t num_cols = 7;
    float spacing    = 2.5;

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_cols; ++col)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((col - (num_cols / 2)) * spacing, (row - (num_rows/ 2)) * spacing, 0.0f));
            m_objects_model_matrices.emplace_back(model);
        }
    }

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

    auto cerberus_normal_map    = std::make_shared<RGL::Texture2D>(); cerberus_normal_map   ->Load(RGL::FileSystem::getPath("models/cerberus/Textures/Cerberus_N.tga"));
    auto cerberus_metallic_map  = std::make_shared<RGL::Texture2D>(); cerberus_metallic_map ->Load(RGL::FileSystem::getPath("models/cerberus/Textures/Cerberus_M.tga"));
    auto cerberus_roughness_map = std::make_shared<RGL::Texture2D>(); cerberus_roughness_map->Load(RGL::FileSystem::getPath("models/cerberus/Textures/Cerberus_R.tga"));

    m_cerberus_model.AddTexture(cerberus_normal_map,    1);
    m_cerberus_model.AddTexture(cerberus_metallic_map,  2);
    m_cerberus_model.AddTexture(cerberus_roughness_map, 3);

    /* Create shader. */
    std::string dir = "../src/demos/22_pbr/";
    m_ambient_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-ambient.frag");
    m_ambient_light_shader->link();

    m_directional_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-directional.frag");
    m_directional_light_shader->link();

    m_point_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-point.frag");
    m_point_light_shader->link();

    m_spot_light_shader = std::make_shared<RGL::Shader>(dir + "pbr-lighting.vert", dir + "pbr-spot.frag");
    m_spot_light_shader->link();

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
}

void PBR::input()
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
        std::string filename = "22_pbr";
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

void PBR::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void PBR::HdrEquirectangularToCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt, const std::shared_ptr<RGL::Texture2D>& m_equirectangular_map)
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

void PBR::IrradianceConvolution(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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

void PBR::PrefilterCubemap(const std::shared_ptr<CubeMapRenderTarget>& cubemap_rt)
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

void PBR::PrecomputeIndirectLight(const std::filesystem::path& hdri_map_filepath)
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

void PBR::PrecomputeBRDF(const std::shared_ptr<Texture2DRenderTarget>& rt)
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

void PBR::GenSkyboxGeometry()
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

void PBR::RenderSpheres()
{
    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_cam_pos", m_camera->position());
    m_ambient_light_shader->setUniform("u_albedo",  glm::vec3(0.5, 0.0, 0.0f));
    m_ambient_light_shader->setUniform("u_ao",      1.0f);

    m_ambient_light_shader->setUniform("u_has_albedo_map",    false);
    m_ambient_light_shader->setUniform("u_has_normal_map",    false);
    m_ambient_light_shader->setUniform("u_has_metallic_map",  false);
    m_ambient_light_shader->setUniform("u_has_roughness_map", false);
    m_ambient_light_shader->setUniform("u_has_ao_map",        false);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* First, render the ambient color only for the opaque objects. */
    m_irradiance_cubemap_rt->bindTexture(5);
    m_prefiltered_env_map_rt->bindTexture(6);
    m_brdf_lut_rt->bindTexture(7);

    for (unsigned row = 0; row < 7; ++row)
    {
        m_ambient_light_shader->setUniform("u_metallic", float(row)/7.0f);
        for (unsigned col = 0; col < 7; ++col)
        {
            m_ambient_light_shader->setUniform("u_roughness", glm::clamp(float(col) / 7.0f, 0.05f, 1.0f));

            uint32_t idx = col + row * 7;
            m_ambient_light_shader->setUniform("u_model",         m_objects_model_matrices[idx]);
            m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[idx]))));
            m_ambient_light_shader->setUniform("u_mvp",           view_projection * m_objects_model_matrices[idx]);

            m_sphere_model.Render();
        }
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
    m_directional_light_shader->setUniform("u_albedo",            glm::vec3(0.5, 0.0, 0.0f));
    m_directional_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_directional_light_shader->setUniform("u_has_albedo_map",    false);
    m_directional_light_shader->setUniform("u_has_normal_map",    false);
    m_directional_light_shader->setUniform("u_has_metallic_map",  false);
    m_directional_light_shader->setUniform("u_has_roughness_map", false);

    m_directional_light_shader->setUniform("u_directional_light.base.color",     m_dir_light_properties.color);
    m_directional_light_shader->setUniform("u_directional_light.base.intensity", m_dir_light_properties.intensity);
    m_directional_light_shader->setUniform("u_directional_light.direction",      m_dir_light_properties.direction);

    for (unsigned row = 0; row < 7; ++row)
    {
        m_directional_light_shader->setUniform("u_metallic", float(row) / 7.0f);
        for (unsigned col = 0; col < 7; ++col)
        {
            m_directional_light_shader->setUniform("u_roughness", glm::clamp(float(col) / 7.0f, 0.05f, 1.0f));

            uint32_t idx = col + row * 7;
            m_directional_light_shader->setUniform("u_model",         m_objects_model_matrices[idx]);
            m_directional_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[idx]))));
            m_directional_light_shader->setUniform("u_mvp",           view_projection * m_objects_model_matrices[idx]);

            m_sphere_model.Render();
        }
    }

    /* Render point lights */
    m_point_light_shader->bind();
    m_point_light_shader->setUniform("u_albedo",            glm::vec3(0.5, 0.0, 0.0f));
    m_point_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_point_light_shader->setUniform("u_has_albedo_map",    false);
    m_point_light_shader->setUniform("u_has_normal_map",    false);
    m_point_light_shader->setUniform("u_has_metallic_map",  false);
    m_point_light_shader->setUniform("u_has_roughness_map", false);

    for(uint8_t p = 0; p < std::size(m_point_light_properties); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",      m_point_light_properties[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity",  m_point_light_properties[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",        m_point_light_properties[p].position);
        m_point_light_shader->setUniform("u_point_light.radius",          m_point_light_properties[p].radius);

        for (unsigned row = 0; row < 7; ++row)
        {
            m_point_light_shader->setUniform("u_metallic", float(row)/7.0f);
            for (unsigned col = 0; col < 7; ++col)
            {
                m_point_light_shader->setUniform("u_roughness",     glm::clamp(float(col) / 7.0f, 0.05f, 1.0f));

                uint32_t idx = col + row * 7;
                m_point_light_shader->setUniform("u_model",         m_objects_model_matrices[idx]);
                m_point_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[idx]))));
                m_point_light_shader->setUniform("u_mvp",           view_projection * m_objects_model_matrices[idx]);

                m_sphere_model.Render();
            }
        }
    }
    /* Render spot lights */
    m_spot_light_shader->bind();
    m_spot_light_shader->setUniform("u_albedo",            glm::vec3(0.5, 0.0, 0.0f));
    m_spot_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_spot_light_shader->setUniform("u_has_albedo_map",    false);
    m_spot_light_shader->setUniform("u_has_normal_map",    false);
    m_spot_light_shader->setUniform("u_has_metallic_map",  false);
    m_spot_light_shader->setUniform("u_has_roughness_map", false);

    m_spot_light_shader->setUniform("u_spot_light.point.base.color",      m_spot_light_properties.color);
    m_spot_light_shader->setUniform("u_spot_light.point.base.intensity",  m_spot_light_properties.intensity);
    m_spot_light_shader->setUniform("u_spot_light.point.position",        m_spot_light_properties.position);
    m_spot_light_shader->setUniform("u_spot_light.point.radius",          m_spot_light_properties.radius);
    m_spot_light_shader->setUniform("u_spot_light.direction",             m_spot_light_properties.direction);
    m_spot_light_shader->setUniform("u_spot_light.inner_angle",           glm::radians(m_spot_light_properties.inner_angle));
    m_spot_light_shader->setUniform("u_spot_light.outer_angle",           glm::radians(m_spot_light_properties.outer_angle));

    for (unsigned row = 0; row < 7; ++row)
    {
        m_spot_light_shader->setUniform("u_metallic", float(row) / 7.0f);
        for (unsigned col = 0; col < 7; ++col)
        {
            m_spot_light_shader->setUniform("u_roughness", glm::clamp(float(col) / 7.0f, 0.05f, 1.0f));

            uint32_t idx = col + row * 7;
            m_spot_light_shader->setUniform("u_model",         m_objects_model_matrices[idx]);
            m_spot_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[idx]))));
            m_spot_light_shader->setUniform("u_mvp",           view_projection * m_objects_model_matrices[idx]);

            m_sphere_model.Render();
        }
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void PBR::RenderTexturedModels()
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

    for (unsigned i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_directional_light_shader->setUniform("u_model",         m_textured_models_model_matrices[i]);
        m_directional_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_textured_models_model_matrices[i]))));
        m_directional_light_shader->setUniform("u_mvp",           view_projection * m_textured_models_model_matrices[i]);

        m_textured_models[i].Render();
    }

    /* Render point lights */
    m_point_light_shader->bind();
    m_point_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_point_light_shader->setUniform("u_has_albedo_map",    true);
    m_point_light_shader->setUniform("u_has_normal_map",    true);
    m_point_light_shader->setUniform("u_has_metallic_map",  true);
    m_point_light_shader->setUniform("u_has_roughness_map", true);

    for (uint8_t p = 0; p < std::size(m_point_light_properties); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",     m_point_light_properties[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity", m_point_light_properties[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",       m_point_light_properties[p].position);
        m_point_light_shader->setUniform("u_point_light.radius",         m_point_light_properties[p].radius);

        for (uint32_t i = 0; i < std::size(m_textured_models_model_matrices); ++i)
        {
            m_point_light_shader->setUniform("u_model",         m_textured_models_model_matrices[i]);
            m_point_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_textured_models_model_matrices[i]))));
            m_point_light_shader->setUniform("u_mvp",           view_projection * m_textured_models_model_matrices[i]);

            m_textured_models[i].Render();
        }
    }
    /* Render spot lights */
    m_spot_light_shader->bind();
    m_spot_light_shader->setUniform("u_albedo",            glm::vec3(0.5, 0.0, 0.0f));
    m_spot_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_spot_light_shader->setUniform("u_has_albedo_map",    true);
    m_spot_light_shader->setUniform("u_has_normal_map",    true);
    m_spot_light_shader->setUniform("u_has_metallic_map",  true);
    m_spot_light_shader->setUniform("u_has_roughness_map", true);

    m_spot_light_shader->setUniform("u_spot_light.point.base.color",      m_spot_light_properties.color);
    m_spot_light_shader->setUniform("u_spot_light.point.base.intensity",  m_spot_light_properties.intensity);
    m_spot_light_shader->setUniform("u_spot_light.point.position",        m_spot_light_properties.position);
    m_spot_light_shader->setUniform("u_spot_light.point.radius",          m_spot_light_properties.radius);
    m_spot_light_shader->setUniform("u_spot_light.direction",             m_spot_light_properties.direction);
    m_spot_light_shader->setUniform("u_spot_light.inner_angle",           glm::radians(m_spot_light_properties.inner_angle));
    m_spot_light_shader->setUniform("u_spot_light.outer_angle",           glm::radians(m_spot_light_properties.outer_angle));

    for (unsigned i = 0; i < std::size(m_textured_models_model_matrices); ++i)
    {
        m_spot_light_shader->setUniform("u_model",         m_textured_models_model_matrices[i]);
        m_spot_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_textured_models_model_matrices[i]))));
        m_spot_light_shader->setUniform("u_mvp",           view_projection * m_textured_models_model_matrices[i]);

        m_textured_models[i].Render();
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void PBR::RenderCerberusPistol()
{
    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_cam_pos", m_camera->position());
    m_ambient_light_shader->setUniform("u_ao", 1.0f);

    m_ambient_light_shader->setUniform("u_has_albedo_map",    true);
    m_ambient_light_shader->setUniform("u_has_normal_map",    true);
    m_ambient_light_shader->setUniform("u_has_metallic_map",  true);
    m_ambient_light_shader->setUniform("u_has_roughness_map", true);
    m_ambient_light_shader->setUniform("u_has_ao_map",        false);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* First, render the ambient color only for the opaque objects. */
    m_irradiance_cubemap_rt->bindTexture(5);
    m_prefiltered_env_map_rt->bindTexture(6);
    m_brdf_lut_rt->bindTexture(7);

    m_ambient_light_shader->setUniform("u_model",         m_cerberus_model_matrix);
    m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_cerberus_model_matrix))));
    m_ambient_light_shader->setUniform("u_mvp",           view_projection * m_cerberus_model_matrix);

    m_cerberus_model.Render();

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

    m_directional_light_shader->setUniform("u_model",         m_cerberus_model_matrix);
    m_directional_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_cerberus_model_matrix))));
    m_directional_light_shader->setUniform("u_mvp",           view_projection * m_cerberus_model_matrix);

    m_cerberus_model.Render();

    /* Render point lights */
    m_point_light_shader->bind();
    m_point_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_point_light_shader->setUniform("u_has_albedo_map",    true);
    m_point_light_shader->setUniform("u_has_normal_map",    true);
    m_point_light_shader->setUniform("u_has_metallic_map",  true);
    m_point_light_shader->setUniform("u_has_roughness_map", true);

    for (uint8_t p = 0; p < std::size(m_point_light_properties); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",     m_point_light_properties[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity", m_point_light_properties[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",       m_point_light_properties[p].position);
        m_point_light_shader->setUniform("u_point_light.radius",         m_point_light_properties[p].radius);

        m_point_light_shader->setUniform("u_model",         m_cerberus_model_matrix);
        m_point_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_cerberus_model_matrix))));
        m_point_light_shader->setUniform("u_mvp",           view_projection * m_cerberus_model_matrix);

        m_cerberus_model.Render();
    }
    /* Render spot lights */
    m_spot_light_shader->bind();
    m_spot_light_shader->setUniform("u_cam_pos",           m_camera->position());
    m_spot_light_shader->setUniform("u_has_albedo_map",    true);
    m_spot_light_shader->setUniform("u_has_normal_map",    true);
    m_spot_light_shader->setUniform("u_has_metallic_map",  true);
    m_spot_light_shader->setUniform("u_has_roughness_map", true);

    m_spot_light_shader->setUniform("u_spot_light.point.base.color",      m_spot_light_properties.color);
    m_spot_light_shader->setUniform("u_spot_light.point.base.intensity",  m_spot_light_properties.intensity);
    m_spot_light_shader->setUniform("u_spot_light.point.position",        m_spot_light_properties.position);
    m_spot_light_shader->setUniform("u_spot_light.point.radius",          m_spot_light_properties.radius);
    m_spot_light_shader->setUniform("u_spot_light.direction",             m_spot_light_properties.direction);
    m_spot_light_shader->setUniform("u_spot_light.inner_angle",           glm::radians(m_spot_light_properties.inner_angle));
    m_spot_light_shader->setUniform("u_spot_light.outer_angle",           glm::radians(m_spot_light_properties.outer_angle));

    m_spot_light_shader->setUniform("u_model",         m_cerberus_model_matrix);
    m_spot_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_cerberus_model_matrix))));
    m_spot_light_shader->setUniform("u_mvp",           view_projection * m_cerberus_model_matrix);

    m_cerberus_model.Render();

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}

void PBR::render()
{
    /* Put render specific code here. Don't update variables here! */
    m_tmo_ps->bindFilterFBO();

    switch (m_current_scene)
    {
        case Scene::SPHERES:
            RenderSpheres();
            break;

        case Scene::TEXTURED:
            RenderTexturedModels();
            break;
        case Scene::CERBERUS_PISTOL:
            RenderCerberusPistol();
            break;
    }

    m_background_shader->bind();
    m_background_shader->setUniform("u_projection", m_camera->m_projection);
    m_background_shader->setUniform("u_view", glm::mat4(glm::mat3(m_camera->m_view)));
    m_background_shader->setUniform("u_lod_level", m_background_lod_level);
    m_env_cubemap_rt->bindTexture();

    glBindVertexArray(m_skybox_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    m_tmo_ps->render(m_exposure, m_gamma);
}

void PBR::render_gui()
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
                    m_current_hdr_map_idx = i;
                    PrecomputeIndirectLight(RGL::FileSystem::getPath("textures/skyboxes/IBL/" + m_hdr_maps_names[m_current_hdr_map_idx]));
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Scene", m_scene_names[int(m_current_scene)].c_str()))
        {
            for (int i = 0; i < std::size(m_scene_names); ++i)
            {
                bool is_selected = (int(m_current_scene) == i);
                if (ImGui::Selectable(m_scene_names[i].c_str(), is_selected))
                {
                    m_current_scene = Scene(i);
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
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
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            for(uint8_t i = 0; i < std::size(m_point_light_properties); ++i)
            {
                if (ImGui::BeginTabItem(std::string("Point" + std::to_string(i+1)).c_str()))
                {
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                    {
                        ImGui::ColorEdit3 ("Color",              &m_point_light_properties[i].color[0]);
                        ImGui::SliderFloat("Light intensity",    &m_point_light_properties[i].intensity, 0.0, 2000.0,  "%1.f");

                        ImGui::SliderFloat ("Radius",                &m_point_light_properties[i].radius,                0.01, 100.0, "%.2f");
                        ImGui::SliderFloat3("Position",              &m_point_light_properties[i].position[0],          -10.0,  10.0,  "%.1f");
                    }
                    ImGui::PopItemWidth();
                    ImGui::EndTabItem();
                }
            }
            if (ImGui::BeginTabItem("Spot"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3 ("Color",              &m_spot_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",    &m_spot_light_properties.intensity, 0.0, 2000.0, "%.1f");

                    ImGui::SliderFloat("Radius",      &m_spot_light_properties.radius,       0.01, 100.0, "%.2f");
                    ImGui::SliderFloat("Inner angle", &m_spot_light_properties.inner_angle,  0.0,  90.0,   "%.1f");
                    ImGui::SliderFloat("Outer angle", &m_spot_light_properties.outer_angle,  0.0,  90.0,   "%.1f");
                    ImGui::SliderFloat3("Position",   &m_spot_light_properties.position[0], -10.0, 10.0,  "%.1f");

                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_spot_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);
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
