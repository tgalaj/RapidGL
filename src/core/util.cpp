#include "util.h"

#include <fstream>
#include <sstream>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>

#include "filesystem.h"

namespace RGL
{
    std::string Util::LoadFile(const std::string & filename)
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

    std::string Util::LoadShaderIncludes(const std::string & shader_code, const std::string& dir)
    {
        std::istringstream ss(shader_code);

        std::string line, new_shader_code = "";
        std::string include_phrase = "#include";

        while(std::getline(ss, line))
        {
            if(line.substr(0, include_phrase.size()) == include_phrase)
            {
                std::string include_file_name = line.substr(include_phrase.size() + 2, line.size() - include_phrase .size() - 3);
                line = LoadFile(dir + include_file_name);
            }

            new_shader_code.append(line + "\n");
        }

        return new_shader_code;
    }


    unsigned char* Util::LoadTextureData(std::string_view filepath, ImageData & image_data, int desired_number_of_channels)
    {
        int width, height, nr_channels;
        unsigned char* data = stbi_load(std::string(filepath).c_str(), &width, &height, &nr_channels, desired_number_of_channels);

        if (data)
        {
            image_data.width    = width;
            image_data.height   = height;
            image_data.channels = desired_number_of_channels == 0 ? nr_channels : desired_number_of_channels;
        }

        return data;
    }

    void Util::ReleaseTextureData(unsigned char* data)
    {
        stbi_image_free(data);
    }
}
