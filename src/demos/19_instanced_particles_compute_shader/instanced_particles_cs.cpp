#include "instanced_particles_cs.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>

InstancedParticlesCS::InstancedParticlesCS()
    : m_emitter_pos                       (0.0,  0.0, 0.0),
      m_emitter_dir                       (0.0,  1.0, 0.0),
      m_acceleration                      (0.0, -0.4, 0.0),
      m_particle_lifetime                 (10.0f),
      m_delta_time                        (0.0f),
      m_start_position_min_max            (0.0f),
      m_start_velocity_min_max            (1.25, 1.5),
      m_start_rotational_velocity_min_max (-15, 15),
      m_direction_constraints             (1, 1, 1),
      m_cone_angle                        (glm::degrees(glm::pi<float>() / 8.0f)),
      m_total_particles                   (64 * 1000),
      m_particles_color                   (1, 0.474, 0.058)
{
}

InstancedParticlesCS::~InstancedParticlesCS()
{
    glDeleteBuffers(1, &m_pos_vbo_id);
    glDeleteBuffers(1, &m_velocity_vbo_id);
    glDeleteBuffers(1, &m_age_vbo_id);
    glDeleteBuffers(1, &m_rotation_vbo_id);
}

void InstancedParticlesCS::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    std::cout << "Number of particles: " << m_total_particles << std::endl;

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(4.0, 1.5, -4.0);
    m_camera->setOrientation(5.0f, 225.0f, 0.0f);
    m_camera->update(0.0);

    /* Create objects */
    m_instanced_model.GenTorus(0.03, 0.07, 20, 20);
    m_grid_model.GenPlaneGrid(20, 20, 20, 20);

    /* Create shader. */
    std::string dir = "../src/demos/02_simple_3d/";
    m_simple_shader = std::make_shared<RGL::Shader>(dir + "simple_3d.vert", dir + "simple_3d.frag");
    m_simple_shader->link();

    dir = "../src/demos/19_instanced_particles_compute_shader/";
    m_particles_render_shader = std::make_shared<RGL::Shader>(dir + "particles.vert", dir + "particles.frag");
    m_particles_render_shader->link();

    m_particles_compute_shader = std::make_shared<RGL::Shader>(dir + "particles.comp");
    m_particles_compute_shader->link();

    /* Generate initial data for the particles */
    std::vector<glm::vec4> initial_positions (m_total_particles, glm::vec4(0.0, 0.0, 0.0, 1.0));
    std::vector<glm::vec4> initial_velocities(m_total_particles, glm::vec4(0.0f));
    std::vector<float>     initial_ages      (m_total_particles);
    std::vector<glm::vec2> initial_rotations (m_total_particles, glm::vec2(0.0));

    /* Fill the first age buffer */
    double rate = m_particle_lifetime / m_total_particles;

    for (uint64_t i = 0; i < m_total_particles; ++i)
    {
        initial_ages[i] = rate * int64_t(i - m_total_particles);
    }

    auto rng = std::default_random_engine{};
    std::shuffle(initial_ages.begin(), initial_ages.end(), rng);

    /* Create and allocate all the buffers for the particle system */
    glGenBuffers(1, &m_pos_vbo_id);
    glGenBuffers(1, &m_velocity_vbo_id);
    glGenBuffers(1, &m_age_vbo_id);
    glGenBuffers(1, &m_rotation_vbo_id);

    /* Set up SSBOs */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_pos_vbo_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initial_positions.size() * sizeof(initial_positions[0]), initial_positions.data(), GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_velocity_vbo_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initial_velocities.size() * sizeof(initial_velocities[0]), initial_velocities.data(), GL_DYNAMIC_COPY);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_age_vbo_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initial_ages.size() * sizeof(initial_ages[0]), initial_ages.data(), GL_DYNAMIC_COPY);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_rotation_vbo_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, initial_rotations.size() * sizeof(initial_rotations[0]), initial_rotations.data(), GL_DYNAMIC_DRAW);

    /* Add new VBOs to mesh's VAO */
    m_instanced_model.AddAttributeBuffer(4 /* attrib index */, 
                                         4 /* binding index */, 
                                         4 /* format size */, 
                                         GL_FLOAT /* data type */, 
                                         m_pos_vbo_id /* buffer id */, 
                                         sizeof(glm::vec4) /* stride */,
                                         1 /* divisor */);

    m_instanced_model.AddAttributeBuffer(5 /* attrib index */, 
                                         5 /* binding index */, 
                                         2 /* format size */, 
                                         GL_FLOAT /* data type */, 
                                         m_rotation_vbo_id /* buffer id */, 
                                         sizeof(glm::vec2) /* stride */,
                                         1 /* divisor */);

    glBindVertexArray(0);
}

void InstancedParticlesCS::input()
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
        std::string filename = "19_instanced_particles_compute_shader";
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
}

void InstancedParticlesCS::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
    m_delta_time = delta_time;
}

