#include "toon_outline.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

ToonOutline::ToonOutline()
    : m_light_color                       (1.0),
      m_dir_light_azimuth_elevation_angles(45.0, 9.0),
      m_light_intensity                   (1.0),
      m_ambient_factor                    (0.18),
      m_specular_power                    (120.0),
      m_specular_intensity                (2.0),
      m_gamma                             (2.2),
      m_advanced_toon_A                   (0.1),
      m_advanced_toon_B                   (0.3),
      m_advanced_toon_C                   (0.6),
      m_advanced_toon_D                   (1.0),
      m_simple_toon_diffuse_levels        (3.0),
      m_simple_toon_specular_levels       (1.0),
      m_toon_shading_method               (ToonShadingMethod::SIMPLE),
      m_rim_color                         (1.0, 1.0, 1.0),
      m_rim_threshold                     (0.1),
      m_rim_amount                        (0.716),
      m_twin_shade_toon_diffuse_levels    (4),
      m_twin_shade_toon_specular_levels   (1),
      m_twin_shade_light_shade_cutoff     (0.6),
      m_twin_shade_dark_shade_cutoff      (0.15),
      m_outline_method                    (OutlineMethod::STENCIL),
      m_outline_color                     (0.0),
      m_stencil_outline_width             (20.0),
      m_depth_threshold                   (0.8),
      m_depth_normal_threshold            (0.5),
      m_depth_normal_threshold_scale      (7.0),
      m_normal_threshold                  (0.4),
      m_ps_outline_width                  (1.0)
{
    m_light_direction = calcDirection(m_dir_light_azimuth_elevation_angles);
}

ToonOutline::~ToonOutline()
{
    if (m_ps_vao_id != 0)
    {
        glDeleteVertexArrays(1, &m_ps_vao_id);
        m_ps_vao_id = 0;
    }

    if (m_fbo_normal_depth != 0)
    {
        glDeleteFramebuffers(1, &m_fbo_normal_depth);
        m_fbo_normal_depth = 0;
    }

    if (m_fbo_shading != 0)
    {
        glDeleteFramebuffers(1, &m_fbo_shading);
        m_fbo_shading = 0;
    }

    if (m_rbo != 0)
    {
        glDeleteRenderbuffers(1, &m_rbo);
        m_rbo = 0;
    }

    if (m_normals_depth_tex_buffer != 0)
    {
        glDeleteTextures(1, &m_normals_depth_tex_buffer);
        m_normals_depth_tex_buffer = 0;
    }

    if (m_shading_tex_buffer != 0)
    {
        glDeleteTextures(1, &m_shading_tex_buffer);
        m_shading_tex_buffer = 0;
    }
}

