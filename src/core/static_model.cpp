#include "static_model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include <assimp/postprocess.h>

#include "util.h"

namespace RGL
{
    void StaticModel::Render(uint32_t num_instances)
    {
        glBindVertexArray(m_vao_name);
    
        for (unsigned int i = 0 ; i < m_mesh_parts.size() ; i++) 
        {
            if(!m_textures.empty())
            {
                const unsigned int material_index = m_mesh_parts[i].m_material_index;

                assert(material_index < m_textures.size());
                
                for(auto & pair : m_textures[material_index])
                {
                     pair.first->Bind(pair.second);
                }
            }

            if(num_instances == 0 )
            {
                glDrawElementsBaseVertex(GLenum(m_draw_mode), 
                                         m_mesh_parts[i].m_indices_count,
                                         GL_UNSIGNED_INT,
                                         (void*)(sizeof(unsigned int) * m_mesh_parts[i].m_base_index),
                                         m_mesh_parts[i].m_base_vertex);
            }
            else
            {
                glDrawElementsInstancedBaseVertex(GLenum(m_draw_mode),
                                                  m_mesh_parts[i].m_indices_count,
                                                  GL_UNSIGNED_INT,
                                                  (void*)(sizeof(unsigned int) * m_mesh_parts[i].m_base_index),
                                                  num_instances,
                                                  m_mesh_parts[i].m_base_vertex);
            }
        }

        glBindTextureUnit(0, 0);
    }

