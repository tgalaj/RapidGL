#include "noise.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"
#include "glm/gtc/noise.hpp"

ProceduralNoise::ProceduralNoise()
    : m_sky_color       (glm::vec3(77, 140, 230) / 255.0f),
      m_cloud_color     (1.0),
      m_dark_wood_color (0.8, 0.5, 0.1),
      m_light_wood_color(1.0, 0.75, 0.25),
      m_slice_matrix    (glm::mat4(1.0f)),
      m_low_threshold   (0.45f),
      m_high_threshold  (0.65f)
{
    m_slice_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(10.0f), glm::vec3(1.0, 0.0, 0.0));
    m_slice_matrix = glm::rotate(m_slice_matrix, glm::radians(-20.0f), glm::vec3(0.0, 0.0, 1.0));
    m_slice_matrix = glm::scale(m_slice_matrix, glm::vec3(50.0, 50.0, 1.0));
    m_slice_matrix = glm::translate(m_slice_matrix, glm::vec3(-0.5, -0.75, 2.0));
}

ProceduralNoise::~ProceduralNoise()
{
}

RGL::Texture ProceduralNoise::gen_perlin_data(uint32_t width, uint32_t height, float base_frequency, float persistance, bool periodic)
{
    RGL::Texture texture;
    texture.m_type = "texture_diffuse";

    float* data = new float[width * height * 4];

    float x_factor = 1.0f / (width  - 1);
    float y_factor = 1.0f / (height - 1);

    for(uint32_t row = 0; row < height; ++row)
    {
        for(uint32_t col = 0; col < width; ++col)
        {
            float x       = x_factor * col;
            float y       = y_factor * row;
            float sum     = 0.0f;
            float freq    = base_frequency;
            float persist = persistance;

            // Compute the sum for each octave
            for(uint32_t oct = 0; oct < 4; oct++)
            {
                glm::vec2 p(x * freq, y * freq);

                float val = 0.0f;
                if (periodic)
                {
                    val = glm::perlin(p, glm::vec2(freq)) * persist;
                }
                else
                {
                    val = glm::perlin(p) * persist;
                }

                sum += val;

                float result = (sum + 1.0f) / 2.0f;
                result = glm::clamp(result, 0.0f, 1.0f);

                data[((row * width + col) * 4) + oct] = result;

                freq *= 2.0f;
                persist *= persistance;
            }
        }
    }

    glGenTextures(1, &texture.m_id);
    glBindTexture(GL_TEXTURE_2D, texture.m_id);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] data;

    return texture;
}

void ProceduralNoise::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(0.0, 0.5, 3.0);

    /* Create models. */
    for (unsigned i = 0; i < 3; ++i)
    {
        m_objects.emplace_back(std::make_shared<RGL::Model>());
    }

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0]->load(RGL::FileSystem::getPath("models/teapot.obj"));
    m_objects[1]->genPlane(3, 3, 2, 2);
    m_objects[2]->genPlane(3, 3, 1, 1);

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-4.0, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(-135.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.15f))); // suzanne
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f),  glm::vec3(1, 0, 0)));                                                  // plane1
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 4.0, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f),  glm::vec3(1, 0, 0)));                                                  // plane2

    /* Add texture to the monkey and sphere models only. */
    uint32_t perlin_tex_width  = 128;
    uint32_t perlin_tex_height = 128;

    RGL::Texture cloud_texture      = gen_perlin_data(perlin_tex_width, perlin_tex_height, 3.0f);
    RGL::Texture wood_grain_texture = gen_perlin_data(perlin_tex_width, perlin_tex_height, 4.0f);
    RGL::Texture decal_texture      = gen_perlin_data(perlin_tex_width, perlin_tex_height, 12.0f);

    m_objects[0]->getMesh(0).addTexture(decal_texture);
    m_objects[1]->getMesh(0).addTexture(cloud_texture);
    m_objects[2]->getMesh(0).addTexture(wood_grain_texture);

    /* Create shader. */
    std::string dir = "../src/demos/16_noise/";
    m_noise_texturing_shader = std::make_shared<RGL::Shader>(dir + "simple.vert", dir + "noise.frag");
    m_noise_texturing_shader->link();
}

void ProceduralNoise::input()
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
        std::string filename = "16_noise";
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

void ProceduralNoise::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void ProceduralNoise::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_noise_texturing_shader->bind();

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    // Decal
    m_noise_texturing_shader->setSubroutine(RGL::Shader::ShaderType::FRAGMENT, "disintegration");
    m_noise_texturing_shader->setUniform("low_threshold", m_low_threshold);
    m_noise_texturing_shader->setUniform("high_threshold", m_high_threshold);
    m_noise_texturing_shader->setUniform("mvp", view_projection * m_objects_model_matrices[0]);
    m_objects[0]->render(m_noise_texturing_shader);

    // Cloud
    m_noise_texturing_shader->setSubroutine(RGL::Shader::ShaderType::FRAGMENT, "cloud");
    m_noise_texturing_shader->setUniform("sky_color", m_sky_color);
    m_noise_texturing_shader->setUniform("cloud_color", m_cloud_color);
    m_noise_texturing_shader->setUniform("mvp", view_projection * m_objects_model_matrices[1]);
    m_objects[1]->render(m_noise_texturing_shader);

    // Wood grain
    m_noise_texturing_shader->setSubroutine(RGL::Shader::ShaderType::FRAGMENT, "wood_grain");
    m_noise_texturing_shader->setUniform("dark_wood_color", m_dark_wood_color);
    m_noise_texturing_shader->setUniform("light_wood_color", m_light_wood_color);
    m_noise_texturing_shader->setUniform("slice_matrix", m_slice_matrix);
    m_noise_texturing_shader->setUniform("mvp", view_projection * m_objects_model_matrices[2]);
    m_objects[2]->render(m_noise_texturing_shader);
}

void ProceduralNoise::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos = ImVec2(RGL::Window::getWidth() - 10.0, 10.0);
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

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Noise properties", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Decal"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::SliderFloat("Low threshold",  &m_low_threshold,  0, 1, "%.2f");
                    ImGui::SliderFloat("High threshold", &m_high_threshold, 0, 1, "%.2f");
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Cloud"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3("Sky color",   &m_sky_color[0]);
                    ImGui::ColorEdit3("Cloud color", &m_cloud_color[0]);
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Wood grain"))
            {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
                {
                    ImGui::ColorEdit3("Dark wood color",  &m_dark_wood_color[0]);
                    ImGui::ColorEdit3("Light wood color", &m_light_wood_color[0]);
                }
                ImGui::PopItemWidth();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