void ToonOutline::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable     (GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp  (GL_KEEP, GL_KEEP, GL_REPLACE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 10.0);

    /* Create models. */
    for (unsigned i = 0; i < 9; ++i)
    {
        m_objects.emplace_back(std::make_shared<RapidGL::Model>());
    }

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0]->load(RapidGL::FileSystem::getPath("models/spot/spot.obj"));
    m_objects[1]->load(RapidGL::FileSystem::getPath("models/bunny.obj"));
    m_objects[2]->load(RapidGL::FileSystem::getPath("models/cone.fbx"));
    m_objects[3]->genCube();
    m_objects[4]->load(RapidGL::FileSystem::getPath("models/cylinder.fbx"));
    m_objects[5]->load(RapidGL::FileSystem::getPath("models/torus_knot.fbx"));
    m_objects[6]->genSphere(0.5, 40);
    m_objects[7]->genTorus(0.5, 1.0, 40, 40);
    m_objects[8]->load(RapidGL::FileSystem::getPath("models/teapot.obj"));

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-10.0,   0.25, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0))); // spot
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( -7.5,  -0.5,  -5)));                                                                         // bunny
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( -5.0,  -0.5,  -5)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01)));                           // cone
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( -2.5,   0.0,  -5)));                                                                         // cube
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(  0.0,  -0.5,  -5)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01)));                           // cylinder
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(  2.5,   0.0,  -5)) * glm::scale(glm::mat4(1.0), glm::vec3(0.01))) ;                          // torus knot
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(  5.0,   0.0,  -5)));                                                                         // sphere
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(  7.5,   0.0,  -5)));                                                                         // torus
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 10.0,  -0.5,  -5)) * glm::scale(glm::mat4(1.0), glm::vec3(0.3)));                            // teapot

    /* Set colors for individual models. */
    m_objects_colors.emplace_back(glm::vec3(0.9, 0.9,  0.9));
    m_objects_colors.emplace_back(glm::vec3(0.5, 0.64, 1.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.9,  0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.0,  0.5));
    m_objects_colors.emplace_back(glm::vec3(0.9, 0.9,  0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.9,  0.9));
    m_objects_colors.emplace_back(glm::vec3(0.9, 0.0,  0.9));
    m_objects_colors.emplace_back(glm::vec3(0.9, 0.0,  0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.9,  0.0));

    RapidGL::Texture texture_spot;
    texture_spot.m_id   = RapidGL::Util::loadGLTexture("spot.png", "models/spot", true);
    texture_spot.m_type = "texture_diffuse";

    RapidGL::Texture default_diffuse_texture;
    default_diffuse_texture.m_id   = RapidGL::Util::loadGLTexture("default_diffuse.png", "textures", true);
    default_diffuse_texture.m_type = "texture_diffuse";

    m_objects[0]->getMesh(0).addTexture(texture_spot);

    for (auto& model : m_objects)
    {
        if (model->getMesh(0).getTexturesCount() == 0)
        {
            model->getMesh(0).addTexture(default_diffuse_texture);
        }
    }

    /* Create the toon shaders. */
    std::string dir = "../src/demos/05_toon_outline/";
    m_simple_toon_shader = std::make_shared<RapidGL::Shader>(dir + "toon.vert", dir + "toon_simple.frag");
    m_simple_toon_shader->link();

    m_advanced_toon_shader = std::make_shared<RapidGL::Shader>(dir + "toon.vert", dir + "toon_advanced.frag");
    m_advanced_toon_shader->link();

    m_simple_rim_toon_shader = std::make_shared<RapidGL::Shader>(dir + "toon.vert", dir + "toon_simple_rim.frag");
    m_simple_rim_toon_shader->link();

    m_toon_twin_shade_shader = std::make_shared<RapidGL::Shader>(dir + "toon.vert", dir + "toon_twin_shade.frag");
    m_toon_twin_shade_shader->link();

    m_toon_shading_methods_names = { "Simple", "Advanced", "Simple with Rim", "Toon Twin Shade" };
    m_toon_shaders               = { m_simple_toon_shader, m_advanced_toon_shader, m_simple_rim_toon_shader, m_toon_twin_shade_shader };

    /* Init GL objects for outline postprocess */
    glGenVertexArrays(1, &m_ps_vao_id);

    /* Create framebuffer for normals and depth rendering */
    glGenFramebuffers(1, &m_fbo_normal_depth);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_normal_depth);

    glGenTextures(1, &m_normals_depth_tex_buffer);
    glBindTexture(GL_TEXTURE_2D, m_normals_depth_tex_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RapidGL::Window::getWidth(), RapidGL::Window::getHeight(), 0 /* border */, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_normals_depth_tex_buffer, 0 /* level */);

    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, RapidGL::Window::getWidth(), RapidGL::Window::getHeight());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* Create framebuffer for shading rendering */
    glGenFramebuffers(1, &m_fbo_shading);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_shading);

    glGenTextures(1, &m_shading_tex_buffer);
    glBindTexture(GL_TEXTURE_2D, m_shading_tex_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RapidGL::Window::getWidth(), RapidGL::Window::getHeight(), 0 /* border */, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_shading_tex_buffer, 0 /* level */);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* Create the outline shaders. */
    m_outline_methods_names = { "Stencil", "Post-Process" };

    m_stencil_outline_shader = std::make_shared<RapidGL::Shader>(dir + "outline_stencil.vert", dir + "outline_stencil.frag");
    m_stencil_outline_shader->link();

    m_generate_data_outline_shader = std::make_shared<RapidGL::Shader>(dir + "outline_ps_gen_data.vert", dir + "outline_ps_gen_data.frag");
    m_generate_data_outline_shader->link();

    m_outline_ps_shader = std::make_shared<RapidGL::Shader>(dir + "outline_ps.vert", dir + "outline_ps.frag");
    m_outline_ps_shader->link();
}

