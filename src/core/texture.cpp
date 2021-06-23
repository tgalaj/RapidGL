#include "texture.h"

#include <glm/glm.hpp>

namespace RGL
{
    TextureSampler::TextureSampler() : m_so_id(0), m_max_anisotropy(1.0f)
    {
    }

    void TextureSampler::Create()
    {
        glCreateSamplers(1, &m_so_id);
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &m_max_anisotropy);

        SetFiltering(Filtering::MIN,       FilteringParam::LINEAR_MIP_LINEAR);
        SetFiltering(Filtering::MAG,       FilteringParam::LINEAR);
        SetWraping  (WrapingCoordinate::S, WrapingParam::CLAMP_TO_EDGE);
        SetWraping  (WrapingCoordinate::T, WrapingParam::CLAMP_TO_EDGE);
    }

    void TextureSampler::SetFiltering(Filtering type, FilteringParam param)
    {
        if (type == Filtering::MAG && param > FilteringParam::LINEAR)
        {
            param = FilteringParam::LINEAR;
        }

        glSamplerParameteri(m_so_id, GLenum(type), GLint(param));
    }

    void TextureSampler::SetMinLod(float min)
    {
        glSamplerParameterf(m_so_id, GL_TEXTURE_MIN_LOD, min);
    }

    void TextureSampler::SetMaxLod(float max)
    {
        glSamplerParameterf(m_so_id, GL_TEXTURE_MAX_LOD, max);
    }

    void TextureSampler::SetWraping(WrapingCoordinate coord, WrapingParam param)
    {
        glSamplerParameteri(m_so_id, GLenum(coord), GLint(param));
    }

    void TextureSampler::SetBorderColor(float r, float g, float b, float a)
    {
        float color[4] = { r, g, b, a };
        glSamplerParameterfv(m_so_id, GL_TEXTURE_BORDER_COLOR, color);
    }

    void TextureSampler::SetCompareMode(CompareMode mode)
    {
        glSamplerParameteri(m_so_id, GL_TEXTURE_COMPARE_MODE, GLint(mode));
    }

    void TextureSampler::SetCompareFunc(CompareFunc func)
    {
        glSamplerParameteri(m_so_id, GL_TEXTURE_COMPARE_FUNC, GLint(func));
    }

    void TextureSampler::SetAnisotropy(float anisotropy)
    {
        std::clamp(anisotropy, 1.0f, m_max_anisotropy);
        glSamplerParameterf(m_so_id, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
    }

    bool Texture2D::Load(std::string_view filepath, bool is_srgb, uint32_t num_mipmaps)
    {
        auto data = Util::LoadTextureData(filepath, m_metadata);

        if (!data)
        {
            fprintf(stderr, "Texture failed to load at path: %s\n", std::string(filepath).c_str());
            return false;
        }

        GLenum format          = 0;
        GLenum internal_format = 0;

        if (m_metadata.channels == 1)
        {
            format          = GL_RED;
            internal_format = GL_R8;
        }
        else if (m_metadata.channels == 3)
        {
            format          = GL_RGB;
            internal_format = is_srgb ? GL_SRGB8 : GL_RGB8;
        }
        else if (m_metadata.channels == 4)
        {
            format          = GL_RGBA;
            internal_format = is_srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        }

        glCreateTextures       (GLenum(TextureType::Texture2D), 1, &m_obj_name);
        glTextureStorage2D     (m_obj_name, num_mipmaps /* levels */, internal_format, m_metadata.width, m_metadata.height);
        glTextureSubImage2D    (m_obj_name, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, m_metadata.width, m_metadata.height, format, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(m_obj_name);

        Util::ReleaseTextureData(data);

        return true;
    }

    bool TextureCubeMap::Load(std::string_view* filepaths, bool is_srgb, uint32_t num_mipmaps)
    {
        constexpr int NUM_FACES = 6;

        unsigned char* images_data[NUM_FACES];

        for (int i = 0; i < NUM_FACES; ++i)
        {
            images_data[i] = Util::LoadTextureData(filepaths[i], m_metadata);

            if (!images_data[i])
            {
                fprintf(stderr, "Texture failed to load at path: %s\n", std::string(filepaths[i]).c_str());
                return false;
            }
        }

        GLuint m_format          = m_metadata.channels == 4 ? GL_RGBA         : GL_RGB;
        GLuint m_internal_format = is_srgb                  ? GL_SRGB8_ALPHA8 : GL_RGBA8;

        const GLuint max_num_mipmaps = 1 + glm::floor(glm::log2(glm::max(float(m_metadata.width), float(m_metadata.height))));
                     num_mipmaps     = glm::clamp(num_mipmaps, 1u, max_num_mipmaps);

        glCreateTextures  (GLenum(TextureType::TextureCubeMap), 1, &m_obj_name);
        glTextureStorage2D(m_obj_name, num_mipmaps, m_internal_format, m_metadata.width, m_metadata.height);

        for (int i = 0; i < NUM_FACES; ++i)
        {
            glTextureSubImage3D(m_obj_name, 
                                0 /*level*/, 
                                0 /*xoffset*/, 
                                0 /*yoffset*/,
                                i /*zoffset*/,
                                m_metadata.width,
                                m_metadata.height,
                                1 /*depth*/,
                                m_format,
                                GL_UNSIGNED_BYTE,
                                images_data[i]);
        }

        glGenerateTextureMipmap(m_obj_name);

        for (int i = 0; i < NUM_FACES; ++i)
        {
            /* Release images' data */
            Util::ReleaseTextureData(images_data[i]);
        }

        return true;
    }

}