#include "skybox.hpp"
#include "util.h"
#include <filesystem.h>

Skybox::Skybox(const std::string& skybox_directory,
               const std::string& left_face,
               const std::string& right_face,
               const std::string& up_face,
               const std::string& down_face,
               const std::string& front_face,
               const std::string& back_face)
    : m_world(glm::mat4(1.0f))
{
    /* Create cubemap texture object */
    std::string filenames[6] = 
    {
        RGL::FileSystem::getPath("textures/skyboxes/" + skybox_directory + "/" + left_face),
        RGL::FileSystem::getPath("textures/skyboxes/" + skybox_directory + "/" + right_face),
        RGL::FileSystem::getPath("textures/skyboxes/" + skybox_directory + "/" + up_face),
        RGL::FileSystem::getPath("textures/skyboxes/" + skybox_directory + "/" + down_face),
        RGL::FileSystem::getPath("textures/skyboxes/" + skybox_directory + "/" + front_face),
        RGL::FileSystem::getPath("textures/skyboxes/" + skybox_directory + "/" + back_face)
    };
    
    m_cubemap_texture.Load(filenames);

    /* Create skybox shader object */
    std::string dir = "../src/demos/08_enviro_mapping/";

    m_skybox_shader = std::make_shared<RGL::Shader>(dir + "skybox.vert", dir + "skybox.frag");
    m_skybox_shader->link();

    /* Create buffer objects */
    m_vao_id = 0;
    m_vbo_id = 0;

    glCreateVertexArrays(1, &m_vao_id);
    glCreateBuffers(1, &m_vbo_id);

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
    glNamedBufferStorage(m_vbo_id, skybox_positions.size() * sizeof(skybox_positions[0]), skybox_positions.data(), 0 /*flags*/);

    /* Set up VAO */
    glEnableVertexArrayAttrib(m_vao_id, 0 /*index*/);

    /* Separate attribute format */
    glVertexArrayAttribFormat(m_vao_id, 0 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
    glVertexArrayAttribBinding(m_vao_id, 0 /*index*/, 0 /*bindingindex*/);
    glVertexArrayVertexBuffer(m_vao_id, 0 /*bindingindex*/, m_vbo_id, 0 /*offset*/, sizeof(glm::vec3) /*stride*/);
}


Skybox::~Skybox()
{
    if (m_vao_id != 0)
    {
        glDeleteVertexArrays(1, &m_vao_id);
        m_vao_id = 0;
    }

    if (m_vbo_id != 0)
    {
        glDeleteBuffers(1, &m_vbo_id);
        m_vbo_id = 0;
    }
}

void Skybox::render(const glm::mat4& projection, const glm::mat4& view)
{
    m_skybox_shader->bind();
    m_skybox_shader->setUniform("view_projection", projection * glm::mat4(glm::mat3(view)));

    m_cubemap_texture.Bind(1);
    glBindVertexArray(m_vao_id);

    glDepthFunc(GL_LEQUAL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);
}

void Skybox::bindSkyboxTexture(GLuint unit)
{
    m_cubemap_texture.Bind(unit);
}