void ToonOutline::input()
{
    /* Close the application when Esc is released. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Escape))
    {
        stop();
    }

    /* It's also possible to take a screenshot. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "05_toon_outline";
        if (take_screenshot_png(filename, 400, 300))
        {
            /* If specified folders in the path are not already created, they'll be created automagically. */
            std::cout << "Saved " << filename << ".png to " << RapidGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
        else
        {
            std::cerr << "Could not save " << filename << ".png to " << RapidGL::FileSystem::getPath("../screenshots/") << std::endl;
        }
    }
}

void ToonOutline::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void ToonOutline::render()
{
    if (m_outline_method == OutlineMethod::STENCIL)
    {
        stencil_outline();
    }

    if (m_outline_method == OutlineMethod::POSTPROCESS)
    {
        ps_outline();
    }
}

void ToonOutline::stencil_outline()
{
    /* 1st pass */
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    render_toon_shaded_objects();

    /* 2nd pass */
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);

    /* render outline */
    m_stencil_outline_shader->bind();
    m_stencil_outline_shader->setUniform("outline_width", 0.1f * m_stencil_outline_width);
    m_stencil_outline_shader->setUniform("outline_color", m_outline_color);
    m_stencil_outline_shader->setUniform("screen_resolution", RapidGL::Window::getSize());

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        auto normal_matrix = glm::transpose(glm::inverse(m_objects_model_matrices[i]));

        m_stencil_outline_shader->setUniform("mvp",        view_projection * m_objects_model_matrices[i]);
        m_stencil_outline_shader->setUniform("mvp_normal", glm::mat3(view_projection * normal_matrix));

        m_objects[i]->render(m_stencil_outline_shader, false);
    }

    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glEnable(GL_DEPTH_TEST);
}

void ToonOutline::ps_outline()
{
    /* Render normals and depth to a RGBA texture */
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_normal_depth);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_generate_data_outline_shader->bind();

    auto view       = m_camera->m_view;
    auto projection = m_camera->m_projection;

    m_generate_data_outline_shader->setUniform("projection", projection);

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        auto normal_matrix = glm::transpose(glm::inverse(view * m_objects_model_matrices[i]));

        m_generate_data_outline_shader->setUniform("view_model",               view * m_objects_model_matrices[i]);
        m_generate_data_outline_shader->setUniform("normal_matrix_view_space", glm::mat3(normal_matrix));

        m_objects[i]->render(m_generate_data_outline_shader, false);
    }

    /* Render shading to a RGBA texture */
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_shading);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    render_toon_shaded_objects();

    /* Compose outlines and shading */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_outline_ps_shader->bind();
    m_outline_ps_shader->setUniform("clip_to_view", glm::inverse(projection));
    m_outline_ps_shader->setUniform("outline_width", m_ps_outline_width);
    m_outline_ps_shader->setUniform("outline_color", m_outline_color);

    m_outline_ps_shader->setUniform("depth_threshold",              m_depth_threshold);
    m_outline_ps_shader->setUniform("depth_normal_threshold",       m_depth_normal_threshold);
    m_outline_ps_shader->setUniform("depth_normal_threshold_scale", m_depth_normal_threshold);
    m_outline_ps_shader->setUniform("normal_threshold",             m_normal_threshold);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_normals_depth_tex_buffer);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_shading_tex_buffer);

    glBindVertexArray(m_ps_vao_id);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ToonOutline::render_toon_shaded_objects()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int toon_shader_id = int(m_toon_shading_method);

    m_toon_shaders[toon_shader_id]->bind();
    m_toon_shaders[toon_shader_id]->setUniform("light_color", m_light_color);
    m_toon_shaders[toon_shader_id]->setUniform("light_direction", m_light_direction);
    m_toon_shaders[toon_shader_id]->setUniform("light_intensity", m_light_intensity);
    m_toon_shaders[toon_shader_id]->setUniform("ambient_factor", m_ambient_factor);

    m_toon_shaders[toon_shader_id]->setUniform("cam_pos", m_camera->position());
    m_toon_shaders[toon_shader_id]->setUniform("specular_power", m_specular_power);
    m_toon_shaders[toon_shader_id]->setUniform("specular_intensity", m_specular_intensity);
    m_toon_shaders[toon_shader_id]->setUniform("gamma", m_gamma);

    if (m_toon_shading_method == ToonShadingMethod::SIMPLE)
    {
        m_toon_shaders[toon_shader_id]->setUniform("diffuse_levels", m_simple_toon_diffuse_levels);
        m_toon_shaders[toon_shader_id]->setUniform("specular_levels", m_simple_toon_specular_levels);
    }

    if (m_toon_shading_method == ToonShadingMethod::ADVANCED)
    {
        m_toon_shaders[toon_shader_id]->setUniform("A", m_advanced_toon_A);
        m_toon_shaders[toon_shader_id]->setUniform("B", m_advanced_toon_B);
        m_toon_shaders[toon_shader_id]->setUniform("C", m_advanced_toon_C);
        m_toon_shaders[toon_shader_id]->setUniform("D", m_advanced_toon_D);
    }

    if (m_toon_shading_method == ToonShadingMethod::SIMPLE_RIM)
    {
        m_toon_shaders[toon_shader_id]->setUniform("rim_color", m_rim_color);
        m_toon_shaders[toon_shader_id]->setUniform("rim_threshold", m_rim_threshold);
        m_toon_shaders[toon_shader_id]->setUniform("rim_amount", m_rim_amount);
    }

    if (m_toon_shading_method == ToonShadingMethod::TWIN_SHADE)
    {
        m_toon_shaders[toon_shader_id]->setUniform("diffuse_levels", m_twin_shade_toon_diffuse_levels);
        m_toon_shaders[toon_shader_id]->setUniform("specular_levels", m_twin_shade_toon_specular_levels);
        m_toon_shaders[toon_shader_id]->setUniform("light_shade_cutoff", m_twin_shade_light_shade_cutoff);
        m_toon_shaders[toon_shader_id]->setUniform("dark_shade_cutoff", m_twin_shade_dark_shade_cutoff);
    }

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_toon_shaders[toon_shader_id]->setUniform("model", m_objects_model_matrices[i]);
        m_toon_shaders[toon_shader_id]->setUniform("normal_matrix", glm::mat3(glm::transpose(glm::inverse(m_objects_model_matrices[i]))));
        m_toon_shaders[toon_shader_id]->setUniform("mvp", view_projection * m_objects_model_matrices[i]);
        m_toon_shaders[toon_shader_id]->setUniform("object_color", m_objects_colors[i]);
        m_objects[i]->render(m_toon_shaders[toon_shader_id]);
    }
}

