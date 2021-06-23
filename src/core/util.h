#pragma once

#include <random>
#include <string>
#include <stb_image.h>
#include <glad/glad.h>
#include <glm/vec3.hpp>

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
        static std::string LoadFile(const std::string & filename);

        static std::string LoadShaderIncludes(const std::string & shader_code, const std::string & dir = "shaders/");
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
        static unsigned char* LoadTextureData   (std::string_view filepath, ImageData & image_data, int desired_number_of_channels = 0);
        static void           ReleaseTextureData(unsigned char* data);

        static double RandomDouble()
        {
            // Returns a random real in [0,1).
            static std::uniform_real_distribution<double> distribution(0.0, 1.0);
            static std::mt19937 generator;
            return distribution(generator);
        }

        static double RandomDouble(double min, double max)
        {
            // Returns a random real in [min, max).
            return min + (max - min) * RandomDouble();
        }

        static int RandomInt(int min, int max)
        {
            // Returns a random integer in [min, max].
            return static_cast<int>(RandomDouble(min, max + 1));
        }

        static glm::vec3 RandomVec3(double min, double max)
        {
            // Returns a random vec3 in [min, max).
            return glm::vec3(RandomDouble(min, max), RandomDouble(min, max), RandomDouble(min, max));
        }
    };
}