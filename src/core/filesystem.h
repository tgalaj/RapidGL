#pragma once

#include <filesystem>
#include <string>

namespace RGL
{
    namespace fs = std::filesystem;

    class FileSystem
    {
    public:
        static fs::path getRootPath();
        static fs::path getResourcesPath();
        static bool directoryExists(const fs::path & path, fs::file_status status = fs::file_status{});
        static void createDirectory(const fs::path & directory_name);
    };
}