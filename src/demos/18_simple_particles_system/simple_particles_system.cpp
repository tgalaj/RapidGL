#include "simple_particles_system.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>

SimpleParticlesSystem::SimpleParticlesSystem()
    : m_draw_buffer_idx           (1),
      m_emitter_pos               (0.0,  0.0, 0.0),
      m_emitter_dir               (0.0,  1.0, 0.0),
      m_acceleration              (0.0, -0.5, 0.0),
      m_no_particles              (8000),
      m_particle_lifetime         (10.0f),
      m_particle_size_min_max     (0.05f),
      m_particle_angle            (glm::half_pi<float>()),
      m_delta_time                (0.0f),
      m_should_fade_out_with_time (false),
      m_start_position_min_max    (0.0f),
      m_start_velocity_min_max    (1.25, 1.5),
      m_direction_constraints     (1, 1, 1),
      m_cone_angle                (glm::degrees(glm::pi<float>() / 8.0f))
{
}

SimpleParticlesSystem::~SimpleParticlesSystem()
{
    glDeleteBuffers           (2, m_pos_vbo_ids);
    glDeleteBuffers           (2, m_velocity_vbo_ids);
    glDeleteBuffers           (2, m_age_vbo_ids);
    glDeleteVertexArrays      (2, m_vao_ids);
    glDeleteTransformFeedbacks(2, m_tfo_ids);
    glDeleteTextures          (1, &m_random_texture_1d);
}

void SimpleParticlesSystem::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint max_width;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_width);
    max_width = max_width / 3;

    if(m_no_particles > max_width)
    {
        m_no_particles = max_width;
    }

    std::cout << "TF no. particles: " << m_no_particles << std::endl;

    /* Set file browser properties */
    m_file_dialog.SetTitle("Load texture");
    m_file_dialog.SetTypeFilters({ ".png" });
    m_file_dialog.SetPwd(RGL::FileSystem::getResourcesPath() / "textures/particles");

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(4.0, 1.5, -4.0);
    m_camera->setOrientation(5.0f, 225.0f, 0.0f);
    m_camera->update(0.0);

    /* Create objects */
    m_grid_model = std::make_shared<RGL::StaticModel>();
    m_grid_model->GenPlaneGrid(20, 20, 20, 20);

    /* Create shader. */
    std::string dir = "src/demos/02_simple_3d/";
    m_simple_shader = std::make_shared<RGL::Shader>(dir + "simple_3d.vert", dir + "simple_3d.frag");
    m_simple_shader->link();

    dir = "src/demos/18_simple_particles_system/";
    m_particles_shader = std::make_shared<RGL::Shader>(dir + "particles.vert", dir + "particles.frag");
    m_particles_shader->link();

    /* Create and allocate all the buffers for the particle system */
    glGenBuffers(2, m_pos_vbo_ids);
    glGenBuffers(2, m_velocity_vbo_ids);
    glGenBuffers(2, m_age_vbo_ids);

    uint64_t size = m_no_particles * 3 * sizeof(GLfloat);
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_velocity_vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_velocity_vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, m_no_particles * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, m_no_particles * sizeof(GLfloat), 0, GL_DYNAMIC_DRAW);

    /* Fill the first age buffer */
    std::vector<GLfloat> initial_particle_ages(m_no_particles);
    float rate = m_particle_lifetime / m_no_particles;

    for (int i = 0; i < m_no_particles; ++i)
    {
        initial_particle_ages[i] = rate * (i - m_no_particles);
    }

    auto rng = std::default_random_engine{};
    std::shuffle(initial_particle_ages.begin(), initial_particle_ages.end(), rng);

    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_no_particles * sizeof(float), initial_particle_ages.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Create VAOs for each set of buffers */
    glGenVertexArrays(2, m_vao_ids);

    /* Array 0 */
    glBindVertexArray(m_vao_ids[0]);

    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo_ids[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, m_velocity_vbo_ids[0]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[0]);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    /* Array 1 */
    glBindVertexArray(m_vao_ids[1]);

    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo_ids[1]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, m_velocity_vbo_ids[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[1]);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    /* Setup transform feedback objects */
    glGenTransformFeedbacks(2, m_tfo_ids);

    /* TF 0 */
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo_ids[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_pos_vbo_ids[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_velocity_vbo_ids[0]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, m_age_vbo_ids[0]);

    /* TF 1 */
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo_ids[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_pos_vbo_ids[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_velocity_vbo_ids[1]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, m_age_vbo_ids[1]);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    /* Create 1D random texture of particles velocities */
    size = m_no_particles * 3;
    std::vector<GLfloat> rand_velocities(size);

    for(int i = 0; i < rand_velocities.size(); ++i)
    {
        rand_velocities[i] = RGL::Util::RandomDouble(0.0, 1.0);
    }

    glCreateTextures(GL_TEXTURE_1D, 1, &m_random_texture_1d);

    glTextureStorage1D(m_random_texture_1d, 1, GL_R32F, size);
    glTextureSubImage1D(m_random_texture_1d, 0, 0, size, GL_RED, GL_FLOAT, rand_velocities.data());

    glTextureParameteri(m_random_texture_1d, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_random_texture_1d, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    /* Create particle texture */
    m_current_texture_filename = "bluewater.png";
    m_particle_texture.Load(RGL::FileSystem::getResourcesPath() / "textures/particles" / m_current_texture_filename);
}

void SimpleParticlesSystem::input()
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
        std::string filename = "18_simple_particles_system";
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
}

void SimpleParticlesSystem::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
    m_delta_time = delta_time;
}

