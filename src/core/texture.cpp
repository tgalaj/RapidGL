#include "texture.h"

#include <glm/glm.hpp>

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

using namespace tinyddsloader;

namespace
{
    struct GLSwizzle
    {
        GLenum m_r, m_g, m_b, m_a;
    };

    struct GLFormat
    {
        DDSFile::DXGIFormat m_dxgiFormat;
        GLenum m_type;
        GLenum m_format;
        GLSwizzle m_swizzle;
    };

    bool translateDdsFormat(DDSFile::DXGIFormat fmt, GLFormat* outFormat)
    {
        static const GLSwizzle sws[] = {
            {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA},
            {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA},
            {GL_BLUE, GL_GREEN, GL_RED, GL_ONE},
            {GL_RED, GL_GREEN, GL_BLUE, GL_ONE},
            {GL_RED, GL_ZERO, GL_ZERO, GL_ZERO},
            {GL_RED, GL_GREEN, GL_ZERO, GL_ZERO},
        };
        using DXGIFmt = DDSFile::DXGIFormat;
        static const GLFormat formats[] = {
            {DXGIFmt::R8G8B8A8_UNorm, GL_UNSIGNED_BYTE, GL_RGBA, sws[0]},
            {DXGIFmt::B8G8R8A8_UNorm, GL_UNSIGNED_BYTE, GL_RGBA, sws[1]},
            {DXGIFmt::B8G8R8X8_UNorm, GL_UNSIGNED_BYTE, GL_RGBA, sws[2]},
            {DXGIFmt::BC1_UNorm, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, sws[0]},
            {DXGIFmt::BC2_UNorm, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, sws[0]},
            {DXGIFmt::BC3_UNorm, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, sws[0]},
            {DXGIFmt::BC4_UNorm, 0, GL_COMPRESSED_RED_RGTC1_EXT, sws[0]},
            {DXGIFmt::BC4_SNorm, 0, GL_COMPRESSED_SIGNED_RED_RGTC1_EXT, sws[0]},
            {DXGIFmt::BC5_UNorm, 0, GL_COMPRESSED_RED_GREEN_RGTC2_EXT, sws[0]},
            {DXGIFmt::BC5_SNorm, 0, GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT,
             sws[0]},
        };
        for (const auto& format : formats)
        {
            if (format.m_dxgiFormat == fmt)
            {
                if (outFormat)
                {
                    *outFormat = format;
                }
                return true;
            }
        }
        return false;
    }

    bool isDdsCompressed(GLenum fmt)
    {
        switch (fmt)
        {
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_RED_RGTC1_EXT:
            case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
            case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
            case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:
                return true;
            default:
                return false;
        }
    }
}

namespace RGL
{
    // --------------------- Texture Sampler -------------------------
    TextureSampler::TextureSampler() : m_so_id(0), m_max_anisotropy(1.0f) {}

    void TextureSampler::Create()
    {
        glCreateSamplers(1, &m_so_id);

        SetFiltering(TextureFiltering::MIN,       TextureFilteringParam::LINEAR_MIP_LINEAR);
        SetFiltering(TextureFiltering::MAG,       TextureFilteringParam::LINEAR);
        SetWraping  (TextureWrapingCoordinate::S, TextureWrapingParam::CLAMP_TO_EDGE);
        SetWraping  (TextureWrapingCoordinate::T, TextureWrapingParam::CLAMP_TO_EDGE);
    }

