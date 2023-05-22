#pragma once
#include <imgui.h>
#include <filesystem>
#include <string>

namespace RGL
{
    class Font
    {
    public:
        Font();
        Font(const std::filesystem::path & filepath, unsigned size_pixels);
        ~Font() = default;

    private:
        friend class GUI;

        ImFont * m_font;
    };
}
