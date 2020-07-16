#include "util.h"

#include <fstream>
#include <sstream>
#include <GLFW\glfw3.h>

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

    unsigned int Util::loadGLTexture(const char* path, const std::string & directory, bool gamma)
    {
        std::string filename(directory + "/" + path);

        unsigned int texture_id;
        glGenTextures(1, &texture_id);

        ImageData img_data;
        auto data = loadTextureData(filename, img_data);

        if (data)
        {
            GLenum format;
            if (img_data.channels == 1)
                format = GL_RED;
            else if (img_data.channels == 3)
                format = GL_RGB;
            else if (img_data.channels == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, format, img_data.width, img_data.height, 0, format, GL_UNSIGNED_BYTE, data);
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
}