    void TextureSampler::SetFiltering(TextureFiltering type, TextureFilteringParam param)
    {
        if (type == TextureFiltering::MAG && param > TextureFilteringParam::LINEAR)
        {
            param = TextureFilteringParam::LINEAR;
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

    void TextureSampler::SetWraping(TextureWrapingCoordinate coord, TextureWrapingParam param)
    {
        glSamplerParameteri(m_so_id, GLenum(coord), GLint(param));
    }

    void TextureSampler::SetBorderColor(float r, float g, float b, float a)
    {
        float color[4] = { r, g, b, a };
        glSamplerParameterfv(m_so_id, GL_TEXTURE_BORDER_COLOR, color);
    }

    void TextureSampler::SetCompareMode(TextureCompareMode mode)
    {
        glSamplerParameteri(m_so_id, GL_TEXTURE_COMPARE_MODE, GLint(mode));
    }

    void TextureSampler::SetCompareFunc(TextureCompareFunc func)
    {
        glSamplerParameteri(m_so_id, GL_TEXTURE_COMPARE_FUNC, GLint(func));
    }

    void TextureSampler::SetAnisotropy(float anisotropy)
    {
        float max_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);

        glm::clamp(anisotropy, 1.0f, max_anisotropy);
        glSamplerParameterf(m_so_id, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
    }

    // --------------------- Texture -------------------------

    void Texture::SetFiltering(TextureFiltering type, TextureFilteringParam param)
    {
        if (type == TextureFiltering::MAG && param > TextureFilteringParam::LINEAR)
        {
            param = TextureFilteringParam::LINEAR;
        }

        glTextureParameteri(m_obj_name, GLenum(type), GLint(param));
    }

    void Texture::SetMinLod(float min)
    {
        glTextureParameterf(m_obj_name, GL_TEXTURE_MIN_LOD, min);
    }

    void Texture::SetMaxLod(float max)
    {
        glTextureParameterf(m_obj_name, GL_TEXTURE_MAX_LOD, max);
    }

    void Texture::SetWraping(TextureWrapingCoordinate coord, TextureWrapingParam param)
    {
        glTextureParameteri(m_obj_name, GLenum(coord), GLint(param));
    }

    void Texture::SetBorderColor(float r, float g, float b, float a)
    {
        float color[4] = { r, g, b, a };
        glTextureParameterfv(m_obj_name, GL_TEXTURE_BORDER_COLOR, color);
    }

    void Texture::SetCompareMode(TextureCompareMode mode)
    {
        glTextureParameteri(m_obj_name, GL_TEXTURE_COMPARE_MODE, GLint(mode));
    }

    void Texture::SetCompareFunc(TextureCompareFunc func)
    {
        glTextureParameteri(m_obj_name, GL_TEXTURE_COMPARE_FUNC, GLint(func));
    }

    void Texture::SetAnisotropy(float anisotropy)
    {
        float max_anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);

        glm::clamp(anisotropy, 1.0f, max_anisotropy);
        glTextureParameterf(m_obj_name, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
    }

    // --------------------- Texture2D -------------------------

    bool Texture2D::Load(const std::filesystem::path& filepath, bool is_srgb, uint32_t num_mipmaps)
    {
        auto data = Util::LoadTextureData(filepath, m_metadata);

        if (!data)
        {
            fprintf(stderr, "Texture failed to load at path: %s\n", filepath.string().c_str());
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

        const GLuint max_num_mipmaps = GetMaxMipMapsLevels(m_metadata.width, m_metadata.height, 0);
                     num_mipmaps     = num_mipmaps == 0 ? max_num_mipmaps : glm::clamp(num_mipmaps, 1u, max_num_mipmaps);

        glCreateTextures       (GLenum(TextureType::Texture2D), 1, &m_obj_name);
        glTextureStorage2D     (m_obj_name, num_mipmaps /* levels */, internal_format, m_metadata.width, m_metadata.height);
        glTextureSubImage2D    (m_obj_name, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, m_metadata.width, m_metadata.height, format, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(m_obj_name);

        SetFiltering(TextureFiltering::MIN,       TextureFilteringParam::LINEAR_MIP_LINEAR);
        SetFiltering(TextureFiltering::MAG,       TextureFilteringParam::LINEAR);
        SetWraping  (TextureWrapingCoordinate::S, TextureWrapingParam::CLAMP_TO_EDGE);
        SetWraping  (TextureWrapingCoordinate::T, TextureWrapingParam::CLAMP_TO_EDGE);

        Util::ReleaseTextureData(data);

        return true;
    }

    bool Texture2D::Load(unsigned char* memory_data, uint32_t data_size, bool is_srgb, uint32_t num_mipmaps)
    {
        auto data = Util::LoadTextureData(memory_data, data_size, m_metadata);

        if (!data)
        {
            fprintf(stderr, "Texture failed to load from the memory.\n");
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

        const GLuint max_num_mipmaps = GetMaxMipMapsLevels(m_metadata.width, m_metadata.height, 0);
                     num_mipmaps     = num_mipmaps == 0 ? max_num_mipmaps : glm::clamp(num_mipmaps, 1u, max_num_mipmaps);


        glCreateTextures       (GLenum(TextureType::Texture2D), 1, &m_obj_name);
        glTextureStorage2D     (m_obj_name, num_mipmaps /* levels */, internal_format, m_metadata.width, m_metadata.height);
        glTextureSubImage2D    (m_obj_name, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, m_metadata.width, m_metadata.height, format, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(m_obj_name);

        SetFiltering(TextureFiltering::MIN,       TextureFilteringParam::LINEAR_MIP_LINEAR);
        SetFiltering(TextureFiltering::MAG,       TextureFilteringParam::LINEAR);
        SetWraping  (TextureWrapingCoordinate::S, TextureWrapingParam::CLAMP_TO_EDGE);
        SetWraping  (TextureWrapingCoordinate::T, TextureWrapingParam::CLAMP_TO_EDGE);

        Util::ReleaseTextureData(data);

        return true;
    }

    bool Texture2D::LoadHdr(const std::filesystem::path & filepath, uint32_t num_mipmaps)
    {
        if (filepath.extension() != ".hdr")
        {
            fprintf(stderr, "This function is meant for loading HDR images only.\n");
            return false;
        }

        auto data = Util::LoadTextureDataHdr(filepath, m_metadata);

        if (!data)
        {
            fprintf(stderr, "Texture failed to load at path: %s\n", filepath.generic_string().c_str());
            return false;
        }

        GLenum format          = GL_RGB;
        GLenum internal_format = GL_RGB16F;

        const GLuint max_num_mipmaps = GetMaxMipMapsLevels(m_metadata.width, m_metadata.height, 0);
                     num_mipmaps     = num_mipmaps == 0 ? max_num_mipmaps : glm::clamp(num_mipmaps, 1u, max_num_mipmaps);

        glCreateTextures       (GLenum(TextureType::Texture2D), 1, &m_obj_name);
        glTextureStorage2D     (m_obj_name, 1 /* levels */, internal_format, m_metadata.width, m_metadata.height);
        glTextureSubImage2D    (m_obj_name, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, m_metadata.width, m_metadata.height, format, GL_FLOAT, data);
        glGenerateTextureMipmap(m_obj_name);

        SetFiltering(TextureFiltering::MIN,       TextureFilteringParam::LINEAR);
        SetFiltering(TextureFiltering::MAG,       TextureFilteringParam::LINEAR);
        SetWraping  (TextureWrapingCoordinate::S, TextureWrapingParam::CLAMP_TO_EDGE);
        SetWraping  (TextureWrapingCoordinate::T, TextureWrapingParam::CLAMP_TO_EDGE);

        Util::ReleaseTextureData(data);

        return true;
    }

    bool Texture2D::LoadDds(const std::filesystem::path& filepath)
    {
        DDSFile dds;
        auto ret = dds.Load(filepath.string().c_str());

        if (Result::Success != ret)
        {
            std::cout << "Failed to load.[" << filepath << "]\n";
            std::cout << "Result : " << int(ret) << "\n";

            fprintf(stderr, "Texture failed to load at path: %s\n", filepath.string().c_str());
            fprintf(stderr, "Result: %d", int(ret));
            return false;
        }

        if (dds.GetTextureDimension() == DDSFile::TextureDimension::Texture2D)
        {
            m_type = TextureType::Texture2D;
        }

        GLFormat format;
        if (!translateDdsFormat(dds.GetFormat(), &format))
        {
            return false;
        }

        glCreateTextures   (GLenum(m_type), 1, &m_obj_name);
        glTextureParameteri(m_obj_name, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(m_obj_name, GL_TEXTURE_MAX_LEVEL, dds.GetMipCount() - 1);
        glTextureParameteri(m_obj_name, GL_TEXTURE_SWIZZLE_R, format.m_swizzle.m_r);
        glTextureParameteri(m_obj_name, GL_TEXTURE_SWIZZLE_G, format.m_swizzle.m_g);
        glTextureParameteri(m_obj_name, GL_TEXTURE_SWIZZLE_B, format.m_swizzle.m_b);
        glTextureParameteri(m_obj_name, GL_TEXTURE_SWIZZLE_A, format.m_swizzle.m_a);

        m_metadata.width  = dds.GetWidth();
        m_metadata.height = dds.GetHeight();

        glTextureStorage2D(m_obj_name, dds.GetMipCount(), format.m_format, m_metadata.width, m_metadata.height);
        dds.Flip();

        for (uint32_t level = 0; level < dds.GetMipCount(); level++)
        {
            auto imageData = dds.GetImageData(level, 0);
            switch (GLenum(m_type))
            {
                case GL_TEXTURE_2D:
                {
                    auto w = imageData->m_width;
                    auto h = imageData->m_height;

                    if (isDdsCompressed(format.m_format))
                    {
                        glCompressedTextureSubImage2D(m_obj_name, level, 0, 0, w, h, format.m_format, imageData->m_memSlicePitch, imageData->m_mem);
                    }
                    else
                    {
                        glTextureSubImage2D(m_obj_name, level, 0, 0, w, h, format.m_format, format.m_type, imageData->m_mem);
                    }
                    break;
                }
                default:
                    return false;
            }
        }

        return true;
    }

    // --------------------- Texture CubeMap -------------------------

    bool TextureCubeMap::Load(const std::filesystem::path* filepaths, bool is_srgb, uint32_t num_mipmaps)
    {
        constexpr int NUM_FACES = 6;

        unsigned char* images_data[NUM_FACES];

        for (int i = 0; i < NUM_FACES; ++i)
        {
            images_data[i] = Util::LoadTextureData(filepaths[i], m_metadata);

            if (!images_data[i])
            {
                fprintf(stderr, "Texture failed to load at path: %s\n", filepaths[i].string().c_str());
                return false;
            }
        }

        GLuint m_format          = m_metadata.channels == 4 ? GL_RGBA         : GL_RGB;
        GLuint m_internal_format = is_srgb                  ? GL_SRGB8_ALPHA8 : GL_RGBA8;

        const GLuint max_num_mipmaps = GetMaxMipMapsLevels(m_metadata.width, m_metadata.height, 0);
                     num_mipmaps     = num_mipmaps == 0 ? max_num_mipmaps : glm::clamp(num_mipmaps, 1u, max_num_mipmaps);

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

        SetFiltering(TextureFiltering::MIN,       TextureFilteringParam::LINEAR_MIP_LINEAR);
        SetFiltering(TextureFiltering::MAG,       TextureFilteringParam::LINEAR);
        SetWraping  (TextureWrapingCoordinate::S, TextureWrapingParam::CLAMP_TO_EDGE);
        SetWraping  (TextureWrapingCoordinate::T, TextureWrapingParam::CLAMP_TO_EDGE);
        SetWraping  (TextureWrapingCoordinate::R, TextureWrapingParam::CLAMP_TO_EDGE);

        for (int i = 0; i < NUM_FACES; ++i)
        {
            /* Release images' data */
            Util::ReleaseTextureData(images_data[i]);
        }

        return true;
    }

}