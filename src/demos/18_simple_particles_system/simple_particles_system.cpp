#include "simple_particles_system.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>

SimpleParticlesSystem::SimpleParticlesSystem()
    : m_draw_buffer_idx(1),
      m_emitter_pos(1, 0, 0),
      m_emitter_dir(-1, 2, 0),
      m_acceleration(0, -0.5, 0),
      m_no_particles      (4000),
      m_particle_lifetime (0.6f),
      m_particle_size(2.05),
      m_delta_time(0.0f),
      m_specular_power    (120.0f),
      m_specular_intenstiy(0.0f),
      m_dir_light_angles  (67.5f),
      m_ambient_color     (0.18f)

{
}

SimpleParticlesSystem::~SimpleParticlesSystem()
{
    glDeleteBuffers(2, m_pos_vbo_ids);
    glDeleteBuffers(2, m_velocity_vbo_ids);
    glDeleteBuffers(2, m_age_vbo_ids);
    glDeleteVertexArrays(2, m_vao_ids);
    glDeleteTransformFeedbacks(2, m_tfo_ids);
    glDeleteTextures(1, &m_random_texture_1d);
    glDeleteTextures(1, &m_particle_texture);
}

void SimpleParticlesSystem::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(-3.0, 1.5, 10.0);
    m_camera->setOrientation(5.0f, 20.0f, 0.0f);

    /* Initialize lights' properties */
    m_dir_light_properties.color = glm::vec3(1.0f);
    m_dir_light_properties.intensity = 0.8f;
    m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);

    /* Create shader. */
    std::string dir = "../src/demos/18_simple_particles_system/";
    m_particles_shader = std::make_shared<RapidGL::Shader>(dir + "particles.vert", dir + "particles.frag");

    const std::vector<const char*> tf_output_names = { "tf_pos", "tf_velocity", "tf_age" };
    m_particles_shader->setTransformFeedbackVaryings(tf_output_names, GL_SEPARATE_ATTRIBS);

    m_particles_shader->link();

    /* Create and allocate all the buffers for the particle system */
    glGenBuffers(2, m_pos_vbo_ids);
    glGenBuffers(2, m_velocity_vbo_ids);
    glGenBuffers(2, m_age_vbo_ids);

    uint64_t size = m_no_particles * 3 * sizeof(GLfloat);
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, m_pos_vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, m_velocity_vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, m_velocity_vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[0]);
    glBufferData(GL_ARRAY_BUFFER, m_no_particles * sizeof(float), 0, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[1]);
    glBufferData(GL_ARRAY_BUFFER, m_no_particles * sizeof(float), 0, GL_DYNAMIC_COPY);

    /* Fill the first age buffer */
    std::vector<GLfloat> particle_ages(m_no_particles);
    float rate = m_particle_lifetime / m_no_particles;

    for (int i = 0; i < m_no_particles; ++i)
    {
        particle_ages[i] = rate * (i - m_no_particles);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_age_vbo_ids[0]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_no_particles * sizeof(float), particle_ages.data());
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
        rand_velocities[i] = RapidGL::Util::randomDouble(0.0, 1.0);
    }

    glGenTextures(1, &m_random_texture_1d);
    glBindTexture(GL_TEXTURE_1D, m_random_texture_1d);
    glTexStorage1D(GL_TEXTURE_1D, 1, GL_R32F, size);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, size, GL_RED, GL_FLOAT, rand_velocities.data());

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_1D, 0);

    /* Create particle texture */
    m_particle_texture = RapidGL::Util::loadGLTexture2D("bluewater.png", "textures/particles", false);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SimpleParticlesSystem::input()
{
    /* Close the application when Esc is released. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Escape))
    {
        stop();
    }

    /* Toggle between wireframe and solid rendering */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::F2))
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
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::F1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "18_simple_particles_system";
        if (take_screenshot_png(filename, RapidGL::Window::getWidth() / 2.0, RapidGL::Window::getHeight() / 2.0))
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

void SimpleParticlesSystem::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
    m_delta_time += delta_time;
}

void SimpleParticlesSystem::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Update pass */
    m_particles_shader->bind();
    m_particles_shader->setSubroutine(RapidGL::Shader::ShaderType::VERTEX, "update");

    glBindTexture(GL_TEXTURE_1D, m_random_texture_1d);
    glBindTextureUnit(1, m_random_texture_1d);

    glBindTexture(GL_TEXTURE_2D, m_particle_texture);
    glBindTextureUnit(0, m_particle_texture);

    m_particles_shader->setUniform("delta_t", m_delta_time);
    m_particles_shader->setUniform("acceleration", m_acceleration);
    m_particles_shader->setUniform("particle_lifetime", m_particle_lifetime);
    m_particles_shader->setUniform("emitter_world_pos", m_emitter_pos);
    m_particles_shader->setUniform("emitter_basis", make_arbitrary_basis(m_emitter_dir));
    m_particles_shader->setUniform("model_view", m_camera->m_view * glm::mat4(1.0f));
    m_particles_shader->setUniform("projection", m_camera->m_projection);
    m_particles_shader->setUniform("particle_size", m_particle_size);
    m_particles_shader->setUniform("particle_lifetime", m_particle_lifetime);

    glEnable(GL_RASTERIZER_DISCARD); // Turn off rasterization
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo_ids[m_draw_buffer_idx]);
    glBeginTransformFeedback(GL_POINTS);

    glBindVertexArray(m_vao_ids[1 - m_draw_buffer_idx]);
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);
    glDrawArrays(GL_POINTS, 0, m_no_particles);
    glBindVertexArray(0);

    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD); // Turn on rasterization

    /* Render pass */
    m_particles_shader->setSubroutine(RapidGL::Shader::ShaderType::VERTEX, "render");

    glDepthMask(GL_FALSE);
    glBindVertexArray(m_vao_ids[m_draw_buffer_idx]);
    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_no_particles);
    glBindVertexArray(0);
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
                        "F2     - toggle wireframe rendering\n"
                        "WASDQE - control camera movement\n"
                        "RMB    - toggle cursor lock and rotate camera\n"
                        "Esc    - close the app\n\n");
        }

        ImGui::Spacing();

        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);

        ImGui::PopItemWidth();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Lights' properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Directional"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3 ("Light color",        &m_dir_light_properties.color[0]);
                    ImGui::SliderFloat("Light intensity",    &m_dir_light_properties.intensity, 0.0, 10.0,  "%.1f");
                    ImGui::SliderFloat("Specular power",     &m_specular_power.x,               1.0, 120.0, "%.0f");
                    ImGui::SliderFloat("Specular intensity", &m_specular_intenstiy.x,           0.0, 1.0,   "%.2f");

                    static float ambient_factor = m_ambient_color.r;
                    if (ImGui::SliderFloat("Ambient color", &ambient_factor, 0.0, 1.0, "%.2f"))
                    {
                        m_ambient_color = glm::vec3(ambient_factor);
                    }

                    if (ImGui::SliderFloat2("Azimuth and Elevation", &m_dir_light_angles[0], -180.0, 180.0, "%.1f"))
                    {
                        m_dir_light_properties.setDirection(m_dir_light_angles.x, m_dir_light_angles.y);
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
