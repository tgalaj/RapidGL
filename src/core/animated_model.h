#pragma once

#include "static_model.h"

#include <glm/mat4x4.hpp>
#include <map>

namespace RGL
{
    class AnimatedModel : public StaticModel
    {
    public:
        AnimatedModel() : m_bones_count             (0), 
                          m_global_inverse_transform(glm::mat4(1.0)), 
                          m_assimp_scene            (nullptr), 
                          m_animation_speed         (1.0),
                          m_current_animation_time  (0.0), 
                          m_current_animation       (0), 
                          m_animations_count        (0) {}

        virtual ~AnimatedModel() {}

        void BoneTransform(float dt, std::vector<glm::mat4> & transforms);
        bool Load         (const std::filesystem::path& filepath, bool srgb_textures = true) override;

        std::vector<std::string> GetAnimationsNames() const;
        uint32_t                 GetAnimationsCount() const { return m_animations_count; }
        uint32_t                 GetBonesCount()      const { return m_bones_count; }

        void SetAnimation(uint32_t animation_index) 
        { 
            m_current_animation      = std::max(0u, std::min(animation_index, m_animations_count - 1)); 
            m_current_animation_time = 0.0f; 
        }

        void SetAnimationSpeed(float speed)
        {
            m_animation_speed = std::max(speed, 0.0f);
        }

    protected:
        static const uint32_t NUM_BONES_PER_VERTEX = 4;

        struct BoneInfo
        {
            glm::mat4 m_bone_offset;
            glm::mat4 m_final_transform;

            BoneInfo()
            {
                m_bone_offset     = glm::mat4(0.0);
                m_final_transform = glm::mat4(0.0);
            }
        };

        struct VertexBoneData
        {
            int   m_ids[NUM_BONES_PER_VERTEX];
            float m_weights[NUM_BONES_PER_VERTEX];
            
            VertexBoneData()
            {
                Reset();
            }

            void Reset()
            {
                memset(m_ids,     0, sizeof(m_ids));
                memset(m_weights, 0, sizeof(m_weights));
            }

            void AddBoneData(int bone_id, float weight)
            {
                for (uint32_t i = 0; i < NUM_BONES_PER_VERTEX; ++i)
                {
                    if (m_weights[i] == 0.0)
                    {
                        m_ids    [i] = bone_id;
                        m_weights[i] = weight;
                        return;
                    }
                }

                // should never get here - more bones that we have space for
                assert(0);
            }
        };

        glm::mat4& operator=(const aiMatrix4x4& from)
        {
            glm::mat4 to;

            // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
            to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
            to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
            to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
            to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;

            return to;
        }

        virtual void CalcInterpolatedScaling (aiVector3D&   out, float animation_time, const aiNodeAnim* node_anim);
        virtual void CalcInterpolatedRotation(aiQuaternion& out, float animation_time, const aiNodeAnim* node_anim);
        virtual void CalcInterpolatedPosition(aiVector3D&   out, float animation_time, const aiNodeAnim* node_anim);

        virtual uint32_t FindScaling (float animation_time, const aiNodeAnim* node_anim);
        virtual uint32_t FindRotation(float animation_time, const aiNodeAnim* node_anim);
        virtual uint32_t FindPosition(float animation_time, const aiNodeAnim* node_anim);

        virtual const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, std::string_view node_name);
        virtual void ReadNodeHierarchy(float animation_time, const aiNode* node, const glm::mat4& parent_transform);

        virtual void LoadBones(uint32_t mesh_index, const aiMesh* mesh, std::vector<VertexBoneData>& bones);
        virtual bool ParseScene(const aiScene* scene, const std::filesystem::path& filepath, bool srgb_textures) override;

        virtual void LoadMeshPart(uint32_t mesh_index, const aiMesh* mesh, VertexData& vertex_data, std::vector<VertexBoneData>& bones_data);
        virtual void CreateBuffers(VertexData& vertex_data, std::vector<VertexBoneData>& bones_data);
        
        std::map<std::string, uint32_t> m_bones_mapping;
        std::vector<BoneInfo>           m_bone_infos;

        uint32_t  m_bones_count;
        glm::mat4 m_global_inverse_transform;

        const aiScene*   m_assimp_scene;
        Assimp::Importer m_importer;

        float    m_animation_speed;
        float    m_current_animation_time;
        uint32_t m_current_animation;
        uint32_t m_animations_count;
    };
}