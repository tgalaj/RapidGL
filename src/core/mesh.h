#pragma once

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <vector>
#include <memory>
#include <string>

#include "shader.h"

namespace RGL
{
    struct VertexBuffers
    {
        struct Vertex
        {
            glm::vec3 m_position;
            glm::vec3 m_normal;
            glm::vec3 m_texcoord;
            glm::vec3 m_tangent;
        };

        std::vector<Vertex>  m_vertices;
        std::vector<GLuint>  m_indices;
    };

    struct Texture 
    {
        GLuint m_id;
        std::string m_type;
        std::string m_path;
    };

    struct MeshData
    {
        MeshData();
        ~MeshData();

        GLuint m_vao_id;
        GLuint m_vbo_ids[2];
        GLuint m_indices_count;
        GLenum m_draw_mode;

        std::vector<Texture> m_textures;
    };

    class Mesh
    {
    public:
        Mesh();
        ~Mesh();

        void setBuffers(const VertexBuffers & buffers);
        void setDrawMode(GLenum draw_mode) const { m_mesh_data->m_draw_mode = draw_mode; }
        void addTexture(const Texture & texture);

        GLenum getDrawMode()      const { return m_mesh_data->m_draw_mode; }
        GLuint getIndicesCount()  const { return m_mesh_data->m_indices_count; }
        GLuint getTexturesCount() const { return m_mesh_data->m_textures.size(); }

        void render(std::shared_ptr<Shader> & shader, bool is_textured = true);

    private:
        std::shared_ptr<MeshData> m_mesh_data;

        enum { VERTEX_DATA, INDEX };
    };
}
