#include "filesystem.h"
#include "root_directory.h"

namespace RapidGL
{
    std::string FileSystem::getPath(const std::string& path)
    {
        return getPathRelativeRoot(path);
    }

    bool FileSystem::directoryExists(const fs::path& path, fs::file_status status)
    {
        if (fs::status_known(status) ? fs::exists(status) : fs::exists(path))
        {
            return true;
        }

        return false;
    }

    void FileSystem::createDirectory(const std::string& directory_name)
    {
        fs::create_directories(directory_name);
    }

    const std::string& FileSystem::getRoot()
    {
        static std::string root = (rapidgl_root != nullptr ? rapidgl_root : "");

        return root;
    }

    std::string FileSystem::getPathRelativeRoot(const std::string& path)
    {
        return getRoot() + "/" + path;
    }
}