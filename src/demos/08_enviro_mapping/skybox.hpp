#pragma once

#include <memory>
#include <string>

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

#include "shader.h"
#include "mesh.h"

class Skybox
{
public:
    /*
     * Creates skybox object.
     * @param skybox_directory absolute or relative directory where skybox image faces are stored.
     * @param left_face  filename of the left  side [+x] of the cubemap
     * @param right_face filename of the right side [-x] of the cubemap
     * @param up_face    filename of the up    side [+y] of the cubemap
     * @param down_face  filename of the down  side [-y] of the cubemap
     * @param front_face filename of the front side [+z] of the cubemap
     * @param back_face  filename of the back  side [-z] of the cubemap
     */
    Skybox(const std::string& skybox_directory,
           const std::string& left_face,
           const std::string& right_face,
           const std::string& up_face,
           const std::string& down_face,
           const std::string& front_face,
           const std::string& back_face);
    ~Skybox();

    void render(const glm::mat4& projection, const glm::mat4& view);
    void bindSkyboxTexture(GLuint unit = 0);

private:
    glm::mat4 m_world;

    GLuint m_vao_id;
    GLuint m_vbo_id;
    GLuint m_cube_map_id;

    std::shared_ptr<RGL::Shader>  m_skybox_shader;
};

