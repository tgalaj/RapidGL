#pragma once

#include <random>
#include <string>
#include <stb_image.h>
#include <glad/glad.h>

namespace RGL
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
        
        static unsigned int loadGLTexture2D(const char* path, const std::string& directory, bool gamma);
        static unsigned int loadGLTextureCube(const std::string* filenames, const std::string& directory, GLuint num_mipmaps = 1u, bool gamma = false);

        static double randomDouble()
        {
            // Returns a random real in [0,1).
            static std::uniform_real_distribution<double> distribution(0.0, 1.0);
            static std::mt19937 generator;
            return distribution(generator);
        }

        static double randomDouble(double min, double max)
        {
            // Returns a random real in [min,max).
            return min + (max - min) * randomDouble();
        }

        static int randomInt(int min, int max)
        {
            // Returns a random integer in [min, max].
            return static_cast<int>(randomDouble(min, max + 1));
        }
    };
}