#include "util.h"

#include <fstream>
#include <sstream>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>

#include "filesystem.h"

namespace RapidGL
{
    std::string Util::loadFile(const std::string & filename)
    {
        if (filename.empty())
        {
            return "";
        }

        std::string filetext;
        std::string line;

        std::ifstream inFile(FileSystem::getPath(filename));

        if (!inFile)
        {
            fprintf(stderr, "Could not open file %s\n", FileSystem::getPath(filename).c_str());
            inFile.close();

            return "";
        }

        while (getline(inFile, line))
        {
            filetext.append(line + "\n");
        }

        inFile.close();

        return filetext;
    }

    std::string Util::loadShaderIncludes(const std::string & shader_code, const std::string& dir)
    {
        std::istringstream ss(shader_code);

        std::string line, new_shader_code = "";
        std::string include_phrase = "#include";

        while(std::getline(ss, line))
        {
            if(line.substr(0, include_phrase.size()) == include_phrase)
            {
                std::string include_file_name = line.substr(include_phrase.size() + 2, line.size() - include_phrase .size() - 3);
                line = loadFile(dir + include_file_name);
            }

            new_shader_code.append(line + "\n");
        }

        return new_shader_code;
    }


    unsigned char* Util::loadTextureData(const std::string & filename, ImageData & image_data, int desired_number_of_channels)
    {
        int width, height, nr_channels;
        unsigned char* data = stbi_load(FileSystem::getPath(filename).c_str(), &width, &height, &nr_channels, desired_number_of_channels);

        image_data.width    = width;
        image_data.height   = height;
        image_data.channels = desired_number_of_channels == 0 ? nr_channels : desired_number_of_channels;

        return data;
    }

    unsigned int Util::loadGLTexture2D(const char* path, const std::string & directory, bool gamma)
    {
        std::string filename(directory + "/" + path);

        unsigned int texture_id;
        glGenTextures(1, &texture_id);

        ImageData img_data;
        auto data = loadTextureData(filename, img_data);

        if (data)
        {
            GLenum format;
            GLenum internal_format;
            if (img_data.channels == 1)
            {
                format          = GL_RED;
                internal_format = GL_RED;
            }
            else if (img_data.channels == 3)
            {
                format          = GL_RGB;
                internal_format = gamma ? GL_SRGB : GL_RGB;
            }
            else if (img_data.channels == 4)
            {
                format          = GL_RGBA;
                internal_format = gamma ? GL_SRGB_ALPHA : GL_RGBA;
            }

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, internal_format, img_data.width, img_data.height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (glfwExtensionSupported("GL_ARB_texture_filter_anisotropic"))
            {
                float aniso = 0.0f;
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            }

            stbi_image_free(data);
        }
        else
        {
            fprintf(stderr, "Texture failed to load at path: %s", path);
            stbi_image_free(data);
        }

        return texture_id;
    }

    unsigned Util::loadGLTextureCube(const std::string * filenames, const std::string& directory, GLuint num_mipmaps, bool gamma)
    {
        const int numCubeFaces = 6;

        /* Pointer to the image data */
        ImageData imgs_data[numCubeFaces];
        unsigned char* raw_data[numCubeFaces];

        for (int i = 0; i < numCubeFaces; ++i)
        {
            auto filename = directory + "/" + filenames[i];
            raw_data[i] = loadTextureData(filename, imgs_data[i]);
        }

        GLuint m_format = imgs_data[0].channels == 4 ? GL_RGBA : GL_RGB;
        GLuint m_internal_format = gamma ? GL_SRGB8_ALPHA8 : GL_RGBA8;

        const GLuint max_num_mipmaps = 1 + glm::floor(glm::log2(glm::max(float(imgs_data[0].width), float(imgs_data[0].height))));
        num_mipmaps = glm::clamp(num_mipmaps, 1u, max_num_mipmaps);

        /* Generate GL texture object */
        unsigned int texture_id;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture_id);
        glTextureStorage2D(texture_id,
                           num_mipmaps,
                           m_internal_format,
                           imgs_data[0].width,
                           imgs_data[0].height);

        for (int i = 0; i < numCubeFaces; ++i)
        {
            glTextureSubImage3D(texture_id,
                                0 /*level*/,
                                0 /*xoffset*/,
                                0 /*yoffset*/,
                                i /*zoffset*/,
                                imgs_data[0].width,
                                imgs_data[0].height,
                                1 /*depth*/,
                                m_format,
                                GL_UNSIGNED_BYTE,
                                raw_data[i]);
        }

        glGenerateTextureMipmap(texture_id);

        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(texture_id, GL_TEXTURE_MAX_ANISOTROPY, 16);

        for (int i = 0; i < numCubeFaces; ++i)
        {
            /* Release images' data */
            stbi_image_free(raw_data[i]);
        }

        return texture_id;
    }
}
