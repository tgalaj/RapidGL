#include "animated_model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <assimp/postprocess.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/dual_quaternion.hpp>

namespace RGL
{
    void AnimatedModel::BoneTransform(float dt, std::vector<glm::mat4>& transforms)
    {
        if (m_assimp_scene->mNumAnimations > 0)
        {
            /* Calc animation duration */
            float ticks_per_second = (float)(m_assimp_scene->mAnimations[m_current_animation]->mTicksPerSecond != 0 ? m_assimp_scene->mAnimations[m_current_animation]->mTicksPerSecond : 25.0f);
            float animation_duration = (float)m_assimp_scene->mAnimations[m_current_animation]->mDuration;

            m_current_animation_time += ticks_per_second * dt * m_animation_speed;
            m_current_animation_time = fmod(m_current_animation_time, animation_duration);

            ReadNodeHierarchy(m_current_animation_time, m_assimp_scene->mRootNode, glm::mat4(1.0f));

            transforms.resize(m_bones_count);

            for (uint32_t i = 0; i < m_bones_count; ++i)
            {
                transforms[i] = m_bone_infos[i].m_final_transform;
            }
        }
    }

    void AnimatedModel::BoneTransform(float dt, std::vector<glm::mat2x4>& transforms)
    {
        if (m_assimp_scene->mNumAnimations > 0)
        {
            /* Calc animation duration */
            float ticks_per_second = (float)(m_assimp_scene->mAnimations[m_current_animation]->mTicksPerSecond != 0 ? m_assimp_scene->mAnimations[m_current_animation]->mTicksPerSecond : 25.0f);
            float animation_duration = (float)m_assimp_scene->mAnimations[m_current_animation]->mDuration;

            m_current_animation_time += ticks_per_second * dt * m_animation_speed;
            m_current_animation_time = fmod(m_current_animation_time, animation_duration);

            ReadNodeHierarchy(m_current_animation_time, m_assimp_scene->mRootNode, glm::mat4(1.0f));

            transforms.resize(m_bones_count);

            for (uint32_t i = 0; i < m_bones_count; ++i)
            {
                 glm::quat rotation(m_bone_infos[i].m_final_transform);
                 glm::fdualquat dq (rotation, m_bone_infos[i].m_final_transform[3]);

                 glm::mat2x4 dq_mat(glm::vec4(dq.real.w, dq.real.x, dq.real.y, dq.real.z), 
                                    glm::vec4(dq.dual.w, dq.dual.x, dq.dual.y, dq.dual.z));

                 transforms[i] = dq_mat;
            }
        }
    }

    void AnimatedModel::CalcInterpolatedScaling(aiVector3D& out, float animation_time, const aiNodeAnim* node_anim)
    {
        if (node_anim->mNumScalingKeys == 1)
        {
            out = node_anim->mScalingKeys[0].mValue;
            return;
        }

        uint32_t scaling_index      = FindScaling(animation_time, node_anim);
        uint32_t next_scaling_index = (scaling_index + 1);

        assert(next_scaling_index < node_anim->mNumScalingKeys);

        float delta_time = (float)(node_anim->mScalingKeys[next_scaling_index].mTime - node_anim->mScalingKeys[scaling_index].mTime);
        float factor     = (animation_time - (float)node_anim->mScalingKeys[scaling_index].mTime) / delta_time;

        assert(factor >= 0.0f && factor <= 1.0f);

        const aiVector3D& start = node_anim->mScalingKeys[scaling_index].mValue;
        const aiVector3D& end   = node_anim->mScalingKeys[next_scaling_index].mValue;

        aiVector3D delta = end - start;
        out = start + factor * delta;
    }