void SimpleParticlesSystem::render()
{
    m_particles_shader->bind();
    m_particles_shader->setUniform("particle_lifetime",      m_particle_lifetime);
    m_particles_shader->setUniform("emitter_world_pos",      m_emitter_pos);
    m_particles_shader->setUniform("emitter_basis",          make_arbitrary_basis(m_emitter_dir));
    m_particles_shader->setUniform("delta_t",                m_delta_time);
    m_particles_shader->setUniform("acceleration",           m_acceleration);
    m_particles_shader->setUniform("model_view",             m_camera->m_view);
    m_particles_shader->setUniform("projection",             m_camera->m_projection);
    m_particles_shader->setUniform("particle_size_min_max",  m_particle_size_min_max);
    m_particles_shader->setUniform("should_keep_color",      !m_should_fade_out_with_time);
    m_particles_shader->setUniform("start_position_min_max", m_start_position_min_max);
    m_particles_shader->setUniform("start_velocity_min_max", m_start_velocity_min_max);
    m_particles_shader->setUniform("direction_constraints",  m_direction_constraints);
    m_particles_shader->setUniform("cone_angle",             glm::radians(m_cone_angle));

    /* Update pass */
    m_particles_shader->setSubroutine(RGL::Shader::ShaderType::VERTEX, "update");

    glEnable(GL_RASTERIZER_DISCARD); // Turn off rasterization
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo_ids[m_draw_buffer_idx]);
    glBeginTransformFeedback(GL_POINTS);

    glBindVertexArray(m_vao_ids[1 - m_draw_buffer_idx]);
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);
    glDrawArrays(GL_POINTS, 0, m_no_particles);

    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD); // Turn on rasterization

    /* Render pass */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw grid */
    m_simple_shader->bind();
    m_simple_shader->setUniform("mvp",        m_camera->m_projection * m_camera->m_view);
    m_simple_shader->setUniform("color",      glm::vec3(0.4));
    m_simple_shader->setUniform("mix_factor", 1.0f);

    m_grid_model->Render();

    /* Draw particles */
    m_particles_shader->bind();
    m_particles_shader->setSubroutine(RGL::Shader::ShaderType::VERTEX, "render");
    
    m_particle_texture.Bind(0);
    glBindTextureUnit(1, m_random_texture_1d);

    glDepthMask(GL_FALSE);
    glBindVertexArray(m_vao_ids[m_draw_buffer_idx]);
    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_no_particles);
    glDepthMask(GL_TRUE);

    /* Swap buffers */
    m_draw_buffer_idx = 1 - m_draw_buffer_idx;
}

