#pragma once
#include <map>
#include "texture.h"

namespace RGL
{
    class Material
    {
    public:
        // Texture type order must match the order in pbr-lighting.glh
        // TextureType is being cast to uint32_t during the mesh rendering.
        enum class TextureType { ALBEDO, NORMAL, METALLIC, ROUGHNESS, AO, EMISSIVE };

        Material();
        ~Material();

        void AddTexture(TextureType texture_type, const std::shared_ptr<Texture2D>& texture);
        void AddVector3(const std::string& uniform_name, const glm::vec3& vector3);
        void AddFloat  (const std::string& uniform_name, float value);
        void AddBool   (const std::string& uniform_name, bool value);

        std::shared_ptr<Texture2D> GetTexture(TextureType texture_type);
        glm::vec3                  GetVector3(const std::string& uniform_name);
        float                      GetFloat  (const std::string& uniform_name);
        bool                       GetBool   (const std::string& uniform_name);

    private:
        std::map<TextureType, std::shared_ptr<Texture2D>> m_texture_map;
        std::map<std::string, glm::vec3>                  m_vec3_map;
        std::map<std::string, float>                      m_float_map;
        std::map<std::string, bool>                       m_bool_map;

        friend class StaticModel;
    };
}