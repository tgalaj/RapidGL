#pragma once
#include "util.h"

#include <glad/glad.h>
#include <string_view>
#include <filesystem>>

namespace RGL
{
    enum class TextureType              { NONE = 0, Texture2D = GL_TEXTURE_2D, TextureCubeMap = GL_TEXTURE_CUBE_MAP };
    enum class TextureFiltering         { MAG                  = GL_TEXTURE_MAG_FILTER,
                                          MIN                  = GL_TEXTURE_MIN_FILTER };
    enum class TextureFilteringParam    { NEAREST              = GL_NEAREST,
                                          LINEAR               = GL_LINEAR, 
                                          NEAREST_MIP_NEAREST  = GL_NEAREST_MIPMAP_NEAREST, 
                                          LINEAR_MIP_NEAREST   = GL_LINEAR_MIPMAP_NEAREST,
                                          NEAREST_MIP_LINEAR   = GL_NEAREST_MIPMAP_LINEAR,
                                          LINEAR_MIP_LINEAR    = GL_LINEAR_MIPMAP_LINEAR };
    enum class TextureWrapingCoordinate { S                    = GL_TEXTURE_WRAP_S,
                                          T                    = GL_TEXTURE_WRAP_T,
                                          R                    = GL_TEXTURE_WRAP_R };
    enum class TextureWrapingParam      { REPEAT               = GL_REPEAT,
                                          MIRRORED_REPEAT      = GL_MIRRORED_REPEAT,
                                          CLAMP_TO_EDGE        = GL_CLAMP_TO_EDGE,
                                          CLAMP_TO_BORDER      = GL_CLAMP_TO_BORDER,
                                          MIRROR_CLAMP_TO_EDGE = GL_MIRROR_CLAMP_TO_EDGE };
    enum class TextureCompareMode       { NONE                 = GL_NONE,
                                          REF                  = GL_COMPARE_REF_TO_TEXTURE };
    enum class TextureCompareFunc       { NEVER                = GL_NEVER,
                                          ALWAYS               = GL_ALWAYS, 
                                          LEQUAL               = GL_LEQUAL, 
                                          GEQUAL               = GL_GEQUAL, 
                                          LESS                 = GL_LESS, 
                                          GREATER              = GL_GREATER, 
                                          EQUAL                = GL_EQUAL, 
                                          NOTEQUAL             = GL_NOTEQUAL };

    class Texture
    {
    public:
        virtual ~Texture() { Release(); };

        Texture           (const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept : m_metadata(other.m_metadata), m_type(other.m_type), m_obj_name(other.m_obj_name)
        {
            other.m_obj_name = 0;
        }

        Texture& operator=(Texture&& other) noexcept
        {
            if (this != &other)
            {
                Release();

                std::swap(m_metadata, other.m_metadata);
                std::swap(m_type,     other.m_type);
                std::swap(m_obj_name, other.m_obj_name);
            }

            return *this;
        }

        virtual void Bind(uint32_t unit) { glBindTextureUnit(unit, m_obj_name); }
        virtual void SetFiltering(TextureFiltering type, TextureFilteringParam param);
        virtual void SetMinLod(float min);
        virtual void SetMaxLod(float max);
        virtual void SetWraping(TextureWrapingCoordinate coord, TextureWrapingParam param);
        virtual void SetBorderColor(float r, float g, float b, float a);
        virtual void SetCompareMode(TextureCompareMode mode);
        virtual void SetCompareFunc(TextureCompareFunc func);
        virtual void SetAnisotropy(float anisotropy);
        
        virtual ImageData GetMetadata() const { return m_metadata; };

    protected:
        Texture() : m_type(TextureType::NONE), m_obj_name(0) {}

        void Release()
        {
            glDeleteTextures(1, &m_obj_name);
            m_obj_name = 0;
        }

        ImageData   m_metadata;
        TextureType m_type;
        GLuint      m_obj_name;
    };

    class Texture2D : public Texture
    {
    public:
        Texture2D() = default;
        bool Load(std::string_view filepath, bool is_srgb = false, uint32_t num_mipmaps = 1);
        bool Load(unsigned char* memory_data, uint32_t data_size, bool is_srgb = false, uint32_t num_mipmaps = 1);
        bool LoadHdr(const std::filesystem::path& filepath);
    };

    class TextureCubeMap : public Texture
    {
    public:
        TextureCubeMap() = default;
        bool Load(std::string* filepaths, bool is_srgb = false, uint32_t num_mipmaps = 1);
    };
}