#include "util.h"

#include <fstream>
#include <sstream>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>

#include "filesystem.h"

namespace RGL
{
    std::string Util::LoadFile(const std::filesystem::path & filename)
    {
        if (filename.empty())
        {
            return "";
        }

        std::string filetext;
        std::string line;

        std::filesystem::path filepath = FileSystem::getRootPath() / filename;
        std::ifstream inFile(filepath);

        if (!inFile)
        {
            fprintf(stderr, "Could not open file %s\n", filepath.string().c_str());
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

    std::vector<unsigned char> Util::LoadFileBinary(const std::filesystem::path& filename)
    {
        std::filesystem::path filepath = FileSystem::getRootPath() / filename;
        std::ifstream file(filepath, std::ios::binary);

        if (!file)
        {
            fprintf(stderr, "Could not open file %s\n", filepath.string().c_str());
            file.close();

            return {};
        }

        // Determine the file size.
        file.seekg(0, std::ios_base::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios_base::beg);

        // Allocate storage.
        std::vector<unsigned char> data(file_size / sizeof(unsigned char));

        // Read the file contents into the allocated storage.
        file.read((char*)&data[0], file_size);

        return data;
    }

    std::string Util::LoadShaderIncludes(const std::string& shader_code, const std::filesystem::path& dir)
    {
        std::istringstream ss(shader_code);

        std::string line, new_shader_code = "";
        std::string include_phrase        = "#include";

        bool included = false;

        while (std::getline(ss, line))
        {
            if (line.substr(0, include_phrase.size()) == include_phrase)
            {
                std::string include_file_name = line.substr(include_phrase.size() + 2, line.size() - include_phrase .size() - 3);
                
                line     = LoadFile(dir / include_file_name);
                included = true;
            }

            new_shader_code.append(line + "\n");
        }

        // Parse #include in the included files
        if (included)
        {
            new_shader_code = LoadShaderIncludes(new_shader_code, dir);
        }

        return new_shader_code;
    }


    unsigned char* Util::LoadTextureData(const std::filesystem::path& filepath, ImageData & image_data, int desired_number_of_channels)
    {
        int width, height, channels_in_file;
        unsigned char* data = stbi_load(filepath.generic_string().c_str(), &width, &height, &channels_in_file, desired_number_of_channels);

        if (data)
        {
            image_data.width    = width;
            image_data.height   = height;
            image_data.channels = desired_number_of_channels == 0 ? channels_in_file : desired_number_of_channels;
        }

        return data;
    }

    unsigned char* Util::LoadTextureData(unsigned char* memory_data, uint32_t data_size, ImageData& image_data, int desired_number_of_channels)
    {
        int width, height, channels_in_file;
        unsigned char* data = stbi_load_from_memory(memory_data, data_size, &width, &height, &channels_in_file, desired_number_of_channels);
        
        if (data)
        {
            image_data.width    = width;
            image_data.height   = height;
            image_data.channels = desired_number_of_channels == 0 ? channels_in_file : desired_number_of_channels;
        }

        return data;
    }

    float* Util::LoadTextureDataHdr(const std::filesystem::path& filepath, ImageData& image_data, int desired_number_of_channels)
    {
        stbi_set_flip_vertically_on_load(true);
            int width, height, channels_in_file;
            float* data = stbi_loadf(filepath.generic_string().c_str(), &width, &height, &channels_in_file, desired_number_of_channels);
        stbi_set_flip_vertically_on_load(false);
        if (data)
        {
            image_data.width    = width;
            image_data.height   = height;
            image_data.channels = desired_number_of_channels == 0 ? channels_in_file : desired_number_of_channels;
        }

        return data;
    }

    void Util::ReleaseTextureData(unsigned char* data)
    {
        stbi_image_free(data);
    }

    void Util::ReleaseTextureData(float* data)
    {
        stbi_image_free(data);
    }
}
