#include "pbr.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/matrix_inverse.hpp>

PBR::PBR()
      : m_dir_light_angles  (0.0f, 0.0f),
        m_spot_light_angles (0.0f, 0.0f),
        m_exposure (1.0f),
        m_gamma (2.2f),
        m_a (1.6f),
        m_d (0.977f),
        m_hdr_max (8.0f),
        m_mid_in (0.18f),
        m_mid_out (0.267f)
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
    m_camera->setPosition(-12.25, 6.2, -14.65);
    m_camera->setOrientation(glm::quat(-0.256, -0.036, -0.95, -0.14));
   

    /* Initialize lights' properties */
    m_dir_light_properties.color     = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 0.2f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    m_point_light_properties[0].color       = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[0].intensity   = 1000.0f;
    m_point_light_properties[0].position    = glm::vec3(-10.0, 10.0, -10.0);

    m_point_light_properties[1].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[1].intensity = 1000.0f;
    m_point_light_properties[1].position  = glm::vec3(10.0, 10.0, -10.0);

    m_point_light_properties[2].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[2].intensity = 1000.0f;
    m_point_light_properties[2].position  = glm::vec3(-10.0, -10.0, -10.0);

    m_point_light_properties[3].color     = glm::vec3(1.0, 1.0, 1.0);
    m_point_light_properties[3].intensity = 1000.0f;
    m_point_light_properties[3].position  = glm::vec3(10.0, -10.0, -10.0);

    m_spot_light_properties.color       = glm::vec3(0.0, 0.0, 1.0);
    m_spot_light_properties.intensity   = 5.0f;
    m_spot_light_properties.position    = glm::vec3(-7.5, 3.0, -5);
    m_spot_light_properties.range       = 35.0f;
    m_spot_light_properties.cutoff      = 45.0f;
    m_spot_light_properties.setDirection(m_spot_light_angles.x, m_spot_light_angles.y);

    /* Create models. */
    m_sphere_model.GenSphere(1.0, 64);

    /* Set model matrices for each model. */
    uint8_t num_rows = 7;
    uint8_t num_cols = 7;
    float spacing    = 2.5;

    for (int row = 0; row < num_rows; ++row)
    {
        //shader.setFloat("metallic", (float)row / (float)nrRows);
        for (int col = 0; col < num_cols; ++col)
        {
            // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            //shader.setFloat("roughness", glm::clamp((float)col / (float)nrColumns, 0.05f, 1.0f));

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((col - (num_cols / 2)) * spacing, (row - (num_rows/ 2)) * spacing, 0.0f));
            m_objects_model_matrices.emplace_back(model);
        }
    }

    /* Add textures to the objects. */
    auto envmap_hdr = std::make_shared<RGL::Texture2D>();
    envmap_hdr->LoadHdr(RGL::FileSystem::getPath("textures/skyboxes/IBL/Newport_Loft.hdr"));
    auto envmap_metadata = envmap_hdr->GetMetadata();

    GenSkyboxGeometry();

    m_env_cubemap_rt = std::make_shared<CubeMapRenderTarget>();
    m_env_cubemap_rt->set_position(glm::vec3(0.0));
    m_env_cubemap_rt->generate_rt(512, 512, true);

    m_irradiance_cubemap_rt = std::make_shared<CubeMapRenderTarget>();
    m_irradiance_cubemap_rt->set_position(glm::vec3(0.0));
    m_irradiance_cubemap_rt->generate_rt(32, 32);

    m_prefiltered_env_map_rt = std::make_shared<CubeMapRenderTarget>();
    m_prefiltered_env_map_rt->set_position(glm::vec3(0.0));
    m_prefiltered_env_map_rt->generate_rt(128, 128, true);

    m_brdf_lut_rt = std::make_shared<Texture2DRenderTarget>();
    m_brdf_lut_rt->generate_rt(512, 512);

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

    // Precomputations
    HdrEquirectangularToCubemap(m_env_cubemap_rt, envmap_hdr);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_env_cubemap_rt->m_cubemap_texture_id);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    IrradianceConvolution(m_irradiance_cubemap_rt);
    PrefilterCubemap(m_prefiltered_env_map_rt);
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
                  << " Orientation: [" << m_camera->orientation().x << ", " << m_camera->orientation().y << ", " << m_camera->orientation().z << ", " << m_camera->orientation().w << "]\n"
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

    uint8_t max_mip_levels = 5;
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
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
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

