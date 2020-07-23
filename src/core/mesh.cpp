#include "mesh.h"

namespace RapidGL
{
    MeshData::MeshData()
        : m_indices_count(0),
          m_draw_mode(GL_TRIANGLES)
    {
        glCreateVertexArrays(1, &m_vao_id);
        glCreateBuffers(sizeof(m_vbo_ids) / sizeof(GLuint), m_vbo_ids);
    }

    MeshData::~MeshData()
    {
        if (m_vao_id != 0)
        {
            glDeleteVertexArrays(1, &m_vao_id);
        }

        if (m_vbo_ids[0] != 0)
        {
            glDeleteBuffers(sizeof(m_vbo_ids) / sizeof(GLuint), m_vbo_ids);
        }

        for (auto & texture : m_textures)
        {
            glDeleteTextures(1, &texture.m_id);
        }
    }

    Mesh::Mesh()
    {
    }

    Mesh::~Mesh()
    {
    }

    void Mesh::setBuffers(const VertexBuffers & buffers)
    {
        m_mesh_data = std::make_shared<MeshData>();
        m_mesh_data->m_indices_count = buffers.m_indices.size();

        /* Set up buffer objects */
        glNamedBufferStorage(m_mesh_data->m_vbo_ids[VERTEX_DATA],  buffers.m_vertices.size()  * sizeof(buffers.m_vertices[0]),  buffers.m_vertices.data(),  0 /*flags*/);
        glNamedBufferStorage(m_mesh_data->m_vbo_ids[INDEX],        buffers.m_indices.size()   * sizeof(buffers.m_indices[0]),   buffers.m_indices.data(),   0 /*flags*/);

        /* Set up VAO */
        glEnableVertexArrayAttrib(m_mesh_data->m_vao_id, 0 /*index*/);
        glEnableVertexArrayAttrib(m_mesh_data->m_vao_id, 1 /*index*/);
        glEnableVertexArrayAttrib(m_mesh_data->m_vao_id, 2 /*index*/);
        glEnableVertexArrayAttrib(m_mesh_data->m_vao_id, 3 /*index*/);

        glVertexArrayElementBuffer(m_mesh_data->m_vao_id, m_mesh_data->m_vbo_ids[INDEX]);

        /* Separate attribute format */
        glVertexArrayAttribFormat(m_mesh_data->m_vao_id, 0 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, offsetof(VertexBuffers::Vertex, m_position) /*relativeoffset*/);
        glVertexArrayAttribFormat(m_mesh_data->m_vao_id, 1 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, offsetof(VertexBuffers::Vertex, m_normal)   /*relativeoffset*/);
        glVertexArrayAttribFormat(m_mesh_data->m_vao_id, 2 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, offsetof(VertexBuffers::Vertex, m_texcoord) /*relativeoffset*/);
        glVertexArrayAttribFormat(m_mesh_data->m_vao_id, 3 /*index*/, 3 /*size*/, GL_FLOAT, GL_FALSE, offsetof(VertexBuffers::Vertex, m_tangent)  /*relativeoffset*/);

        glVertexArrayAttribBinding(m_mesh_data->m_vao_id, 0 /*index*/, 0 /*bindingindex*/);
        glVertexArrayAttribBinding(m_mesh_data->m_vao_id, 1 /*index*/, 0 /*bindingindex*/);
        glVertexArrayAttribBinding(m_mesh_data->m_vao_id, 2 /*index*/, 0 /*bindingindex*/);
        glVertexArrayAttribBinding(m_mesh_data->m_vao_id, 3 /*index*/, 0 /*bindingindex*/);
        
        glVertexArrayVertexBuffer(m_mesh_data->m_vao_id, 0 /*bindingindex*/, m_mesh_data->m_vbo_ids[VERTEX_DATA], 0 /*offset*/, sizeof(buffers.m_vertices[0]) /*stride*/);
    }

    void Mesh::addTexture(const Texture & texture)
    {
        m_mesh_data->m_textures.push_back(texture);
    }

    void Mesh::render(std::shared_ptr<Shader> & shader, bool is_textured)
    {        
        if (is_textured)
        {
            unsigned int diffuse_nr = 1;
            unsigned int specular_nr = 1;
            unsigned int normal_nr = 1;
            unsigned int height_nr = 1;

            for (unsigned int i = 0; i < m_mesh_data->m_textures.size(); ++i)
            {
                glActiveTexture(GL_TEXTURE0 + i);

                std::string number;
                std::string name = m_mesh_data->m_textures[i].m_type;

                if (name == "texture_diffuse")
                    number = std::to_string(diffuse_nr++);
                else if (name == "texture_specular")
                    number = std::to_string(specular_nr++);
                else if (name == "texture_normal")
                    number = std::to_string(normal_nr++);
                else if (name == "texture_height")
                    number = std::to_string(height_nr++);

                shader->setUniform((name + number).c_str(), static_cast<int>(i));
                glBindTexture(GL_TEXTURE_2D, m_mesh_data->m_textures[i].m_id);
            }
        }

        glBindVertexArray(m_mesh_data->m_vao_id);
        glDrawElements(m_mesh_data->m_draw_mode, m_mesh_data->m_indices_count, GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
