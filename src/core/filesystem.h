#pragma once

#include <filesystem>
#include <string>

namespace RGL
{
    namespace fs = std::filesystem;

    class FileSystem
    {
    public:
        static std::string getPath(const std::string& path);
        static bool directoryExists(const fs::path& path, fs::file_status status = fs::file_status{});
        static void createDirectory(const std::string& directory_name);

    private:
        static const std::string& getRoot();
        static std::string getPathRelativeRoot(const std::string& path);
    };
}