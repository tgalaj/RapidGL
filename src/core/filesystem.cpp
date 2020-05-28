#include "filesystem.h"
#include "root_directory.h"

namespace RapidGL
{
    std::string FileSystem::getPath(const std::string& path)
    {
        return getPathRelativeRoot(path);
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