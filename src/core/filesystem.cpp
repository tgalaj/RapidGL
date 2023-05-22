#include "filesystem.h"
#include "root_directory.h"

namespace RGL
{
    fs::path FileSystem::getRootPath()
    {
        return fs::path(RAPIDGL_ROOT);
    }

    fs::path FileSystem::getResourcesPath()
    {
        return fs::path(RAPIDGL_RESOURCES);
    }

    bool FileSystem::directoryExists(const fs::path& path, fs::file_status status)
    {
        if (fs::status_known(status) ? fs::exists(status) : fs::exists(path))
        {
            return true;
        }

        return false;
    }

    void FileSystem::createDirectory(const fs::path& directory_name)
    {
        fs::create_directories(directory_name);
    }
}