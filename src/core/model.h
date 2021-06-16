#pragma once

#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "mesh.h"
#include "shader.h"

namespace RGL
{
    class Model
    {
    public:
        explicit Model(bool enable_gamma_correction = false);
        virtual ~Model();

        /* Primitives */
        void genCone       (float height = 3.0f, float r = 1.5f, unsigned int slices = 10, unsigned int stacks = 10);
        void genCube       (float radius = 1.0f);
        void genCylinder   (float height = 3.0f, float radius = 1.5f, unsigned int slices = 10);
        void genPlane      (float width  = 1.0f, float height = 1.0f, unsigned int slices = 5, unsigned int stacks = 5);
        void genPlaneGrid  (float width  = 1.0f, float height = 1.0f, unsigned int slices = 5, unsigned int stacks = 5);
        void genSphere     (float radius = 1.5f, unsigned int slices = 12);
        void genTorus      (float innerRadius = 1.0f, float outerRadius = 2.0f, unsigned int slices = 10, unsigned int stacks = 10);
        void genTrefoilKnot(unsigned int slices = 100, unsigned int stacks = 20);
        void genPQTorusKnot(unsigned int slices = 256, unsigned int stacks = 16, int p = 2, int q = 3, float knot_r = 0.75, float tube_r = 0.15);
        void genQuad       (float width = 1.0f, float height = 1.0f);

        void load(const std::string & filename);
        void render(std::shared_ptr<Shader> & shader, bool is_textured = true, uint32_t num_instances = 0);

        void setDrawMode(GLenum draw_mode);
        GLenum getDrawMode() { return getMesh(0).getDrawMode(); }

        Mesh & getMesh(unsigned int index = 0)
        {
            if (index > m_meshes.size())
            {
                index = m_meshes.size() - 1;
            }

            return m_meshes[index];
        }

        unsigned meshesCount() const { return m_meshes.size(); }

    protected:
        void calcTangentSpace(VertexBuffers & buffers) const;
        void genPrimitive(VertexBuffers & buffers, bool generate_tangents = true);

        void processNode(aiNode * node, const aiScene * scene, aiString & directory);
        Mesh processMesh(aiMesh * mesh, const aiScene * scene, aiString & directory) const;

        void loadMaterialTextures(Mesh & mesh, aiMaterial * mat, aiTextureType type, const std::string & type_name, aiString & directory) const;

        bool m_gamma_correction;
        std::vector<Mesh> m_meshes;
    };
}
