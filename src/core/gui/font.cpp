#include "font.h"
#include "filesystem.h"

namespace RGL
{
    Font::Font()
    {
        m_font = ImGui::GetIO().Fonts->AddFontDefault();
    }

    Font::Font(const std::filesystem::path& filepath, unsigned size_pixels)
    {
        auto path = FileSystem::getRootPath() / filepath;
        m_font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), size_pixels);
    }
}
