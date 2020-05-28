#pragma once

#include <string>

namespace RapidGL
{
    class FileSystem
    {
    public:
        static std::string getPath(const std::string& path);

    private:
        static const std::string& getRoot();
        static std::string getPathRelativeRoot(const std::string& path);
    };
}