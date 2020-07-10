#pragma once

#include <string>
#include <stb_image.h>
#include <glad/glad.h>

namespace RapidGL
{
    struct ImageData
    {
        ImageData()
            : width(0),
              height(0),
              channels(0)
        {}

        GLuint width;
        GLuint height;
        GLuint channels;
    };

    class Util
    {
    public:
        /**
        * @brief   Loads a file in a text mode.
        * @param   std::string Relative path, with file name
        *                      and extension, to the file that
        *                      needs to be loaded.
        * @returns Full file's source as a std::string.
        */
        static std::string loadFile(const std::string & filename);

        static std::string loadShaderIncludes(const std::string & shader_code, const std::string & dir = "shaders/");
        /**
        * @brief   Loads a file that contains an image data.
        * @param   std::string Relative path, with file name
        *          and extension, to the file that needs to be
        *          loaded.
        * @param   image_data
        * @param   desired_number_of_channels
        * @returns Pointer to unsigned char that contains image's data.
        *          Has to be freed with stbi_image_free(data)!
        */
        static unsigned char* loadTextureData(const std::string & filename, ImageData & image_data, int desired_number_of_channels = 0);
        
        static unsigned int loadGLTexture(const char* path, const std::string& directory, bool gamma);
    };
}