    bool StaticModel::Load(const std::filesystem::path& filepath, bool srgb_textures)
    {
        /* Release the previously loaded mesh if it was loaded. */
        if(m_vao_name)
        {
            Release();
        }

        /* Load model */
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath.generic_string(), aiProcess_Triangulate              | 
                                                                            aiProcess_GenSmoothNormals         | 
                                                                            aiProcess_CalcTangentSpace         |
                                                                            aiProcess_FlipUVs                  |
                                                                            aiProcess_JoinIdenticalVertices    | 
                                                                            aiProcess_RemoveRedundantMaterials | 
                                                                            aiProcess_GenBoundingBoxes );

        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            fprintf(stderr, "Assimp error while loading mesh %s\n Error: %s\n", filepath.generic_string(), importer.GetErrorString());
            return false;
        }

        return ParseScene(scene, filepath, srgb_textures);
    }

    bool StaticModel::ParseScene(const aiScene* scene, const std::filesystem::path& filepath, bool srgb_textures)
    {
        m_mesh_parts.resize(scene->mNumMeshes);
        m_textures.resize(scene->mNumMaterials);

        VertexData vertex_data;

        uint32_t vertices_count = 0;
        uint32_t indices_count  = 0;

        /* Count the number of vertices and indices. */
        for (uint32_t i = 0; i < m_mesh_parts.size(); ++i)
        {
            m_mesh_parts[i].m_material_index = scene->mMeshes[i]->mMaterialIndex;
            m_mesh_parts[i].m_indices_count  = scene->mMeshes[i]->mNumFaces * 3;
            m_mesh_parts[i].m_base_vertex    = vertices_count;
            m_mesh_parts[i].m_base_index     = indices_count;

            vertices_count += scene->mMeshes[i]->mNumVertices;
            indices_count  += m_mesh_parts[i].m_indices_count;
        }

        /* Reserve space in the vectors for the vertex attributes and indices. */
        vertex_data.positions.reserve(vertices_count);
        vertex_data.texcoords.reserve(vertices_count);
        vertex_data.normals.reserve(vertices_count);
        vertex_data.tangents.reserve(vertices_count);
        vertex_data.indices.reserve(indices_count);

        /* Load mesh parts one by one. */
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = -min;

        for (uint32_t i = 0; i < m_mesh_parts.size(); ++i)
        {
            auto mesh = scene->mMeshes[i];
            LoadMeshPart(mesh, vertex_data);

            min = glm::min(min, vec3_cast(mesh->mAABB.mMin));
            max = glm::max(max, vec3_cast(mesh->mAABB.mMax));
        }

        m_unit_scale = 1.0f / glm::compMax(max - min);

        /* Load materials. */
        if (!LoadMaterials(scene, filepath, srgb_textures))
        {
            fprintf(stderr, "Assimp error while loading mesh %s\n Error: Could not load the materials.\n", filepath.generic_string());
            return false;
        }

        /* Populate buffers on the GPU with the model's data. */
        CreateBuffers(vertex_data);

        return true;
    }

    void StaticModel::LoadMeshPart(const aiMesh* mesh, VertexData& vertex_data)
    {
        const glm::vec3 zero_vec3(0.0f, 0.0f, 0.0f);

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            auto pos      = vec3_cast(mesh->mVertices[i]);
            auto texcoord = mesh->HasTextureCoords(0)        ? vec3_cast(mesh->mTextureCoords[0][i]) : zero_vec3;
            auto normal   = mesh->HasNormals()               ? vec3_cast(mesh->mNormals[i])          : zero_vec3;
            auto tangent  = mesh->HasTangentsAndBitangents() ? vec3_cast(mesh->mTangents[i])         : zero_vec3;

            vertex_data.positions.push_back(pos);
            vertex_data.texcoords.push_back(glm::vec2(texcoord.x, texcoord.y));
            vertex_data.normals.push_back(normal);
            vertex_data.tangents.push_back(tangent);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);

            for (char i = 0; i < face.mNumIndices; ++i)
            {
                vertex_data.indices.push_back(face.mIndices[i]);
            }
        }
    }

    bool StaticModel::LoadMaterials(const aiScene* scene, const std::filesystem::path& filepath, bool srgb_textures)
    {
        // Extract the directory part from the file name
        std::string::size_type last_slash_index = filepath.generic_string().rfind("/");
        std::string dir;

        if (last_slash_index == std::string::npos)
        {
            dir = ".";
        }
        else if (last_slash_index == 0)
        {
            dir = "/";
        }
        else
        {
            dir = filepath.generic_string().substr(0, last_slash_index);
        }

        /* Check if the model has embedded textures. */
        if (scene->HasTextures())
        {
            for (uint32_t i = 0; i < scene->mNumTextures; ++i)
            {
                uint32_t data_size = scene->mTextures[i]->mHeight > 0 ? scene->mTextures[i]->mWidth * scene->mTextures[i]->mHeight : scene->mTextures[i]->mWidth;

                auto texture = std::make_shared<Texture2D>();
                if (texture->Load(reinterpret_cast<unsigned char*>(scene->mTextures[i]->pcData), data_size, srgb_textures))
                {
                    if(scene->mTextures[i]->achFormatHint[0] & 0x01)
                    {
                        texture->SetWraping(RGL::TextureWrapingCoordinate::S, RGL::TextureWrapingParam::REPEAT);
                        texture->SetWraping(RGL::TextureWrapingCoordinate::T, RGL::TextureWrapingParam::REPEAT);
                    }
                    m_textures[i].push_back({ texture, 0 });

                    printf("Loaded embedded texture for the model '%s'\n", filepath.generic_string().c_str());
                }
                else
                {
                    fprintf(stderr, "Error loading embedded texture for the model %s.\n", filepath.generic_string().c_str());
                    return false;
                }
            }
        }
        else
        {
            /* If not, load the materials and textures normally. */
            for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
            {
                auto pMaterial = scene->mMaterials[i];

                if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
                {
                    aiString path;

                    if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
                    {
                        std::string p(path.data);

                        if (p.substr(0, 2) == ".\\")
                        {
                            p = p.substr(2, p.size() - 2);
                        }

                        std::string full_path = dir + "/" + p;

                        auto texture = std::make_shared<Texture2D>();
                        if (!texture->Load(full_path, srgb_textures))
                        {
                            fprintf(stderr, "Error loading texture %s.\n", full_path.c_str());
                            return false;
                        }
                        else
                        {
                            printf("Loaded texture '%s'\n", full_path.c_str());
                            m_textures[i].push_back({ texture, 0 });
                        }
                    }
                }
            }
        }

        return true;
    }

    void StaticModel::CreateBuffers(VertexData& vertex_data)
    {
        bool has_tangents = !vertex_data.tangents.empty();

        const GLsizei positions_size_bytes = vertex_data.positions.size() * sizeof(vertex_data.positions[0]);
        const GLsizei texcoords_size_bytes = vertex_data.texcoords.size() * sizeof(vertex_data.texcoords[0]);
        const GLsizei normals_size_bytes   = vertex_data.normals  .size() * sizeof(vertex_data.normals  [0]);
        const GLsizei tangents_size_bytes  = has_tangents ? vertex_data.tangents .size() * sizeof(vertex_data.tangents [0]) : 0;
        const GLsizei total_size_bytes     = positions_size_bytes + texcoords_size_bytes + normals_size_bytes + tangents_size_bytes;

        glCreateBuffers     (1, &m_vbo_name);
        glNamedBufferStorage(m_vbo_name, total_size_bytes, nullptr, GL_DYNAMIC_STORAGE_BIT);

        uint64_t offset = 0;
        glNamedBufferSubData(m_vbo_name, offset, positions_size_bytes, vertex_data.positions.data());

        offset += positions_size_bytes;
        glNamedBufferSubData(m_vbo_name, offset, texcoords_size_bytes, vertex_data.texcoords.data());

        offset += texcoords_size_bytes;
        glNamedBufferSubData(m_vbo_name, offset, normals_size_bytes, vertex_data.normals.data());

        if(has_tangents)
        {
            offset += normals_size_bytes;
            glNamedBufferSubData(m_vbo_name, offset, tangents_size_bytes, vertex_data.tangents.data());
        }

        glCreateBuffers     (1, &m_ibo_name);
        glNamedBufferStorage(m_ibo_name, sizeof(vertex_data.indices[0]) * vertex_data.indices.size(), vertex_data.indices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &m_vao_name);

        offset = 0;
        glVertexArrayVertexBuffer(m_vao_name, 0 /* bindingindex*/, m_vbo_name, offset, sizeof(vertex_data.positions[0]) /*stride*/);
                          
        offset += positions_size_bytes;
        glVertexArrayVertexBuffer(m_vao_name, 1 /* bindingindex*/, m_vbo_name, offset, sizeof(vertex_data.texcoords[0]) /*stride*/);
        
        offset += texcoords_size_bytes;
        glVertexArrayVertexBuffer(m_vao_name, 2 /* bindingindex*/, m_vbo_name,  offset, sizeof(vertex_data.normals[0]) /*stride*/);

        if (has_tangents)
        {
            offset += normals_size_bytes;
            glVertexArrayVertexBuffer(m_vao_name, 3 /* bindingindex*/, m_vbo_name, offset, sizeof(vertex_data.tangents[0]) /*stride*/);
        }

        glVertexArrayElementBuffer(m_vao_name, m_ibo_name);

                          glEnableVertexArrayAttrib(m_vao_name, 0 /*attribindex*/); // positions
                          glEnableVertexArrayAttrib(m_vao_name, 1 /*attribindex*/); // texcoords
                          glEnableVertexArrayAttrib(m_vao_name, 2 /*attribindex*/); // normals
        if (has_tangents) glEnableVertexArrayAttrib(m_vao_name, 3 /*attribindex*/); // tangents

                          glVertexArrayAttribFormat(m_vao_name, 0 /*attribindex */, 3 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/); 
                          glVertexArrayAttribFormat(m_vao_name, 1 /*attribindex */, 2 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/); 
                          glVertexArrayAttribFormat(m_vao_name, 2 /*attribindex */, 3 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/); 
        if (has_tangents) glVertexArrayAttribFormat(m_vao_name, 3 /*attribindex */, 3 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);

                          glVertexArrayAttribBinding(m_vao_name, 0 /*attribindex*/, 0 /*bindingindex*/); // positions
                          glVertexArrayAttribBinding(m_vao_name, 1 /*attribindex*/, 1 /*bindingindex*/); // texcoords
                          glVertexArrayAttribBinding(m_vao_name, 2 /*attribindex*/, 2 /*bindingindex*/); // normals
        if (has_tangents) glVertexArrayAttribBinding(m_vao_name, 3 /*attribindex*/, 3 /*bindingindex*/); // tangents
    }

    /* The first available input attribute index is 4. */
    void StaticModel::AddAttributeBuffer(GLuint attrib_index, GLuint binding_index, GLint format_size, GLenum data_type, GLuint buffer_id, GLsizei stride, GLuint divisor)
    {
        if(m_vao_name)
        {
            glVertexArrayVertexBuffer  (m_vao_name, binding_index, buffer_id, 0 /*offset*/, stride);
            glEnableVertexArrayAttrib  (m_vao_name, attrib_index);
            glVertexArrayAttribFormat  (m_vao_name, attrib_index, format_size, data_type, GL_FALSE, 0 /*relativeoffset*/);
            glVertexArrayAttribBinding (m_vao_name, attrib_index, binding_index);
            glVertexArrayBindingDivisor(m_vao_name, binding_index, divisor);
        }
    }

    void StaticModel::AddTexture(const std::shared_ptr<Texture2D>& texture, uint32_t bindingindex, uint32_t mesh_id)
    {
        assert(mesh_id < m_mesh_parts.size());

        /* If the mesh part doesn't have any texture assigned, add the new one. */
        if (m_mesh_parts[mesh_id].m_material_index == INVALID_MATERIAL)
        {
            TexturesContainer container = { {texture, bindingindex} };
            m_textures.push_back(container);
            m_mesh_parts[mesh_id].m_material_index = m_textures.size() - 1;
        }
        /* If mesh part had already assigned a texture, add new one. */
        else
        {
            auto material_index = m_mesh_parts[mesh_id].m_material_index;
            m_textures[material_index].push_back({texture, bindingindex});
        }
    }

    void StaticModel::CalcTangentSpace(VertexData& vertex_data)
    {
        vertex_data.tangents.resize(vertex_data.positions.size());
        std::fill(std::begin(vertex_data.tangents), std::end(vertex_data.tangents), glm::vec3(0.0f));

        for(unsigned i = 0; i < vertex_data.indices.size(); i += 3)
        {
            auto i0 = vertex_data.indices[i];
            auto i1 = vertex_data.indices[i + 1];
            auto i2 = vertex_data.indices[i + 2];

            auto edge1 = vertex_data.positions[i1] - vertex_data.positions[i0];
            auto edge2 = vertex_data.positions[i2] - vertex_data.positions[i0];

            auto delta_uv1 = vertex_data.texcoords[i1] - vertex_data.texcoords[i0];
            auto delta_uv2 = vertex_data.texcoords[i2] - vertex_data.texcoords[i0];

            float dividend = (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
            float f = dividend == 0.0f ? 0.0f : 1.0f / dividend;

            glm::vec3 tangent(0.0f);
            tangent.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
            tangent.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
            tangent.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

            vertex_data.tangents[i0] += tangent;
            vertex_data.tangents[i1] += tangent;
            vertex_data.tangents[i2] += tangent;
        }

        for (unsigned i = 0; i < vertex_data.positions.size(); ++i)
        {
            vertex_data.tangents[i] = glm::normalize(vertex_data.tangents[i]);
        }
    }

    void StaticModel::GenPrimitive(VertexData& vertex_data, bool generate_tangents)
    {
        /* Release the previously loaded mesh if it was loaded. */
        if (m_vao_name)
        {
            Release();
        }

        if (generate_tangents)
        {
            CalcTangentSpace(vertex_data);
        }

        CreateBuffers(vertex_data);
        
        MeshPart mesh_part;
        mesh_part.m_base_index   = 0;
        mesh_part.m_base_vertex  = 0;
        mesh_part.m_indices_count = vertex_data.indices.size();

        m_mesh_parts.push_back(mesh_part);
    }

    void StaticModel::GenCone(float height, float radius, uint32_t slices, uint32_t stacks)
    {
        VertexData vertex_data;

        float thetaInc = glm::two_pi<float>() / float(slices);
        float theta = 0.0f;

        /* Center bottom */
        glm::vec3 p = glm::vec3(0.0f, height, 0.0f);

        vertex_data.positions.push_back(-p);
        vertex_data.normals  .push_back(glm::vec3(0.0f, -1.0f, 0.0f));
        vertex_data.texcoords.push_back(glm::vec2(0.5f, 0.5f));

        /* Bottom */
        for (uint32_t sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            vertex_data.positions.push_back(glm::vec3(glm::cos(theta) * radius, -height, -glm::sin(theta) * radius));
            vertex_data.normals  .push_back(glm::vec3(0.0f, -1.0f, 0.0f));
            vertex_data.texcoords.push_back(glm::vec2(glm::cos(theta) * 0.5f + 0.5f, glm::sin(theta) * 0.5f + 0.5f));
        }

        /* Sides */
        float l = glm::sqrt(height * height + radius * radius);

        for (uint32_t stackCount = 0; stackCount <= stacks; ++stackCount)
        {
            float level = stackCount / float(stacks);

            for (uint32_t sliceCount = 0; sliceCount <= slices; ++sliceCount, theta += thetaInc)
            {
                vertex_data.positions.push_back( glm::vec3(glm::cos(theta) * radius * (1.0f - level),
                                                -height + height * level,
                                                -glm::sin(theta) * radius * (1.0f - level)));
                vertex_data.normals  .push_back( glm::vec3(glm::cos(theta) * height / l, radius / l, -glm::sin(theta) * height / l));
                vertex_data.texcoords.push_back( glm::vec2(sliceCount / float(slices), level));
            }
        }

        uint32_t centerIdx = 0;
        uint32_t idx = 1;

        /* Indices Bottom */
        for (uint32_t sliceCount = 0; sliceCount < slices; ++sliceCount)
        {
            vertex_data.indices.push_back(centerIdx);
            vertex_data.indices.push_back(idx + 1);
            vertex_data.indices.push_back(idx);

            ++idx;
        }
        ++idx;

        /* Indices Sides */
        for (uint32_t stackCount = 0; stackCount < stacks; ++stackCount)
        {
            for (uint32_t sliceCount = 0; sliceCount < slices; ++sliceCount)
            {
                vertex_data.indices.push_back(idx);
                vertex_data.indices.push_back(idx + 1);
                vertex_data.indices.push_back(idx + slices + 1);

                vertex_data.indices.push_back(idx + 1);
                vertex_data.indices.push_back(idx + slices + 2);
                vertex_data.indices.push_back(idx + slices + 1);

                ++idx;
            }

            ++idx;
        }

        GenPrimitive(vertex_data);
    }

    void StaticModel::GenCube(float radius)
    {
        VertexData vertex_data;

        float r2 = radius * 0.5f;

        vertex_data.positions =
        {
            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, -r2, -r2),

            glm::vec3(-r2, +r2, -r2),
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, +r2, -r2),

            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, +r2, -r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, -r2, -r2),

            glm::vec3(-r2, -r2, +r2),
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, -r2, +r2),

            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(-r2, +r2, -r2),

            glm::vec3(+r2, -r2, -r2),
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, +r2, -r2) 
        };

        vertex_data.normals =
        {
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            
            glm::vec3(0.0f, +1.0f, 0.0f),
            glm::vec3(0.0f, +1.0f, 0.0f),
            glm::vec3(0.0f, +1.0f, 0.0f),
            glm::vec3(0.0f, +1.0f, 0.0f),
            
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            
            glm::vec3(0.0f, 0.0f, +1.0f),
            glm::vec3(0.0f, 0.0f, +1.0f),
            glm::vec3(0.0f, 0.0f, +1.0f),
            glm::vec3(0.0f, 0.0f, +1.0f),
            
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            
            glm::vec3(+1.0f, 0.0f, 0.0f),
            glm::vec3(+1.0f, 0.0f, 0.0f),
            glm::vec3(+1.0f, 0.0f, 0.0f),
            glm::vec3(+1.0f, 0.0f, 0.0f)
        };

        vertex_data.texcoords =
        {
            glm::vec2(0.0f, 0.0f),
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
                    
            glm::vec2(0.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
                    
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),
            glm::vec2(0.0f, 0.0f),

            glm::vec2(0.0f, 0.0f),
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),

            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f),

            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f),
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f)
        };

        vertex_data.indices = 
        {
            0,  2,  1,  0,  3,  2,
            4,  5,  6,  4,  6,  7,
            8,  9,  10, 8,  10, 11,
            12, 15, 14, 12, 14, 13,
            16, 17, 18, 16, 18, 19,
            20, 23, 22, 20, 22, 21
        };

        GenPrimitive(vertex_data);
    }

    void StaticModel::GenCubeMap(float radius)
    {
        VertexData vertex_data;

        float r2 = radius * 0.5f;

        vertex_data.positions =
        {
            // Front
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(-r2, +r2, +r2),
            // Right
            glm::vec3(+r2, -r2, +r2),
            glm::vec3(+r2, -r2, -r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, +r2, +r2),
            // Back
            glm::vec3(-r2, -r2, -r2),
            glm::vec3(-r2, +r2, -r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, -r2, -r2),
            // Left
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(-r2, +r2, -r2),
            glm::vec3(-r2, -r2, -r2),
            // Bottom
            glm::vec3(-r2, -r2, +r2),
            glm::vec3(-r2, -r2, -r2),
            glm::vec3(+r2, -r2, -r2),
            glm::vec3(+r2, -r2, +r2),
            // Top
            glm::vec3(-r2, +r2, +r2),
            glm::vec3(+r2, +r2, +r2),
            glm::vec3(+r2, +r2, -r2),
            glm::vec3(+r2, -r2, +r2)
        };

        vertex_data.indices =
        {
            0,  2,  1,  0,  3,  2,
            4,  6,  5,  4,  7,  6,
            8,  10, 9,  8,  11, 10,
            12, 14, 13, 12, 15, 14,
            16, 18, 17, 16, 19, 18,
            20, 22, 21, 20, 23, 22
        };

        GenPrimitive(vertex_data);
    }

    void StaticModel::GenCylinder(float height, float radius, uint32_t slices)
    {
        VertexData vertex_data;

        float halfHeight = height * 0.5f;
        glm::vec3 p1 = glm::vec3(0.0f, halfHeight, 0.0f);
        glm::vec3 p2 = -p1;

        float thetaInc = glm::two_pi<float>() / float(slices);
        float theta = 0.0f;
        float sign = -1.0f;

        /* Center bottom */
        vertex_data.positions.push_back(p2);
        vertex_data.normals  .push_back(glm::vec3(0.0f, -1.0f, 0.0f));
        vertex_data.texcoords.push_back(glm::vec2(0.5f, 0.5f));

        /* Bottom */
        for (uint32_t sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            vertex_data.positions.push_back(glm::vec3(glm::cos(theta) * radius, -halfHeight, -glm::sin(theta) * radius));
            vertex_data.normals  .push_back(glm::vec3(0.0f, -1.0f, 0.0f));
            vertex_data.texcoords.push_back(glm::vec2(glm::cos(theta) * 0.5f + 0.5f, glm::sin(theta) * 0.5f + 0.5f));
        }

        /* Center top */
        vertex_data.positions.push_back(p1);
        vertex_data.normals  .push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        vertex_data.texcoords.push_back(glm::vec2(0.5f, 0.5f));

        /* Top */
        for (uint32_t sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            vertex_data.positions.push_back(glm::vec3(glm::cos(theta) * radius, halfHeight, -glm::sin(theta) * radius));
            vertex_data.normals  .push_back(glm::vec3(0.0f, 1.0f, 0.0f));
            vertex_data.texcoords.push_back(glm::vec2(glm::cos(theta) * 0.5f + 0.5f, glm::sin(theta) * 0.5f + 0.5f));
        }

        /* Sides */
        for (uint32_t sideCount = 0; sideCount <= slices; ++sideCount, theta += thetaInc)
        {
            sign = -1.0f;

            for (int i = 0; i < 2; ++i)
            {
                vertex_data.positions.push_back(glm::vec3(glm::cos(theta) * radius, halfHeight * sign, -glm::sin(theta) * radius));
                vertex_data.normals  .push_back(glm::vec3(glm::cos(theta), 0.0f, -glm::sin(theta)));
                vertex_data.texcoords.push_back(glm::vec2(sideCount / (float)slices, (sign + 1.0f) * 0.5f));

                sign = 1.0f;
            }
        }

        uint32_t centerIdx = 0;
        uint32_t idx = 1;

        /* Indices Bottom */
        for (uint32_t sideCount = 0; sideCount < slices; ++sideCount)
        {
            vertex_data.indices.push_back(centerIdx);
            vertex_data.indices.push_back(idx + 1);
            vertex_data.indices.push_back(idx);

            ++idx;
        }
        ++idx;

        /* Indices Top */
        centerIdx = idx;
        ++idx;

        for (uint32_t sideCount = 0; sideCount < slices; ++sideCount)
        {
            vertex_data.indices.push_back(centerIdx);
            vertex_data.indices.push_back(idx);
            vertex_data.indices.push_back(idx + 1);

            ++idx;
        }
        ++idx;

        /* Indices Sides */
        for (uint32_t sideCount = 0; sideCount < slices; ++sideCount)
        {
            vertex_data.indices.push_back(idx);
            vertex_data.indices.push_back(idx + 2);
            vertex_data.indices.push_back(idx + 1);

            vertex_data.indices.push_back(idx + 2);
            vertex_data.indices.push_back(idx + 3);
            vertex_data.indices.push_back(idx + 1);

            idx += 2;
        }

        GenPrimitive(vertex_data);
    }

    void StaticModel::GenPlane(float width, float height, uint32_t slices, uint32_t stacks)
    {
        VertexData vertex_data;

        float widthInc  = width  / float(slices);
        float heightInc = height / float(stacks);

        float w = -width * 0.5f;
        float h = -height * 0.5f;

        for (uint32_t j = 0; j <= stacks; ++j, h += heightInc)
        {
            for (uint32_t i = 0; i <= slices; ++i, w += widthInc)
            {
                vertex_data.positions.push_back(glm::vec3(w, 0.0f, h));
                vertex_data.normals  .push_back(glm::vec3(0.0f, 1.0f, 0.0f));
                vertex_data.texcoords.push_back(glm::vec2(i, j));
            }
            w = -width * 0.5f;
        }

        uint32_t idx = 0;

        for (uint32_t j = 0; j < stacks; ++j)
        {
            for (uint32_t i = 0; i < slices; ++i)
            {
                vertex_data.indices.push_back(idx);
                vertex_data.indices.push_back(idx + slices + 1);
                vertex_data.indices.push_back(idx + 1);

                vertex_data.indices.push_back(idx + 1);
                vertex_data.indices.push_back(idx + slices + 1);
                vertex_data.indices.push_back(idx + slices + 2);

                ++idx;
            }

            ++idx;
        }

        GenPrimitive(vertex_data);    }

    void StaticModel::GenPlaneGrid(float width, float height, uint32_t slices, uint32_t stacks)
    {
        m_draw_mode = DrawMode::LINES;

        VertexData vertex_data;

        float widthInc = width / float(slices);
        float heightInc = height / float(stacks);

        float w = -width * 0.5f;
        float h = -height * 0.5f;

        for (uint32_t j = 0; j <= stacks; ++j, h += heightInc)
        {
            for (uint32_t i = 0; i <= slices; ++i, w += widthInc)
            {
                vertex_data.positions.push_back(glm::vec3(w   , 0.0f, h   ));
                vertex_data.normals  .push_back(glm::vec3(0.0f, 1.0f, 0.0f));
                vertex_data.texcoords.push_back(glm::vec2(i, j));
            }
            w = -width * 0.5f;
        }

        uint32_t idx = 0;

        for (uint32_t j = 0; j < stacks; ++j)
        {
            for (uint32_t i = 0; i < slices; ++i)
            {
                vertex_data.indices.push_back(idx);
                vertex_data.indices.push_back(idx + 1);

                vertex_data.indices.push_back(idx + 1);
                vertex_data.indices.push_back(idx + slices + 2);

                vertex_data.indices.push_back(idx + slices + 2);
                vertex_data.indices.push_back(idx + slices + 1);

                vertex_data.indices.push_back(idx + slices + 1);
                vertex_data.indices.push_back(idx);

                ++idx;
            }

            ++idx;
        }
        GenPrimitive(vertex_data, false);
    }

    void StaticModel::GenSphere(float radius, uint32_t slices)
    {
        VertexData vertex_data;

        float deltaPhi = glm::two_pi<float>() / static_cast<float>(slices);

        uint32_t parallels = static_cast<uint32_t>(slices * 0.5f);

        for (uint32_t i = 0; i <= parallels; ++i)
        {
            for (uint32_t j = 0; j <= slices; ++j)
            {
                vertex_data.positions.push_back(glm::vec3(radius * glm::sin(deltaPhi * i) * glm::sin(deltaPhi * j),
                                                          radius * glm::cos(deltaPhi * i),
                                                          radius * glm::sin(deltaPhi * i) * glm::cos(deltaPhi * j)));
                vertex_data.normals  .push_back(glm::vec3(radius * glm::sin(deltaPhi * i) * glm::sin(deltaPhi * j) / radius,
                                                          radius * glm::cos(deltaPhi * i) / radius,
                                                          radius * glm::sin(deltaPhi * i) * glm::cos(deltaPhi * j) / radius));
                vertex_data.texcoords.push_back(glm::vec2(       j / static_cast<float>(slices),
                                                          1.0f - i / static_cast<float>(parallels)));
            }
        }

        for (uint32_t i = 0; i < parallels; ++i)
        {
            for (uint32_t j = 0; j < slices; ++j)
            {
                vertex_data.indices.push_back(i       * (slices + 1) + j);
                vertex_data.indices.push_back((i + 1) * (slices + 1) + j);
                vertex_data.indices.push_back((i + 1) * (slices + 1) + (j + 1));

                vertex_data.indices.push_back(i       * (slices + 1) + j);
                vertex_data.indices.push_back((i + 1) * (slices + 1) + (j + 1));
                vertex_data.indices.push_back(i       * (slices + 1) + (j + 1));
            }
        }

        GenPrimitive(vertex_data);
    }

    void StaticModel::GenTorus(float innerRadius, float outerRadius, uint32_t slices, uint32_t stacks)
    {
        VertexData vertex_data;

        float phi   = 0.0f;
        float theta = 0.0f;

        float cos2PIp = 0.0f;
        float sin2PIp = 0.0f;
        float cos2PIt = 0.0f;
        float sin2PIt = 0.0f;

        float torusRadius = (outerRadius - innerRadius) * 0.5f;
        float centerRadius = outerRadius - torusRadius;

        float phiInc = 1.0f / float(slices);
        float thetaInc = 1.0f / float(stacks);

        vertex_data.positions.reserve((stacks + 1) * (slices + 1));
        vertex_data.texcoords.reserve((stacks + 1) * (slices + 1));
        vertex_data.normals.reserve((stacks + 1) * (slices + 1));
        vertex_data.tangents.reserve((stacks + 1) * (slices + 1));
        vertex_data.indices.reserve(stacks * slices * 2 * 3);

        for (uint32_t sideCount = 0; sideCount <= slices; ++sideCount, phi += phiInc)
        {
            cos2PIp = glm::cos(glm::two_pi<float>() * phi);
            sin2PIp = glm::sin(glm::two_pi<float>() * phi);

            theta = 0.0f;
            for (uint32_t faceCount = 0; faceCount <= stacks; ++faceCount, theta += thetaInc)
            {
                cos2PIt = glm::cos(glm::two_pi<float>() * theta);
                sin2PIt = glm::sin(glm::two_pi<float>() * theta);

                vertex_data.positions.push_back(glm::vec3((centerRadius + torusRadius * cos2PIt) * cos2PIp,
                                                          (centerRadius + torusRadius * cos2PIt) * sin2PIp,
                                                           torusRadius * sin2PIt));

                vertex_data.normals.push_back(glm::vec3(cos2PIp * cos2PIt,
                                                        sin2PIp * cos2PIt,
                                                        sin2PIt));

                vertex_data.texcoords.push_back(glm::vec2(phi, theta));
            }
        }

        for (uint32_t sideCount = 0; sideCount < slices; ++sideCount)
        {
            for (uint32_t faceCount = 0; faceCount < stacks; ++faceCount)
            {
                uint32_t v0 = sideCount       * (stacks + 1) + faceCount;
                uint32_t v1 = (sideCount + 1) * (stacks + 1) + faceCount;
                uint32_t v2 = (sideCount + 1) * (stacks + 1) + (faceCount + 1);
                uint32_t v3 = sideCount       * (stacks + 1) + (faceCount + 1);

                vertex_data.indices.push_back(v0);
                vertex_data.indices.push_back(v1);
                vertex_data.indices.push_back(v2);

                vertex_data.indices.push_back(v0);
                vertex_data.indices.push_back(v2);
                vertex_data.indices.push_back(v3);
            }
        }

        GenPrimitive(vertex_data);
    }

    /* Code courtesy of: https://prideout.net/blog/old/blog/index.html@tag=toon-shader.html */
    void StaticModel::GenTrefoilKnot(uint32_t slices, uint32_t stacks)
    {
        VertexData vertex_data;

        auto evaluate_trefoil = [](float s, float t)
        {
            const float a = 0.5f;
            const float b = 0.3f;
            const float c = 0.5f;
            const float d = 0.1f;
            const float u = (1 - s) * 2 * glm::two_pi<float>();
            const float v = t * glm::two_pi<float>();
            const float r = a + b * cos(1.5f * u);
            const float x = r * cos(u);
            const float y = r * sin(u);
            const float z = c * sin(1.5f * u);

            glm::vec3 dv;
            dv.x = -1.5f * b * sin(1.5f * u) * cos(u) -
                    (a + b * cos(1.5f * u)) * sin(u);
            dv.y = -1.5f * b * sin(1.5f * u) * sin(u) +
                    (a + b * cos(1.5f * u)) * cos(u);
            dv.z =  1.5f * c * cos(1.5f * u);

            glm::vec3 q   = glm::normalize(dv);
            glm::vec3 qvn = glm::normalize(glm::vec3(q.y, -q.x, 0));
            glm::vec3 ww  = glm::cross(qvn, q);

            glm::vec3 range;
            range.x = x + d * (qvn.x * cos(v) + ww.x * sin(v));
            range.y = y + d * (qvn.y * cos(v) + ww.y * sin(v));
            range.z = z + d * ww.z * sin(v);

            return range;
        };

        float ds = 1.0 / slices;
        float dt = 1.0 / stacks;

        const float E = 0.01f;

        // The upper bounds in these loops are tweaked to reduce the
        // chance of precision error causing an incorrect # of iterations.
    
        for (float s = 0; s < 1.0 - ds / 2.0; s += ds)
        {
            for (float t = 0; t < 1.0 - dt / 2.0; t += dt)
            {
                glm::vec3 p = evaluate_trefoil(s, t);
                glm::vec3 u = evaluate_trefoil(s + E, t) - p;
                glm::vec3 v = evaluate_trefoil(s, t + E) - p;
                glm::vec3 n = glm::normalize(glm::cross(v, u));

                vertex_data.positions.push_back(p);
                vertex_data.normals  .push_back(n);
                vertex_data.texcoords.push_back(glm::vec2(s, t));
            }
        }

        uint32_t n            = 0;
        uint32_t vertex_count = vertex_data.positions.size();

        for (uint32_t i = 0; i < slices; ++i)
        {
            for (uint32_t j = 0; j < stacks; ++j)
            {
                vertex_data.indices.push_back(n + j);
                vertex_data.indices.push_back(n + (j + 1) % stacks);
                vertex_data.indices.push_back((n + j + stacks) % vertex_count);

                vertex_data.indices.push_back((n + j + stacks) % vertex_count);
                vertex_data.indices.push_back((n + (j + 1) % stacks) % vertex_count);
                vertex_data.indices.push_back((n + (j + 1) % stacks + stacks) % vertex_count);
            }

            n += stacks;
        }

        GenPrimitive(vertex_data);
    }

    /* Implementation inspired by: https://blackpawn.com/texts/pqtorus/default.html */
    void StaticModel::GenPQTorusKnot(uint32_t slices, uint32_t stacks, int p, int q, float knot_r, float tube_r)
    {
        VertexData vertex_data;

        float theta      = 0.0;
        float theta_step = glm::two_pi<float>() / slices;

        float phi      = -3.14 / 4.0;
        float phi_step = glm::two_pi<float>() / stacks;

        if (p < 1)
        {
            p = 1;
        }

        if (q < 0)
        {
            q = 0;
        }

        for (uint32_t slice = 0; slice <= slices; ++slice, theta += theta_step)
        {
            phi = -3.14 / 4.0;

            float r = knot_r * (0.5 * (2.0 + glm::sin(q * theta)));
            auto  P = glm::vec3(glm::cos(p * theta), glm::cos(q * theta), glm::sin(p * theta)) * r;

            auto theta_next = theta + theta_step * 1.0;
                      r     = knot_r * (0.5 * (2.0 + glm::sin(q * theta_next)));
            auto P_next     = glm::vec3(glm::cos(p * theta_next), glm::cos(q * theta_next), glm::sin(p * theta_next)) * r;

            auto T = P_next - P;
            auto N = P_next + P;
            auto B = glm::normalize(glm::cross(T, N));
                 N = glm::normalize(glm::cross(B, T)); /* corrected normal */

            for (uint32_t stack = 0; stack <= stacks; ++stack, phi += phi_step)
            {
                glm::vec2 circle_vertex_position = glm::vec2(glm::cos(phi), glm::sin(phi)) * tube_r;

                vertex_data.positions.push_back(N * circle_vertex_position.x + B * circle_vertex_position.y + P);
                vertex_data.normals  .push_back(glm::normalize(vertex_data.positions[stack] - P));
                vertex_data.texcoords.push_back(glm::vec2(slice / float(slices), 1.0 - stack / float(stacks)));
            }
        }

        for (uint32_t slice = 0; slice < slices; ++slice)
        {
            for (uint32_t stack = 0; stack < stacks; ++stack)
            {
                uint32_t v0 = slice * (stacks + 1) + stack;
                uint32_t v1 = (slice + 1) * (stacks + 1) + stack;
                uint32_t v2 = (slice + 1) * (stacks + 1) + (stack + 1);
                uint32_t v3 = slice * (stacks + 1) + (stack + 1);
                
                vertex_data.indices.push_back(v2);
                vertex_data.indices.push_back(v1);
                vertex_data.indices.push_back(v0);
                
                vertex_data.indices.push_back(v3);
                vertex_data.indices.push_back(v2);
                vertex_data.indices.push_back(v0);
            }
        }

        GenPrimitive(vertex_data);
    }

    void StaticModel::GenQuad(float width, float height)
    {
        m_draw_mode = DrawMode::TRIANGLE_STRIP;

        VertexData vertex_data;

        float halfWidth  = width * 0.5f;
        float halfHeight = height * 0.5f;

        vertex_data.positions.push_back(glm::vec3(-halfWidth, 0.0f, -halfHeight));
        vertex_data.positions.push_back(glm::vec3(-halfWidth, 0.0f, halfHeight));
        vertex_data.positions.push_back(glm::vec3(halfWidth, 0.0f, -halfHeight));
        vertex_data.positions.push_back(glm::vec3(halfWidth, 0.0f, halfHeight));

        vertex_data.normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        vertex_data.normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        vertex_data.normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        vertex_data.normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

        vertex_data.texcoords.push_back(glm::vec2(0.0f, 1.0f));
        vertex_data.texcoords.push_back(glm::vec2(0.0f, 0.0f));
        vertex_data.texcoords.push_back(glm::vec2(1.0f, 1.0f));
        vertex_data.texcoords.push_back(glm::vec2(1.0f, 0.0f));

        vertex_data.indices.push_back(0);
        vertex_data.indices.push_back(1);
        vertex_data.indices.push_back(2);
        vertex_data.indices.push_back(3);

        GenPrimitive(vertex_data, false);
    }
}