    void AnimatedModel::CalcInterpolatedRotation(aiQuaternion& out, float animation_time, const aiNodeAnim* node_anim)
    {
        // we need at least two values to interpolate...
        if (node_anim->mNumRotationKeys == 1)
        {
            out = node_anim->mRotationKeys[0].mValue;
            return;
        }

        uint32_t rotation_index      = FindRotation(animation_time, node_anim);
        uint32_t next_rotation_index = (rotation_index + 1);

        assert(next_rotation_index < node_anim->mNumRotationKeys);

        float delta_time = (float)(node_anim->mRotationKeys[next_rotation_index].mTime - node_anim->mRotationKeys[rotation_index].mTime);
        float factor     = (animation_time - (float)node_anim->mRotationKeys[rotation_index].mTime) / delta_time;

        assert(factor >= 0.0f && factor <= 1.0f);

        const aiQuaternion& start_rotation_quat = node_anim->mRotationKeys[rotation_index].mValue;
        const aiQuaternion& end_rotation_quat   = node_anim->mRotationKeys[next_rotation_index].mValue;

        aiQuaternion::Interpolate(out, start_rotation_quat, end_rotation_quat, factor);
        out = out.Normalize();

    }
    void AnimatedModel::CalcInterpolatedPosition(aiVector3D& out, float animation_time, const aiNodeAnim* node_anim)
    {
        if (node_anim->mNumPositionKeys == 1)
        {
            out = node_anim->mPositionKeys[0].mValue;
            return;
        }

        uint32_t position_index      = FindPosition(animation_time, node_anim);
        uint32_t next_position_index = (position_index + 1);

        assert(next_position_index < node_anim->mNumPositionKeys);

        float delta_time = (float)(node_anim->mPositionKeys[next_position_index].mTime - node_anim->mPositionKeys[position_index].mTime);
        float factor     = (animation_time - (float)node_anim->mPositionKeys[position_index].mTime) / delta_time;

        assert(factor >= 0.0f && factor <= 1.0f);

        const aiVector3D& start = node_anim->mPositionKeys[position_index].mValue;
        const aiVector3D& end  = node_anim->mPositionKeys[next_position_index].mValue;

        aiVector3D delta = end - start;
        out = start + factor * delta;
    }

    uint32_t AnimatedModel::FindScaling(float animation_time, const aiNodeAnim* node_anim)
    {
        assert(node_anim->mNumScalingKeys > 0);

        for (uint32_t i = 0; i < node_anim->mNumScalingKeys - 1; i++)
        {
            if (animation_time < (float)node_anim->mScalingKeys[i + 1].mTime)
            {
                return i;
            }
        }

        assert(0);

        return 0;
    }

    uint32_t AnimatedModel::FindRotation(float animation_time, const aiNodeAnim* node_anim)
    {
        assert(node_anim->mNumRotationKeys > 0);

        for (uint32_t i = 0; i < node_anim->mNumRotationKeys - 1; i++)
        {
            if (animation_time < (float)node_anim->mRotationKeys[i + 1].mTime)
            {
                return i;
            }
        }

        assert(0);

        return 0;
    }

    uint32_t AnimatedModel::FindPosition(float animation_time, const aiNodeAnim* node_anim)
    {
        for (uint32_t i = 0; i < node_anim->mNumPositionKeys - 1; i++)
        {
            if (animation_time < (float)node_anim->mPositionKeys[i + 1].mTime)
            {
                return i;
            }
        }

        assert(0);

        return 0;
    }

    const aiNodeAnim* AnimatedModel::FindNodeAnim(const aiAnimation* animation, std::string_view node_name)
    {
        for (uint32_t i = 0; i < animation->mNumChannels; i++)
        {
            const aiNodeAnim* node_anim = animation->mChannels[i];

            if (std::string(node_anim->mNodeName.data) == node_name)
            {
                return node_anim;
            }
        }

        return nullptr;
    }