void InstancedParticlesCS::render()
{
    /* Execute the compute shader */
    m_particles_compute_shader->bind();
    m_particles_compute_shader->setUniform("u_particle_lifetime",                 m_particle_lifetime);
    m_particles_compute_shader->setUniform("u_emitter_world_pos",                 m_emitter_pos);
    m_particles_compute_shader->setUniform("u_emitter_basis",                     make_arbitrary_basis(m_emitter_dir));
    m_particles_compute_shader->setUniform("u_delta_t",                           m_delta_time);
    m_particles_compute_shader->setUniform("u_acceleration",                      m_acceleration);
    m_particles_compute_shader->setUniform("u_start_position_min_max",            m_start_position_min_max);
    m_particles_compute_shader->setUniform("u_start_velocity_min_max",            m_start_velocity_min_max);
    m_particles_compute_shader->setUniform("u_start_rotational_velocity_min_max", m_start_rotational_velocity_min_max); 
    m_particles_compute_shader->setUniform("u_direction_constraints",             m_direction_constraints);
    m_particles_compute_shader->setUniform("u_cone_angle",                        glm::radians(m_cone_angle));
    m_particles_compute_shader->setUniform("u_random",                            RGL::Util::RandomVec3(0, 1));

    glDispatchCompute(std::ceilf((float)m_total_particles / 1024.0f), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    /* Draw the scene */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw the grid */
    m_simple_shader->bind();
    m_simple_shader->setUniform("mvp", m_camera->m_projection * m_camera->m_view);
    m_simple_shader->setUniform("color", glm::vec3(0.4));
    m_simple_shader->setUniform("mix_factor", 1.0f);

    m_grid_model.Render();

    m_simple_shader->setUniform("color", m_particles_color);

    /* Draw the particles */
    m_particles_render_shader->bind();
    m_particles_render_shader->setUniform("u_mvp",        m_camera->m_projection * m_camera->m_view);
    m_particles_render_shader->setUniform("u_model_view", m_camera->m_view);
    m_particles_render_shader->setUniform("u_diffuse",    m_particles_color);
    m_instanced_model.Render(m_total_particles);
}

void InstancedParticlesCS::render_gui()
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
            static std::string label = "Number of particles: " + std::to_string(m_total_particles);
            ImGui::Text(label.c_str());
            ImGui::Spacing();

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
            ImGui::ColorEdit3  ("Particles color",                   &m_particles_color[0]);
            ImGui::SliderFloat3("Particles acceleration",            &m_acceleration[0],                      -10.0f, 10.0f,  "%.1f");
            ImGui::SliderFloat ("Particle lifetime",                 &m_particle_lifetime,                     0.1f,  20.0f,  "%.1f");
            ImGui::SliderFloat2("Start position min max",            &m_start_position_min_max[0],            -5.0f,  5.0f,   "%.1f");
            ImGui::SliderFloat2("Start velocity min max",            &m_start_velocity_min_max[0],            -5.0f,  5.0f,   "%.1f");
            ImGui::SliderFloat2("Start rotational velocity min max", &m_start_rotational_velocity_min_max[0], -15.0f, 15.0f, "%.1f");
            ImGui::SliderFloat3("Direction constraints",             &m_direction_constraints[0],              0.0f,  1.0f,   "%1.0f");
            ImGui::SliderFloat ("Cone angle [deg]",                  &m_cone_angle,                            0.0f,  180.0f, "%.1f");

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
                m_start_position_min_max    = glm::vec2(0);
                m_start_velocity_min_max    = glm::vec2(1.25, 1.5);
                m_direction_constraints     = glm::vec3(1, 1, 1);
                m_cone_angle                = glm::degrees(glm::pi<float>() / 8.0f);

                reset_particles_buffers();
            }

            ImGui::SameLine();
            if (ImGui::Button("Fire preset"))
            {
                m_acceleration              = glm::vec3(0, 0.1, 0.0);
                m_particle_lifetime         = 3.0f;
                m_start_position_min_max    = glm::vec2(-2, 2);
                m_start_velocity_min_max    = glm::vec2(0.1, 0.5);
                m_direction_constraints     = glm::vec3(0, 1, 0);
                m_cone_angle                = 0.0f;

                reset_particles_buffers();
            }

            ImGui::SameLine();
            if (ImGui::Button("Smoke preset"))
            {
                m_acceleration              = glm::vec3(0, 0.1, 0.0);
                m_particle_lifetime         = 10.0f;
                m_start_position_min_max    = glm::vec2(0, 0);
                m_start_velocity_min_max    = glm::vec2(0.1, 0.2);
                m_direction_constraints     = glm::vec3(1, 1, 1);
                m_cone_angle                = glm::degrees(glm::pi<float>() / 1.5f);

                reset_particles_buffers();
            }
        }
        ImGui::PopItemWidth();
        ImGui::Spacing();
    }
    ImGui::End();
}

void InstancedParticlesCS::reset_particles_buffers()
{
    std::vector<float> initial_ages(m_total_particles);
    double rate = m_particle_lifetime / m_total_particles;

    for (uint64_t i = 0; i < m_total_particles; ++i)
    {
        initial_ages[i] = rate * int64_t(i - m_total_particles);
    }

    auto rng = std::default_random_engine{};
    std::shuffle(initial_ages.begin(), initial_ages.end(), rng);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_age_vbo_id);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, initial_ages.size() * sizeof(initial_ages[0]), initial_ages.data());
}
