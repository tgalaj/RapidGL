#pragma once

#include <filesystem>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "mesh_part.h"
#include "texture.h"

namespace RGL
{
    struct VertexData
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> texcoords;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec3> tangents;
        std::vector<uint32_t>  indices;
    };

    enum class DrawMode { POINTS         = GL_POINTS, 
                          LINES          = GL_LINES, 
                          TRIANGLES      = GL_TRIANGLES, 
                          TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
                          PATCHES        = GL_PATCHES };

    class StaticModel
    {
    public:
        StaticModel()
            : m_vao_name (0),
              m_vbo_name (0),
              m_ibo_name (0),
              m_draw_mode(DrawMode::TRIANGLES)
        {
        }

        ~StaticModel() { Release(); }

        StaticModel           (const StaticModel&) = delete;
        StaticModel& operator=(const StaticModel&) = delete;

        StaticModel(StaticModel&& other) noexcept
            : m_vao_name (other.m_vao_name),
              m_vbo_name (other.m_vbo_name),
              m_ibo_name (other.m_ibo_name),
              m_draw_mode(other.m_draw_mode)
        {
            other.m_vao_name  = 0;
            other.m_vbo_name  = 0;
            other.m_ibo_name  = 0;
            other.m_draw_mode = DrawMode::TRIANGLES;
        }

        StaticModel& operator=(StaticModel&& other)
        {
            if (this != &other)
            {
                Release();

                std::swap(m_vao_name,  other.m_vao_name);
                std::swap(m_vbo_name,  other.m_vbo_name);
                std::swap(m_ibo_name,  other.m_ibo_name);
                std::swap(m_draw_mode, other.m_draw_mode);
            }
        }

        virtual void AddAttributeBuffer(GLuint attrib_index, GLuint binding_index, GLint format_size, GLenum data_type, GLuint buffer_id, GLsizei stride, GLuint divisor = 0);
        virtual void AddTexture(const std::shared_ptr<Texture2D> & texture, uint32_t bindingindex = 0, uint32_t mesh_id = 0);
        virtual void SetDrawMode(DrawMode mode) { m_draw_mode = mode; }

        virtual bool Load(const std::filesystem::path& filepath);
        virtual void Render(uint32_t num_instances = 0);

        /* Primitives */
        virtual void GenCone       (float    height      = 3.0f, float    radius      = 1.5f, uint32_t slices = 10, uint32_t stacks = 10);
        virtual void GenCube       (float    radius      = 1.0f);                     
        virtual void GenCubeMap    (float    radius      = 1.0f);                     
        virtual void GenCylinder   (float    height      = 3.0f, float    radius      = 1.5f, uint32_t slices = 10);
        virtual void GenPlane      (float    width       = 1.0f, float    height      = 1.0f, uint32_t slices = 5, uint32_t stacks = 5);
        virtual void GenPlaneGrid  (float    width       = 1.0f, float    height      = 1.0f, uint32_t slices = 5, uint32_t stacks = 5);
        virtual void GenSphere     (float    radius      = 1.5f, uint32_t slices      = 12);
        virtual void GenTorus      (float    innerRadius = 1.0f, float    outerRadius = 2.0f, uint32_t slices = 10, uint32_t stacks = 10);
        virtual void GenTrefoilKnot(uint32_t slices      = 100,  uint32_t stacks      = 20);
        virtual void GenPQTorusKnot(uint32_t slices      = 256,  uint32_t stacks      = 16,   int p = 2, int q = 3, float knot_r = 0.75, float tube_r = 0.15);
        virtual void GenQuad       (float    width       = 1.0f, float    height      = 1.0f);

    protected:
        bool ParseScene(const aiScene* scene, const std::filesystem::path& filepath);
        void LoadMeshPart(const aiMesh* mesh, VertexData& vertex_data);
        bool LoadMaterials(const aiScene* scene, const std::filesystem::path& filepath);
        void CreateBuffers(VertexData& vertex_data);

        void CalcTangentSpace(VertexData& vertex_data);
        void GenPrimitive(VertexData& vertex_data, bool generate_tangents = true);

        void Release()
        {
            glDeleteBuffers(1, &m_vbo_name);
            m_vbo_name = 0;

            glDeleteBuffers(1, &m_ibo_name);
            m_ibo_name = 0;

            glDeleteVertexArrays(1, &m_vao_name);
            m_vao_name = 0;

            m_draw_mode = DrawMode::TRIANGLES;

            m_mesh_parts.clear();
            m_textures.clear();
        }

        GLuint   m_vao_name;
        GLuint   m_vbo_name;
        GLuint   m_ibo_name;
        DrawMode m_draw_mode;
        
        using TexturesContainer = std::vector<std::pair<std::shared_ptr<Texture2D>, uint32_t>>;

        std::vector<MeshPart> m_mesh_parts;
        std::vector<TexturesContainer> m_textures;
    };
}
