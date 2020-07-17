#include "model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <assimp/postprocess.h>

#include "geom_primitive.h"
#include "util.h"

namespace RapidGL
{
    Model::Model(bool enable_gamma_correction)
        : m_gamma_correction(enable_gamma_correction)
    {
    }

    Model::~Model()
    {
    }

    void Model::load(const std::string & filename)
    {
        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate              | 
                             aiProcess_GenSmoothNormals         | 
                             aiProcess_CalcTangentSpace         | 
                             aiProcess_FlipUVs                  | 
                             aiProcess_PreTransformVertices     | 
                             aiProcess_RemoveRedundantMaterials | 
                             aiProcess_ImproveCacheLocality     | 
                             aiProcess_JoinIdenticalVertices;
        const aiScene * scene = importer.ReadFile(filename, flags);

        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            fprintf(stderr, "Assimp error while loading mesh %s\n Error: %s\n", filename.c_str(), importer.GetErrorString());
            return;
        }

        aiString directory = aiString(filename.substr(0, filename.rfind("/")));

        processNode(scene->mRootNode, scene, directory);
    }

    void Model::processNode(aiNode * node, const aiScene * scene, aiString & directory)
    {
        for (GLuint i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
            m_meshes.push_back(processMesh(mesh, scene, directory));
        }

        for (GLuint i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene, directory);
        }
    }

    Mesh Model::processMesh(aiMesh * mesh, const aiScene * scene, aiString & directory) const
    {
        VertexBuffers buffers;
        Mesh ve_mesh;

        /* Process positions, normals, texcoords */
        for (GLuint i = 0; i < mesh->mNumVertices; ++i)
        {
            VertexBuffers::Vertex vertex;
            vertex.m_position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertex.m_normal   = glm::vec3(mesh->mNormals[i].x,  mesh->mNormals[i].y,  mesh->mNormals[i].z);

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.m_tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            }
            else
            {
                vertex.m_tangent = glm::vec3(0.0, 1.0, 0.0);
            }

            if (mesh->mTextureCoords[0])
            {
                vertex.m_texcoord = glm::vec3(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, 0.0f);
            }
            else
            {
                vertex.m_texcoord = glm::vec3(0.0f);
            }

            buffers.m_vertices.push_back(vertex);
        }

        /* Process indices */
        for (GLuint i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];

            for (GLuint j = 0; j < face.mNumIndices; ++j)
            {
                buffers.m_indices.push_back(face.mIndices[j]);
            }
        }

        /* Process textures */
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial * material = scene->mMaterials[mesh->mMaterialIndex];

            loadMaterialTextures(ve_mesh, material, aiTextureType_DIFFUSE,  "texture_diffuse",  directory);
            loadMaterialTextures(ve_mesh, material, aiTextureType_HEIGHT,   "texture_normal",   directory);
            loadMaterialTextures(ve_mesh, material, aiTextureType_NORMALS,  "texture_normal",   directory);
            loadMaterialTextures(ve_mesh, material, aiTextureType_SPECULAR, "texture_specular", directory);
            loadMaterialTextures(ve_mesh, material, aiTextureType_AMBIENT,  "texture_height",   directory);
        }

        /* Feed Vertex Engine's Mesh with data */
        ve_mesh.setBuffers(buffers);

        return ve_mesh;
    }

    void Model::loadMaterialTextures(Mesh & mesh, aiMaterial * mat, aiTextureType type, const std::string & type_name, aiString & directory) const
    {
        GLuint texturesCount = mat->GetTextureCount(type);

        if (texturesCount > 0)
        {
            for (GLuint i = 0; i < texturesCount; ++i)
            {
                aiString str, fullPath(directory);
                mat->GetTexture(type, i, &str);

                fullPath.Append("/");
                fullPath.Append(str.C_Str());

                Texture texture;
                texture.m_id   = Util::loadGLTexture(fullPath.C_Str(), "models/", m_gamma_correction);
                texture.m_type = type_name;
                texture.m_path = str.C_Str();

                mesh.addTexture(texture);
            }
        }
    }

    void Model::calcTangentSpace(VertexBuffers & buffers) const
    {
        for(unsigned i = 0; i < buffers.m_vertices.size(); ++i)
        {
            buffers.m_vertices[i].m_tangent = glm::vec3(0.0f);
        }

        for(unsigned i = 0; i < buffers.m_indices.size(); i += 3)
        {
            auto i0 = buffers.m_indices[i];
            auto i1 = buffers.m_indices[i + 1];
            auto i2 = buffers.m_indices[i + 2];

            auto edge1 = buffers.m_vertices[i1].m_position - buffers.m_vertices[i0].m_position;
            auto edge2 = buffers.m_vertices[i2].m_position - buffers.m_vertices[i0].m_position;

            auto delta_uv1 = buffers.m_vertices[i1].m_texcoord - buffers.m_vertices[i0].m_texcoord;
            auto delta_uv2 = buffers.m_vertices[i2].m_texcoord - buffers.m_vertices[i0].m_texcoord;

            float dividend = (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
            float f = dividend == 0.0f ? 0.0f : 1.0f / dividend;

            glm::vec3 tangent(0.0f);
            tangent.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
            tangent.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
            tangent.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

            buffers.m_vertices[i0].m_tangent += tangent;
            buffers.m_vertices[i1].m_tangent += tangent;
            buffers.m_vertices[i2].m_tangent += tangent;
        }

        for (unsigned i = 0; i < buffers.m_vertices.size(); ++i)
        {
            buffers.m_vertices[i].m_tangent = glm::normalize(buffers.m_vertices[i].m_tangent);
        }
    }

    void Model::genPrimitive(VertexBuffers & buffers, bool generate_tangents)
    {
        Mesh mesh;

        if (generate_tangents)
        {
            calcTangentSpace(buffers);
        }

        mesh.setBuffers(buffers);

        if(m_meshes.size() > 0)
        {
            return;
        }

        m_meshes.push_back(mesh);
    }

    void Model::genCone(float height, float radius, unsigned int slices, unsigned int stacks)
    {
        VertexBuffers buffers;
        GeomPrimitive::genCone(buffers, height, radius, slices, stacks);

        genPrimitive(buffers);
    }

    void Model::genCube(float radius)
    {
        VertexBuffers buffers;
        GeomPrimitive::genCube(buffers, radius);

        genPrimitive(buffers);
    }

    void Model::genCylinder(float height, float r, unsigned int slices)
    {
        VertexBuffers buffers;
        GeomPrimitive::genCylinder(buffers, height, r, slices);

        genPrimitive(buffers);
    }

    void Model::genPlane(float width, float height, unsigned int slices, unsigned int stacks)
    {
        VertexBuffers buffers;
        GeomPrimitive::genPlane(buffers, width, height, slices, stacks);

        genPrimitive(buffers);
    }

    void Model::genSphere(float radius, unsigned int slices)
    {
        VertexBuffers buffers;
        GeomPrimitive::genSphere(buffers, radius, slices);

        genPrimitive(buffers);
    }

    void Model::genTorus(float innerRadius, float outerRadius, unsigned int slices, unsigned int stacks)
    {
        VertexBuffers buffers;
        GeomPrimitive::genTorus(buffers, innerRadius, outerRadius, slices, stacks);

        genPrimitive(buffers);
    }

    void Model::genQuad(float width, float height)
    {
        VertexBuffers buffers;
        GeomPrimitive::genQuad(buffers, width, height);

        genPrimitive(buffers, false);
        m_meshes[m_meshes.size() - 1].setDrawMode(GL_TRIANGLE_STRIP);
    }

    void Model::render(std::shared_ptr<Shader>& shader)
    {
        for(unsigned i = 0; i < m_meshes.size(); ++i)
        {
            m_meshes[i].render(shader);
        }
    }

    void Model::setDrawMode(GLenum draw_mode)
    {
        for(unsigned i = 0; i < m_meshes.size(); ++i)
        {
            m_meshes[i].setDrawMode(draw_mode);
        }
    }
}
