#include "simple_3d.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

Simple3d::Simple3d()
    : m_mix_factor(1.0f)
{
}

Simple3d::~Simple3d()
{
}

void Simple3d::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 10.0);

    /* Create models. */
    for (unsigned i = 0; i < 8; ++i)
    {
        m_objects.emplace_back(std::make_shared<RapidGL::Model>());
    }

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0]->load(RapidGL::FileSystem::getPath("models/monkey.obj"));
    m_objects[1]->genCone(1.0, 0.5);
    m_objects[2]->genCube();
    m_objects[3]->genCylinder(1.0, 0.5);
    m_objects[4]->genPlane();
    m_objects[5]->genSphere(0.5);
    m_objects[6]->genTorus(0.5, 1.0);
    m_objects[7]->genQuad();

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-7.5, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0))); // monkey
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-5.0, 0.5, -5)));                                                                         // cone
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-2.5, 0.0, -5)));                                                                         // cube
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0, 0.0, -5)));                                                                         // cylinder
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 2.5, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1, 0, 0)));  // plane
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 5.0, 0.0, -5)));                                                                         // sphere
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 7.5, 0.0, -5)));                                                                         // torus
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(10.0, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1, 0, 0)));  // quad

    /* Set colors for individual models. */
    m_objects_colors.emplace_back(glm::vec3(1.0, 0.0, 0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 1.0, 0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.0, 1.0));
    m_objects_colors.emplace_back(glm::vec3(1.0, 1.0, 0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 1.0, 1.0));
    m_objects_colors.emplace_back(glm::vec3(1.0, 0.0, 1.0));
    m_objects_colors.emplace_back(glm::vec3(0.5, 0.0, 0.0));
    m_objects_colors.emplace_back(glm::vec3(0.0, 0.5, 0.0));

    /* Add texture to the monkey and sphere models only. */
    RapidGL::Texture texture;
    texture.m_id = RapidGL::Util::loadGLTexture("bricks.png", "textures", false);
    texture.m_type = "texture_diffuse";

    m_objects[0]->getMesh(0).addTexture(texture);
    m_objects[5]->getMesh(0).addTexture(texture);

    /* Create shader. */
    std::string dir = "../src/demos/02_simple_3d/";
    m_simple_texturing_shader = std::make_shared<RapidGL::Shader>(dir + "simple_3d.vert", dir + "simple_3d.frag");
    m_simple_texturing_shader->link();
}

void Simple3d::input()
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
        std::string filename = "02_simple_3d";
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

void Simple3d::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void Simple3d::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_simple_texturing_shader->bind();
    m_simple_texturing_shader->setUniform("mix_factor", m_mix_factor);

    auto view_projection = m_camera->m_projection * m_camera->m_view;
    
    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_simple_texturing_shader->setUniform("color", m_objects_colors[i]);
        m_simple_texturing_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);
        m_objects[i]->render(m_simple_texturing_shader);
    }
}

void Simple3d::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos = ImVec2(RapidGL::Window::getWidth() - 10.0, 10.0);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 400, 0 });

    ImGui::Begin("Info");
    {
        ImGui::SliderFloat("Color mix factor", &m_mix_factor, 0.0, 1.0);

        ImGui::Text("\nControls info: \n\n"
                    "F1     - take a screenshot\n"
                    "F2     - toggle wireframe rendering\n"
                    "WASDQE - control camera movement\n"
                    "RMB    - toggle cursor lock and rotate camera\n"
                    "Esc    - close the app\n\n");
    }
    ImGui::End();
}