void PBR::render()
{
    /* Put render specific code here. Don't update variables here! */
    m_tmo_ps->bindFilterFBO();

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("u_cam_pos", m_camera->position());
    m_ambient_light_shader->setUniform("u_albedo", glm::vec3(0.5, 0.0, 0.0f));
    m_ambient_light_shader->setUniform("u_ao",     1.0f);

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
            m_ambient_light_shader->setUniform("u_model", m_objects_model_matrices[idx]);
            m_ambient_light_shader->setUniform("u_normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[idx]))));
            m_ambient_light_shader->setUniform("u_mvp", view_projection * m_objects_model_matrices[idx]);

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
    //m_directional_light_shader->bind();

    //m_directional_light_shader->setUniform("directional_light.base.color",     m_dir_light_properties.color);
    //m_directional_light_shader->setUniform("directional_light.base.intensity", m_dir_light_properties.intensity);
    //m_directional_light_shader->setUniform("directional_light.direction",      m_dir_light_properties.direction);
    //
    //m_directional_light_shader->setUniform("cam_pos",            m_camera->position());
    //m_directional_light_shader->setUniform("specular_intensity", m_specular_intenstiy.x);
    //m_directional_light_shader->setUniform("specular_power",     m_specular_power.x);
    //m_directional_light_shader->setUniform("gamma",              m_gamma);

    //for (unsigned i = 0; i < m_objects.size(); ++i)
    //{
    //    m_directional_light_shader->setUniform("model", m_objects_model_matrices[i]);
    //    m_directional_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
    //    m_directional_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

    //    m_objects[i].Render();
    //}

    /* Render point lights */
    m_point_light_shader->bind();
    m_point_light_shader->setUniform("u_albedo", glm::vec3(0.5, 0.0, 0.0f));
    m_point_light_shader->setUniform("u_cam_pos", m_camera->position());

    for(uint8_t p = 0; p < std::size(m_point_light_properties); ++p)
    {
        m_point_light_shader->setUniform("u_point_light.base.color",      m_point_light_properties[p].color);
        m_point_light_shader->setUniform("u_point_light.base.intensity",  m_point_light_properties[p].intensity);
        m_point_light_shader->setUniform("u_point_light.position",        m_point_light_properties[p].position);
        //m_point_light_shader->setUniform("u_point_light.range",           m_point_light_properties.range);

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
    //m_spot_light_shader->bind();

    //m_spot_light_shader->setUniform("spot_light.point.base.color",      m_spot_light_properties.color);
    //m_spot_light_shader->setUniform("spot_light.point.base.intensity",  m_spot_light_properties.intensity);
    //m_spot_light_shader->setUniform("spot_light.point.atten.constant",  m_spot_light_properties.attenuation.constant);
    //m_spot_light_shader->setUniform("spot_light.point.atten.linear",    m_spot_light_properties.attenuation.linear);
    //m_spot_light_shader->setUniform("spot_light.point.atten.quadratic", m_spot_light_properties.attenuation.quadratic);
    //m_spot_light_shader->setUniform("spot_light.point.position",        m_spot_light_properties.position);
    //m_spot_light_shader->setUniform("spot_light.point.range",           m_spot_light_properties.range);
    //m_spot_light_shader->setUniform("spot_light.direction",             m_spot_light_properties.direction);
    //m_spot_light_shader->setUniform("spot_light.cutoff",                glm::radians(90.0f - m_spot_light_properties.cutoff));

    //m_spot_light_shader->setUniform("cam_pos",            m_camera->position());
    //m_spot_light_shader->setUniform("specular_intensity", m_specular_intenstiy.z);
    //m_spot_light_shader->setUniform("specular_power",     m_specular_power.z);
    //m_spot_light_shader->setUniform("gamma",              m_gamma);

    //for (unsigned i = 0; i < m_objects.size(); ++i)
    //{
    //    m_spot_light_shader->setUniform("model", m_objects_model_matrices[i]);
    //    m_spot_light_shader->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
    //    m_spot_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

    //    m_objects[i].Render();
    //}

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    m_background_shader->bind();
    m_background_shader->setUniform("u_projection", m_camera->m_projection);
    m_background_shader->setUniform("u_view", glm::mat4(glm::mat3(m_camera->m_view)));
    m_env_cubemap_rt->bindTexture();
    //m_irradiance_cubemap_rt->bindTexture();
    //m_prefiltered_env_map_rt->bindTexture();
    glBindVertexArray(m_skybox_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    m_tmo_ps->render(m_exposure, m_gamma, m_a, m_d, m_hdr_max, m_mid_in, m_mid_out);
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
        ImGui::SliderFloat("Exposure", &m_exposure, 0.0, 10.0, "%.1f");
        ImGui::SliderFloat("Gamma",    &m_gamma,    0.0, 10.0, "%.1f");
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
                        ImGui::SliderFloat("Light intensity",    &m_point_light_properties[i].intensity, 0.0, 5000.0,  "%1.f");

                        //ImGui::SliderFloat ("Range",                 &m_point_light_properties[i].range,                 0.01, 100.0, "%.2f");
                        ImGui::SliderFloat3("Position",              &m_point_light_properties[i].position[0],          -10.0, 10.0,  "%.1f");
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
                    ImGui::SliderFloat("Light intensity",    &m_spot_light_properties.intensity, 0.0, 100.0, "%.1f");

                    ImGui::SliderFloat("Range",                 &m_spot_light_properties.range,                 0.01, 100.0, "%.2f");
                    ImGui::SliderFloat("Cut-off angle",         &m_spot_light_properties.cutoff,                33.0, 90.0,  "%.1f");
                    ImGui::SliderFloat3("Position",             &m_spot_light_properties.position[0],          -10.0, 10.0,  "%.1f");

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
