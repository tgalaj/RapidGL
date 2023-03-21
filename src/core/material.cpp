#include "material.h"

namespace RGL
{
    Material::Material()
    {
        AddVector3("u_albedo",            glm::vec3(1.0f));
        AddVector3("u_emission",          glm::vec3(0.0f));
        AddFloat  ("u_ao",                1.0f);
        AddFloat  ("u_roughness",         0.0f);
        AddFloat  ("u_metallic",          0.0f);
        AddBool   ("u_has_albedo_map",    false);
        AddBool   ("u_has_normal_map",    false);
        AddBool   ("u_has_emissive_map",  false);
        AddBool   ("u_has_ao_map",        false);
        AddBool   ("u_has_metallic_map",  false);
        AddBool   ("u_has_roughness_map", false);
    }

    Material::~Material()
    {
    }

    void Material::AddTexture(TextureType texture_type, const std::shared_ptr<Texture2D>& texture)
    {
        m_texture_map[texture_type] = texture;
    }

    void Material::AddVector3(const std::string& uniform_name, const glm::vec3& vector3)
    {
        m_vec3_map[uniform_name] = vector3;
    }

    void Material::AddFloat(const std::string& uniform_name, float value)
    {
        m_float_map[uniform_name] = value;
    }

    void Material::AddBool(const std::string& uniform_name, bool value)
    {
        m_bool_map[uniform_name] = value;
    }

    std::shared_ptr<Texture2D> Material::GetTexture(TextureType texture_type)
    {
        if (m_texture_map.contains(texture_type))
        {
            return m_texture_map[texture_type];
        }

        assert(false && "Couldn't find texture with the specified texture type!");

        return nullptr;
    }
    
    glm::vec3 Material::GetVector3(const std::string& uniform_name)
    {
        if (m_vec3_map.contains(uniform_name))
        {
            return m_vec3_map[uniform_name];
        }

        return glm::vec3(0.0f);
    }

    float Material::GetFloat(const std::string& uniform_name)
    {
        if (m_float_map.contains(uniform_name))
        {
            return m_float_map[uniform_name];
        }

        return 0.0f;
    }

    bool Material::GetBool(const std::string& uniform_name)
    {
        if (m_bool_map.contains(uniform_name))
        {
            return m_bool_map[uniform_name];
        }

        return false;
    }
}