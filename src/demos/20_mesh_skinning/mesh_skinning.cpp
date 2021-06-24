#include "mesh_skinning.h"

#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

MeshSkinning::MeshSkinning()
    : m_mix_factor(0.0f)
{
}

MeshSkinning::~MeshSkinning()
{
}

void MeshSkinning::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    /* Create virtual camera. */
    m_camera = std::make_shared<RGL::Camera>(60.0, RGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 10.0);

    /* Create models. */
    for (unsigned i = 0; i < 8; ++i)
    {
        m_objects.emplace_back(RGL::StaticModel());
    }

    /* You can load model from a file or generate a primitive on the fly. */
    m_objects[0].Load(RGL::FileSystem::getPath("models/spot/spot.obj"));
    m_objects[1].GenCone(1.0, 0.5);
    m_objects[2].GenCube();
    m_objects[3].GenCylinder(1.0, 0.5);
    m_objects[4].GenPlane();
    m_objects[5].GenSphere(0.5);
    m_objects[6].GenTorus(0.5, 1.0);
    m_objects[7].GenQuad();

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-7.5, 0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0))); // spot
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
    auto texture_spot = std::make_shared<RGL::Texture2D>();
    texture_spot->Load(RGL::FileSystem::getPath("models/spot/spot.png"));

    auto texture_bricks = std::make_shared<RGL::Texture2D>();
    texture_bricks->Load(RGL::FileSystem::getPath("textures/bricks.png"));

    m_objects[0].AddTexture(texture_spot);
    m_objects[5].AddTexture(texture_bricks);

    /* Create shader. */
    std::string dir = "../src/demos/02_simple_3d/";
    m_simple_texturing_shader = std::make_shared<RGL::Shader>(dir + "simple_3d.vert", dir + "simple_3d.frag");
    m_simple_texturing_shader->link();
}

void MeshSkinning::input()
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
        std::string filename = "20_mesh_skinning";
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

void MeshSkinning::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void MeshSkinning::render()
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
        m_objects[i].Render();
    }
}

void MeshSkinning::render_gui()
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
        ImGui::SliderFloat("Color mix factor", &m_mix_factor, 0.0, 1.0);

        ImGui::Text("\nControls info: \n\n"
                    "F1     - take a screenshot\n"
                    "F2     - toggle wireframe rendering\n"
                    "WASDQE - control camera movement\n"
                    "RMB    - press to rotate the camera\n"
                    "Esc    - close the app\n\n");
    }
    ImGui::End();
}
