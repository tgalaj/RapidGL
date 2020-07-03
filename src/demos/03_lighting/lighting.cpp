#include "lighting.h"
#include "filesystem.h"
#include "input.h"
#include "util.h"
#include "gui/gui.h"

/* 
   ** TODO: **
   - GUI: ability to control the lights' properties
   - Dir and spot lights: direction depends on angles (eg. azimuth, elevation)
*/

Lighting::Lighting()
    : m_mix_factor(1.0f)
{
}

Lighting::~Lighting()
{
}

void Lighting::init_app()
{
    /* Initialize all the variables, buffers, etc. here. */
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    /* Create virtual camera. */
    m_camera = std::make_shared<RapidGL::Camera>(60.0, RapidGL::Window::getAspectRatio(), 0.01, 100.0);
    m_camera->setPosition(1.5, 0.0, 10.0);

    /* Create models. */
    for (unsigned i = 0; i < 9; ++i)
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
    m_objects[8]->genPlane(50, 50);

    /* Set model matrices for each model. */
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-7.5,  0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 1, 0))); // monkey
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-5.0,  0.5, -5)));                                                                         // cone
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(-2.5,  0.0, -5)));                                                                         // cube
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0,  0.0, -5)));                                                                         // cylinder
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 2.5,  0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1, 0, 0)));  // plane
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 5.0,  0.0, -5)));                                                                         // sphere
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 7.5,  0.0, -5)));                                                                         // torus
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3(10.0,  0.0, -5)) * glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(1, 0, 0)));  // quad
    m_objects_model_matrices.emplace_back(glm::translate(glm::mat4(1.0), glm::vec3( 0.0, -1.0, -5)));                                                                         // ground plane

    /* Add textures to the objects. */
    RapidGL::Texture texture;
    texture.m_id = RapidGL::Util::loadGLTexture("bricks.jpg", "textures", false);
    texture.m_type = "texture_diffuse";

    RapidGL::Texture default_diffuse_texture;
    default_diffuse_texture.m_id = RapidGL::Util::loadGLTexture("default_diffuse.png", "textures", false);
    default_diffuse_texture.m_type = "texture_diffuse";

    m_objects[0]->getMesh(0).addTexture(texture);
    m_objects[5]->getMesh(0).addTexture(texture);

    for (auto& model : m_objects)
    {
        if (model->getMesh(0).getTexturesCount() == 0)
        {
            model->getMesh(0).addTexture(default_diffuse_texture);
        }
    }

    /* Create shader. */
    std::string dir = "../src/demos/03_lighting/";
    m_ambient_light_shader = std::make_shared<RapidGL::Shader>(dir + "lighting.vert", dir + "lighting-ambient.frag");
    m_ambient_light_shader->link();

    m_directional_light_shader = std::make_shared<RapidGL::Shader>(dir + "lighting.vert", dir + "lighting-directional.frag");
    m_directional_light_shader->link();

    m_point_light_shader = std::make_shared<RapidGL::Shader>(dir + "lighting.vert", dir + "lighting-point.frag");
    m_point_light_shader->link();

    m_spot_light_shader = std::make_shared<RapidGL::Shader>(dir + "lighting.vert", dir + "lighting-spot.frag");
    m_spot_light_shader->link();
}