    void AnimatedModel::ReadNodeHierarchy(float animation_time, const aiNode* node, const glm::mat4& parent_transform)
    {
              std::string  node_name      = std::string(node->mName.data);
        const aiAnimation* animation      = m_assimp_scene->mAnimations[m_current_animation];
              glm::mat4    node_transform = mat4_cast(node->mTransformation);
        const aiNodeAnim* node_anim       = FindNodeAnim(animation, node_name);

        if (node_anim)
        {
            // Interpolate scaling and generate scaling transformation matrix
            aiVector3D scaling;
            CalcInterpolatedScaling(scaling, animation_time, node_anim);
            glm::mat4 scaling_mat = glm::scale(glm::mat4(1.0), vec3_cast(scaling));

            // Interpolate rotation and generate rotation transformation matrix
            aiQuaternion rotation_quat;
            CalcInterpolatedRotation(rotation_quat, animation_time, node_anim);
            glm::quat rotation     = quat_cast(rotation_quat);
            glm::mat4 rotation_mat = glm::toMat4(rotation);

            // Interpolate translation and generate translation transformation matrix
            aiVector3D translation;
            CalcInterpolatedPosition(translation, animation_time, node_anim);
            glm::mat4 translation_mat = glm::translate(glm::mat4(1.0), vec3_cast(translation));

            // Combine the above transformations
            node_transform = translation_mat * rotation_mat * scaling_mat;
        }

        glm::mat4 global_transform = parent_transform * node_transform;

        if (m_bones_mapping.find(node_name) != m_bones_mapping.end())
        {
            uint32_t bone_index = m_bones_mapping[node_name];
            m_bone_infos[bone_index].m_final_transform = m_global_inverse_transform * global_transform * m_bone_infos[bone_index].m_bone_offset;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            ReadNodeHierarchy(animation_time, node->mChildren[i], global_transform);
        }
    }