void SimpleParticlesSystem::render_gui()
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
        {
            static glm::vec2 emitter_dir_angles = { 0.0, 0.0 };
            if (ImGui::SliderFloat2("Emitter azimuth and elevation", &emitter_dir_angles[0], -180.0, 180.0, "%.1f"))
            {
                float az = glm::radians(emitter_dir_angles.x);
                float el = glm::radians(emitter_dir_angles.y);

                m_emitter_dir.x = glm::sin(el) * glm::cos(az);
                m_emitter_dir.y = glm::cos(el);
                m_emitter_dir.z = glm::sin(el) * glm::sin(az);

                m_emitter_dir = glm::normalize(m_emitter_dir);
            }
            ImGui::SliderFloat3("Particles acceleration", &m_acceleration[0],           -10.0f, 10.0f,  "%.1f");
            ImGui::SliderFloat ("Particle lifetime",      &m_particle_lifetime,          0.1f,  20.0f,  "%.1f");
            ImGui::SliderFloat2("Particle size min max",  &m_particle_size_min_max[0],   0.01f, 5.0f,   "%.2f");
            ImGui::SliderFloat2("Start position min max", &m_start_position_min_max[0], -5.0f,  5.0f,   "%.1f");
            ImGui::SliderFloat2("Start velocity min max", &m_start_velocity_min_max[0], -5.0f,  5.0f,   "%.1f");
            ImGui::SliderFloat3("Direction constraints",  &m_direction_constraints[0],   0.0f,  1.0f,   "%1.0f");
            ImGui::SliderFloat ("Cone angle [deg]",       &m_cone_angle,                 0.0f,  180.0f, "%.1f");
            ImGui::Checkbox    ("Fade out with time",     &m_should_fade_out_with_time);

            ImGui::Spacing();

            ImGui::TextUnformatted(("Current texture: " + m_current_texture_filename).c_str());
            ImGui::SameLine();
            if (ImGui::Button("Load texture"))
            {
                m_file_dialog.Open();
            }

            ImGui::Spacing();

            if(ImGui::Button("Reset particles buffers"))
            {
                reset_particles_buffers();
            }

            ImGui::Spacing();

            if (ImGui::Button("Fountain preset"))
            {
                m_acceleration              = glm::vec3(0, -0.5, 0.0);
                m_particle_lifetime         = 10.0f;
                m_particle_size_min_max     = glm::vec2(0.05f);
                m_should_fade_out_with_time = false;
                m_start_position_min_max    = glm::vec2(0);
                m_start_velocity_min_max    = glm::vec2(1.25, 1.5);
                m_direction_constraints     = glm::vec3(1, 1, 1);
                m_cone_angle                = glm::degrees(glm::pi<float>() / 8.0f);

                m_current_texture_filename = "bluewater.png";
                m_particle_texture.Load(RGL::FileSystem::getResourcesPath() / "textures/particles" / m_current_texture_filename);

                reset_particles_buffers();
            }

            ImGui::SameLine();
            if (ImGui::Button("Fire preset"))
            {
                m_acceleration              = glm::vec3(0, 0.1, 0.0);
                m_particle_lifetime         = 3.0f;
                m_particle_size_min_max     = glm::vec2(0.5f);
                m_should_fade_out_with_time = true;
                m_start_position_min_max    = glm::vec2(-2, 2);
                m_start_velocity_min_max    = glm::vec2(0.1, 0.5);
                m_direction_constraints     = glm::vec3(0, 1, 0);
                m_cone_angle                = 0.0f;

                m_current_texture_filename = "fire.png";
                m_particle_texture.Load(RGL::FileSystem::getResourcesPath() / "textures/particles" / m_current_texture_filename);

                reset_particles_buffers();
            }

            ImGui::SameLine();
            if (ImGui::Button("Smoke preset"))
            {
                m_acceleration              = glm::vec3(0, 0.1, 0.0);
                m_particle_lifetime         = 10.0f;
                m_particle_size_min_max     = glm::vec2(0.1f, 2.5f);
                m_should_fade_out_with_time = false;
                m_start_position_min_max    = glm::vec2(0, 0);
                m_start_velocity_min_max    = glm::vec2(0.1, 0.2);
                m_direction_constraints     = glm::vec3(1, 1, 1);
                m_cone_angle                = glm::degrees(glm::pi<float>() / 1.5f);

                m_current_texture_filename = "smoke.png";
                m_particle_texture.Load(RGL::FileSystem::getResourcesPath() / "textures/particles" / m_current_texture_filename);

                reset_particles_buffers();
            }
        }
        ImGui::PopItemWidth();
        ImGui::Spacing();
    }
    ImGui::End();

    m_file_dialog.Display();
    if (m_file_dialog.HasSelected())
    {
        glFinish();
        std::string filepath = m_file_dialog.GetSelected().string();

        static std::string res_string = "resources\\";

        /* Get relative path with stripped resources\\ string. */
        auto pos = filepath.find(res_string);

        /* It is only allowed to load files from the RapidGL's resources directory. */
        if (pos != std::string::npos)
        {
            m_current_texture_filename = filepath.substr(pos + res_string.size(), filepath.size());

            /* Load texture data. */
            m_particle_texture.Load(RGL::FileSystem::getResourcesPath() / "textures/particles" / m_current_texture_filename);

            /* Strip relative path to get filename + extenstion only. */
            m_current_texture_filename = m_current_texture_filename.substr(m_current_texture_filename.find_last_of("\\") + 1, m_current_texture_filename.size());

            m_file_dialog.ClearSelected();
        }
        else
        {
            std::cerr << "ERROR: It is only allowed to load files from the RapidGL's ./resources directory!\n";
            m_file_dialog.ClearSelected();
        }
    }
}

void SimpleParticlesSystem::reset_particles_buffers()
{
    std::vector<GLfloat> particle_ages(m_no_particles);
    float rate = m_particle_lifetime / m_no_particles;

    for (int i = 0; i < m_no_particles; ++i)
    {
        particle_ages[i] = rate * (i - m_no_particles);
    }

    auto rng = std::default_random_engine{};
    std::shuffle(particle_ages.begin(), particle_ages.end(), rng);

    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_no_particles * sizeof(float), particle_ages.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_draw_buffer_idx = 1;
}