void Lighting::input()
{
    /* Close the application when Esc is released. */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Escape))
    {
        stop();
    }

    /* Toggle between wireframe and solid rendering */
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Alpha2))
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
    if (RapidGL::Input::getKeyUp(RapidGL::KeyCode::Alpha1))
    {
        /* Specify filename of the screenshot. */
        std::string filename = "03_lighting";
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

void Lighting::update(double delta_time)
{
    /* Update variables here. */
    m_camera->update(delta_time);
}

void Lighting::render()
{
    /* Put render specific code here. Don't update variables here! */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ambient_light_shader->bind();
    m_ambient_light_shader->setUniform("ambient_factor", 0.18f);

    auto view_projection = m_camera->m_projection * m_camera->m_view;

    /* First, render the ambient color only for the opaque objects. */
    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        //m_ambient_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_ambient_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->render(m_ambient_light_shader);
    }

    /*
     * Disable writing to the depth buffer and additively
     * shade only those pixels, that were shaded in the ambient step.
     */
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_EQUAL);

    /* Render directional light(s) */
    m_directional_light_shader->bind();

    m_directional_light_shader->setUniform("directional_light.base.color",     glm::vec3(1.0, 1.0, 1.0));
    m_directional_light_shader->setUniform("directional_light.base.intensity", 0.5f);
    m_directional_light_shader->setUniform("directional_light.direction",      glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
    
    m_directional_light_shader->setUniform("cam_pos",            m_camera->position());
    m_directional_light_shader->setUniform("specular_intensity", 0.2f);
    m_directional_light_shader->setUniform("specular_power",     120.0f);

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_directional_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_directional_light_shader->setUniform("normal_matrix", glm::transpose(glm::inverse(glm::mat3(m_objects_model_matrices[i]))));
        m_directional_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->render(m_directional_light_shader);
    }

    /* Render point lights */
    m_point_light_shader->bind();

    m_point_light_shader->setUniform("point_light.base.color",      glm::vec3(1.0, 0.0, 0.0));
    m_point_light_shader->setUniform("point_light.base.intensity",  5.0f);
    m_point_light_shader->setUniform("point_light.atten.constant",  1.0f);
    m_point_light_shader->setUniform("point_light.atten.linear",    1.0f);
    m_point_light_shader->setUniform("point_light.atten.quadratic", 2.0f);
    m_point_light_shader->setUniform("point_light.position",        glm::vec3(0.0, 1.0, -2.0));
    m_point_light_shader->setUniform("point_light.range",           5.0f);

    m_point_light_shader->setUniform("cam_pos", m_camera->position());
    m_point_light_shader->setUniform("specular_intensity", 0.2f);
    m_point_light_shader->setUniform("specular_power", 120.0f);

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_point_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_point_light_shader->setUniform("normal_matrix", glm::transpose(glm::inverse(glm::mat3(m_objects_model_matrices[i]))));
        m_point_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->render(m_point_light_shader);
    }

    /* Render spot lights */
    m_spot_light_shader->bind();

    m_spot_light_shader->setUniform("spot_light.point.base.color",      glm::vec3(0.0, 0.0, 1.0));
    m_spot_light_shader->setUniform("spot_light.point.base.intensity",  100.0f);
    m_spot_light_shader->setUniform("spot_light.point.atten.constant",  1.0f);
    m_spot_light_shader->setUniform("spot_light.point.atten.linear",    1.0f);
    m_spot_light_shader->setUniform("spot_light.point.atten.quadratic", 8.0f);
    m_spot_light_shader->setUniform("spot_light.point.position",        glm::vec3(-7.5, 3.0, -5));
    m_spot_light_shader->setUniform("spot_light.point.range",           5.0f);
    m_spot_light_shader->setUniform("spot_light.direction",             glm::normalize(glm::vec3(0.0, -1.0, 0.0)));
    m_spot_light_shader->setUniform("spot_light.cutoff",                glm::radians(45.0f));

    m_spot_light_shader->setUniform("cam_pos", m_camera->position());
    m_spot_light_shader->setUniform("specular_intensity", 0.2f);
    m_spot_light_shader->setUniform("specular_power", 120.0f);

    for (unsigned i = 0; i < m_objects.size(); ++i)
    {
        m_spot_light_shader->setUniform("model", m_objects_model_matrices[i]);
        m_spot_light_shader->setUniform("normal_matrix", glm::transpose(glm::inverse(glm::mat3(m_objects_model_matrices[i]))));
        m_spot_light_shader->setUniform("mvp", view_projection * m_objects_model_matrices[i]);

        m_objects[i]->render(m_spot_light_shader);
    }

    /* Enable writing to the depth buffer. */
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_ONE, GL_ZERO);
}

void Lighting::render_gui()
{
    /* This method is responsible for rendering GUI using ImGUI. */

    /* 
     * It's possible to call render_gui() from the base class.
     * It renders performance info overlay.
     */
    CoreApp::render_gui();

    /* Create your own GUI using ImGUI here. */
    ImVec2 window_pos = ImVec2(790.0, 10.0);
    ImVec2 window_pos_pivot = ImVec2(1.0f, 0.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowSize({ 400, 0 });

    ImGui::Begin("Info");
    {
        ImGui::SliderFloat("Color mix factor", &m_mix_factor, 0.0, 1.0);

        ImGui::Text("\nControls info: \n\n"
                    "Alpha 1 - take a screenshot\n"
                    "Alpha 2 - toggle wireframe rendering\n"
                    "WASDQE  - control camera movement\n"
                    "RMB     - toggle cursor lock and rotate camera\n"
                    "Esc     - close the app\n\n");
    }
    ImGui::End();
}