void ToonOutline::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

/* Create your own GUI using ImGUI here. */
    ImVec2 window_pos       = ImVec2(RapidGL::Window::getWidth() - 10.0, 10.0);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 400, 0 });

    ImGui::Begin("Info");
    {
        if (ImGui::CollapsingHeader("Help"))
        {
            ImGui::Text("Controls info: \n\n"
                        "F1     - take a screenshot\n"
                        "WASDQE - control camera movement\n"
                        "RMB    - toggle cursor lock and rotate camera\n"
                        "Esc    - close the app\n\n");
        }

        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);

        ImGui::Spacing();

        if (ImGui::BeginCombo("Toon shading method", m_toon_shading_methods_names[int(m_toon_shading_method)].c_str() ))
        {
            for (int i = 0; i < m_toon_shading_methods_names.size(); ++i)
            {
                bool is_selected = (m_toon_shading_method == ToonShadingMethod(i));
                if (ImGui::Selectable(m_toon_shading_methods_names[i].c_str(), is_selected))
                {
                    m_toon_shading_method = ToonShadingMethod(i);
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Outline rendering method", m_outline_methods_names[int(m_outline_method)].c_str()))
        {
            for (int i = 0; i < m_outline_methods_names.size(); ++i)
            {
                bool is_selected = (m_outline_method == OutlineMethod(i));
                if (ImGui::Selectable(m_outline_methods_names[i].c_str(), is_selected))
                {
                    m_outline_method = OutlineMethod(i);

                    switch (m_outline_method)
                    {
                        case OutlineMethod::STENCIL:
                            glEnable(GL_STENCIL_TEST);
                            break;
                        case OutlineMethod::POSTPROCESS:
                            glDisable(GL_STENCIL_TEST);
                            break;
                    }
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::Spacing();

        if (m_toon_shading_method == ToonShadingMethod::SIMPLE)
        {
            ImGui::Text("Simple Toon shader properties\n");

            ImGui::SliderFloat("Diffuse levels",  &m_simple_toon_diffuse_levels,  1.0, 10.0, "%.0f");
            ImGui::SliderFloat("Specular levels", &m_simple_toon_specular_levels, 1.0, 10.0, "%.0f");
        }

        if (m_toon_shading_method == ToonShadingMethod::ADVANCED)
        {
            ImGui::Text("Advanced Toon shader properties\n");

            ImGui::SliderFloat("Threshold A", &m_advanced_toon_A, 0.01, 1.0, "%.2f");
            ImGui::SliderFloat("Threshold B", &m_advanced_toon_B, 0.01, 1.0, "%.2f");
            ImGui::SliderFloat("Threshold C", &m_advanced_toon_C, 0.01, 1.0, "%.2f");
            ImGui::SliderFloat("Threshold D", &m_advanced_toon_D, 0.01, 1.0, "%.2f");
        }

        if (m_toon_shading_method == ToonShadingMethod::SIMPLE_RIM)
        {
            ImGui::Text("Simple Toon wirh Rim shader properties\n");

            ImGui::ColorEdit3 ("Rim color",     &m_rim_color.x);
            ImGui::SliderFloat("Rim threshold", &m_rim_threshold, 0.0, 1.0, "%.2f");
            ImGui::SliderFloat("Rim amount",    &m_rim_amount,    0.0, 1.0, "%.2f");
        }

        if (m_toon_shading_method == ToonShadingMethod::TWIN_SHADE)
        {
            ImGui::Text("Twin Shade Toon shader properties\n");

            ImGui::SliderFloat("Diffuse levels",     &m_twin_shade_toon_diffuse_levels,  1.0, 10.0, "%.0f");
            ImGui::SliderFloat("Specular levels",    &m_twin_shade_toon_specular_levels, 1.0, 10.0, "%.0f");
            ImGui::SliderFloat("Light shade cutoff", &m_twin_shade_light_shade_cutoff,   0.0, 1.0,  "%.2f");
            ImGui::SliderFloat("Dark shade cutoff",  &m_twin_shade_dark_shade_cutoff,    0.0, 1.0,  "%.2f");
        }

        ImGui::SliderFloat("Gamma",         &m_gamma, 0.0, 10.0, "%.1f");
        ImGui::ColorEdit3 ("Outline color", &m_outline_color[0]);

        if (m_outline_method == OutlineMethod::STENCIL)
        {
            ImGui::SliderFloat("Outline width", &m_stencil_outline_width, 0.0, 100.0, "%.1f");
        }

        if (m_outline_method == OutlineMethod::POSTPROCESS)
        {
            ImGui::SliderFloat("Outline width",                &m_ps_outline_width,             0.0, 10.0, "%.0f");
            ImGui::SliderFloat("Depth threshold",              &m_depth_threshold,              0.0, 10.0, "%.1f");
            ImGui::SliderFloat("Depth-Normal threshold",       &m_depth_normal_threshold,       0.0, 1.0,  "%.2f");
            ImGui::SliderFloat("Depth-Normal threshold scale", &m_depth_normal_threshold_scale, 0.0, 10.0, "%.1f");
            ImGui::SliderFloat("Normal threshold",             &m_normal_threshold,             0.0, 1.0,  "%.2f");
        }
        
        ImGui::Separator();

        ImGui::Spacing();
        ImGui::Text("Light's properties\n");

        ImGui::SliderFloat("Ambient color", &m_ambient_factor, 0.0, 1.0, "%.2f");

        ImGui::Spacing();

        ImGui::ColorEdit3 ("Light color",        &m_light_color[0]);
        ImGui::SliderFloat("Light intensity",    &m_light_intensity,    0.0, 10.0,  "%.1f");
        ImGui::SliderFloat("Specular power",     &m_specular_power,     1.0, 120.0, "%.0f");
        ImGui::SliderFloat("Specular intensity", &m_specular_intensity, 0.0, 1.0,   "%.2f");

        if (ImGui::SliderFloat2("Azimuth and Elevation", &m_dir_light_azimuth_elevation_angles[0], -180.0, 180.0, "%.1f"))
        {
            m_light_direction = calcDirection(m_dir_light_azimuth_elevation_angles);
        }        

        ImGui::PopItemWidth();
    }
    ImGui::End();
}