    void AnimatedModel::LoadBones(uint32_t mesh_index, const aiMesh* mesh, std::vector<VertexBoneData>& bones)
    {
        for (uint32_t i = 0; i < mesh->mNumBones; i++)
        {
            uint32_t bone_index = 0;
            std::string bone_name(mesh->mBones[i]->mName.data);

            if (m_bones_mapping.find(bone_name) == m_bones_mapping.end())
            {
                // Allocate an index for a new bone
                bone_index = m_bones_count;
                m_bones_count++;
                
                BoneInfo bi;
                m_bone_infos.push_back(bi);

                m_bone_infos[bone_index].m_bone_offset = mat4_cast(mesh->mBones[i]->mOffsetMatrix);
                m_bones_mapping[bone_name]             = bone_index;
            }
            else
            {
                bone_index = m_bones_mapping[bone_name];
            }

            for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; j++)
            {
                uint32_t vertex_id = m_mesh_parts[mesh_index].m_base_vertex + mesh->mBones[i]->mWeights[j].mVertexId;
                float    weight    = mesh->mBones[i]->mWeights[j].mWeight;

                bones[vertex_id].AddBoneData(bone_index, weight);
            }
        }
    }

    bool AnimatedModel::Load(const std::filesystem::path& filepath, bool srgb_textures)
    {
        /* Release the previously loaded mesh if it was loaded. */
        if(m_vao_name)
        {
            Release();
        }

        /* Load model */
        m_assimp_scene = m_importer.ReadFile(filepath.generic_string(), aiProcess_Triangulate              |
                                                                        aiProcess_GenSmoothNormals         | 
                                                                        aiProcess_CalcTangentSpace         |
                                                                        aiProcess_FlipUVs                  |
                                                                        aiProcess_JoinIdenticalVertices    | 
                                                                        aiProcess_RemoveRedundantMaterials | 
                                                                        aiProcess_GenBoundingBoxes );

        if (!m_assimp_scene || m_assimp_scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !m_assimp_scene->mRootNode)
        {
            fprintf(stderr, "Assimp error while loading mesh %s\n Error: %s\n", filepath.generic_string(), m_importer.GetErrorString());
            return false;
        }

        m_global_inverse_transform = mat4_cast(m_assimp_scene->mRootNode->mTransformation);
        m_global_inverse_transform = glm::inverse(m_global_inverse_transform);
        m_animations_count         = m_assimp_scene->mNumAnimations;

        return ParseScene(m_assimp_scene, filepath, srgb_textures);
    }

    std::vector<std::string> AnimatedModel::GetAnimationsNames() const
    {
        auto animations_count = m_assimp_scene->mNumAnimations;
        std::vector<std::string> animations_names(animations_count);

        for (uint32_t i = 0; i < animations_count; ++i)
        {
            animations_names[i] = m_assimp_scene->mAnimations[i]->mName.C_Str();
        }

        return animations_names;
    }

    bool AnimatedModel::ParseScene(const aiScene* scene, const std::filesystem::path& filepath, bool srgb_textures)
    {
        m_mesh_parts.resize(scene->mNumMeshes);
        m_textures.resize(scene->mNumMaterials);

        VertexData vertex_data;
        std::vector<VertexBoneData> bones_data;

        uint32_t vertices_count = 0;
        uint32_t indices_count = 0;

        /* Count the number of vertices and indices. */
        for (uint32_t i = 0; i < m_mesh_parts.size(); ++i)
        {
            m_mesh_parts[i].m_material_index = scene->mMeshes[i]->mMaterialIndex;
            m_mesh_parts[i].m_indices_count = scene->mMeshes[i]->mNumFaces * 3;
            m_mesh_parts[i].m_base_vertex = vertices_count;
            m_mesh_parts[i].m_base_index = indices_count;

            vertices_count += scene->mMeshes[i]->mNumVertices;
            indices_count  += m_mesh_parts[i].m_indices_count;
        }

        /* Reserve space in the vectors for the vertex attributes and indices. */
        vertex_data.positions.reserve(vertices_count);
        vertex_data.texcoords.reserve(vertices_count);
        vertex_data.normals.reserve(vertices_count);
        vertex_data.tangents.reserve(vertices_count);
        vertex_data.indices.reserve(indices_count);
        bones_data.resize(vertices_count);

        /* Load mesh parts one by one. */
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = -min;

        for (uint32_t i = 0; i < m_mesh_parts.size(); ++i)
        {
            aiMesh* mesh = scene->mMeshes[i];
            LoadMeshPart(i, mesh, vertex_data, bones_data);

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
        CreateBuffers(vertex_data, bones_data);

        return true;
    }

    void AnimatedModel::LoadMeshPart(uint32_t mesh_index, const aiMesh* mesh, VertexData& vertex_data, std::vector<VertexBoneData>& bones_data)
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

        LoadBones(mesh_index, mesh, bones_data);

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

    void AnimatedModel::CreateBuffers(VertexData& vertex_data, std::vector<VertexBoneData>& bones_data)
    {
        bool has_tangents   = !vertex_data.tangents.empty();
        bool has_bones_data = !bones_data.empty();

        std::vector<float> bone_weights; 
        bone_weights.reserve(bones_data.size() * NUM_BONES_PER_VERTEX);

        std::vector<int> bone_ids; 
        bone_ids.reserve(bones_data.size() * NUM_BONES_PER_VERTEX);

        if (has_bones_data)
        {
            for (uint32_t i = 0; i < bones_data.size(); ++i)
            {
                for (auto& bone_weight : bones_data[i].m_weights)
                {
                    bone_weights.push_back(bone_weight);
                }
            }

            for (uint32_t i = 0; i < bones_data.size(); ++i)
            {
                for (auto& bone_id : bones_data[i].m_ids)
                {
                    bone_ids.push_back(bone_id);
                }
            }
        }

        const GLsizei positions_size_bytes    = vertex_data.positions.size() * sizeof(vertex_data.positions[0]);
        const GLsizei texcoords_size_bytes    = vertex_data.texcoords.size() * sizeof(vertex_data.texcoords[0]);
        const GLsizei normals_size_bytes      = vertex_data.normals  .size() * sizeof(vertex_data.normals  [0]);

        const GLsizei tangents_size_bytes     = has_tangents   ? vertex_data.tangents.size() * sizeof(vertex_data.tangents[0]) : 0;
        const GLsizei bone_weights_size_bytes = has_bones_data ? bone_weights        .size() * sizeof(bone_weights        [0]) : 0;
        const GLsizei bone_ids_size_bytes     = has_bones_data ? bone_ids            .size() * sizeof(bone_ids            [0]) : 0;

        const GLsizei total_size_bytes = positions_size_bytes + texcoords_size_bytes + normals_size_bytes + tangents_size_bytes + bone_weights_size_bytes + bone_ids_size_bytes;

        glCreateBuffers(1, &m_vbo_name);
        glNamedBufferStorage(m_vbo_name, total_size_bytes, nullptr, GL_DYNAMIC_STORAGE_BIT);

        uint64_t offset = 0;

        glNamedBufferSubData(m_vbo_name, offset, positions_size_bytes, vertex_data.positions.data());
        offset += positions_size_bytes;

        glNamedBufferSubData(m_vbo_name, offset, texcoords_size_bytes, vertex_data.texcoords.data());
        offset += texcoords_size_bytes;

        glNamedBufferSubData(m_vbo_name, offset, normals_size_bytes, vertex_data.normals.data());
        offset += normals_size_bytes;

        if (has_tangents)
        {
            glNamedBufferSubData(m_vbo_name, offset, tangents_size_bytes, vertex_data.tangents.data());
            offset += tangents_size_bytes;
        }

        if (has_bones_data)
        {
            glNamedBufferSubData(m_vbo_name, offset, bone_weights_size_bytes, bone_weights.data());
            offset += bone_weights_size_bytes;

            glNamedBufferSubData(m_vbo_name, offset, bone_ids_size_bytes, bone_ids.data());
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
        offset += normals_size_bytes;

        if (has_tangents)
        {
            glVertexArrayVertexBuffer(m_vao_name, 3 /* bindingindex*/, m_vbo_name, offset, sizeof(vertex_data.tangents[0]) /*stride*/);
            offset += tangents_size_bytes;
        }

        if (has_bones_data)
        {
            glVertexArrayVertexBuffer(m_vao_name, 4 /* bindingindex*/, m_vbo_name, offset, sizeof(bone_weights[0]) * 4 /*stride*/);
            offset += bone_weights_size_bytes;

            glVertexArrayVertexBuffer(m_vao_name, 5 /* bindingindex*/, m_vbo_name, offset, sizeof(bone_ids[0]) * 4 /*stride*/);
        }

        glVertexArrayElementBuffer(m_vao_name, m_ibo_name);

        glEnableVertexArrayAttrib(m_vao_name, 0 /*attribindex*/); // positions
        glEnableVertexArrayAttrib(m_vao_name, 1 /*attribindex*/); // texcoords
        glEnableVertexArrayAttrib(m_vao_name, 2 /*attribindex*/); // normals
        if (has_tangents) 
        {
            glEnableVertexArrayAttrib(m_vao_name, 3 /*attribindex*/); // tangents
        }

        if (has_bones_data)
        {
            glEnableVertexArrayAttrib(m_vao_name, 4 /*attribindex*/); // bone weights
            glEnableVertexArrayAttrib(m_vao_name, 5 /*attribindex*/); // bone ids
        }

        glVertexArrayAttribFormat(m_vao_name, 0 /*attribindex */, 3 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/); 
        glVertexArrayAttribFormat(m_vao_name, 1 /*attribindex */, 2 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/); 
        glVertexArrayAttribFormat(m_vao_name, 2 /*attribindex */, 3 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/); 
        if (has_tangents) 
        {
            glVertexArrayAttribFormat(m_vao_name, 3 /*attribindex */, 3 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
        }

        if (has_bones_data)
        {
            glVertexArrayAttribFormat (m_vao_name, 4 /*attribindex */, 4 /* size */, GL_FLOAT, GL_FALSE, 0 /*relativeoffset*/);
            glVertexArrayAttribIFormat(m_vao_name, 5 /*attribindex */, 4 /* size */, GL_INT,             0 /*relativeoffset*/);
        }

        glVertexArrayAttribBinding(m_vao_name, 0 /*attribindex*/, 0 /*bindingindex*/); // positions
        glVertexArrayAttribBinding(m_vao_name, 1 /*attribindex*/, 1 /*bindingindex*/); // texcoords
        glVertexArrayAttribBinding(m_vao_name, 2 /*attribindex*/, 2 /*bindingindex*/); // normals
        if (has_tangents) 
        {
            glVertexArrayAttribBinding(m_vao_name, 3 /*attribindex*/, 3 /*bindingindex*/); // tangents
        }

        if (has_bones_data)
        {
            glVertexArrayAttribBinding(m_vao_name, 4 /*attribindex*/, 4 /*bindingindex*/); // bone weights
            glVertexArrayAttribBinding(m_vao_name, 5 /*attribindex*/, 5 /*bindingindex*/); // bone ids
        }
    }
}