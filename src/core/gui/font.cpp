#include "font.h"
#include "filesystem.h"

namespace RGL
{
    Font::Font()
    {
        m_font = ImGui::GetIO().Fonts->AddFontDefault();
    }

    Font::Font(const std::string& filepathname, unsigned size_pixels)
    {
        m_font = ImGui::GetIO().Fonts->AddFontFromFileTTF(FileSystem::getPath(filepathname).c_str(), size_pixels);
    }
}
