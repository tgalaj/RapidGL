#include "gui.h"
#include "gui/imgui_impl_opengl3.h"

#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include <imgui_internal.h>
#include <sstream>

namespace RapidGL
{
    glm::vec2 GUI::m_window_size = glm::vec2(0.0f);

    GUI::~GUI()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void GUI::init(GLFWwindow * window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 460");

        m_window_size = glm::vec2(Window::getWidth(), Window::getHeight());

        ImGui::GetIO().Fonts->AddFontDefault();
    }

    void GUI::prepare()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GUI::render()
    {
        glViewport(0, 0, GLsizei(m_window_size.x), GLsizei(m_window_size.y));
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void GUI::updateWindowSize(float width, float height)
    {
        m_window_size = glm::vec2(width, height);
    }

    void GUI::beginHUD()
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
        ImGui::Begin("##Backbuffer", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
    }

    void GUI::endHUD()
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        window->DrawList->PushClipRectFullScreen();

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    float GUI::text(const std::shared_ptr<Font> & font, const std::string& text, const glm::vec2& position, float size, const glm::vec4& color, bool center, bool text_shadow)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        std::stringstream stream(text);
        std::string line;

        const auto text_color = glm::clamp(color, 0.0f, 1.0f);

        float y = 0.0f;
        int i = 0;

        while (std::getline(stream, line))
        {
            const auto text_size = font->m_font->CalcTextSizeA(size, FLT_MAX, 0.0f, line.c_str());

            if (center)
            {
                if(text_shadow)
                {
                    window->DrawList->AddText(font->m_font, size, { (position.x - text_size.x / 2.0f) + 1.0f, (position.y + text_size.y * i) + 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                    window->DrawList->AddText(font->m_font, size, { (position.x - text_size.x / 2.0f) - 1.0f, (position.y + text_size.y * i) - 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                    window->DrawList->AddText(font->m_font, size, { (position.x - text_size.x / 2.0f) + 1.0f, (position.y + text_size.y * i) - 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                    window->DrawList->AddText(font->m_font, size, { (position.x - text_size.x / 2.0f) - 1.0f, (position.y + text_size.y * i) + 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                }

                window->DrawList->AddText(font->m_font, size, { position.x - text_size.x / 2.0f, position.y + text_size.y * i }, ImGui::GetColorU32({ text_color.r, text_color.g, text_color.b, text_color.a }), line.c_str());
            }
            else
            {
                if(text_shadow)
                {
                    window->DrawList->AddText(font->m_font, size, { (position.x) + 1.0f, (position.y + text_size.y * i) + 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                    window->DrawList->AddText(font->m_font, size, { (position.x) - 1.0f, (position.y + text_size.y * i) - 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                    window->DrawList->AddText(font->m_font, size, { (position.x) + 1.0f, (position.y + text_size.y * i) - 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                    window->DrawList->AddText(font->m_font, size, { (position.x) - 1.0f, (position.y + text_size.y * i) + 1.0f }, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, text_color.a }), line.c_str());
                }

                window->DrawList->AddText(font->m_font, size, { position.x, position.y + text_size.y * i }, ImGui::GetColorU32({ text_color.r, text_color.g, text_color.b, text_color.a }), line.c_str());
            }

            y = position.y + text_size.y * (i + 1);
            i++;
        }

        return y;
    }

    void GUI::line(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, float thickness)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const auto line_color = glm::clamp(color, 0.0f, 1.0f);

        window->DrawList->AddLine({ from.x, from.y }, { to.x, to.y }, ImGui::GetColorU32({ line_color.r, line_color.g, line_color.b, line_color.a }), thickness);
    }

    void GUI::circle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness, uint32_t segments)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const auto circle_color = glm::clamp(color, 0.0f, 1.0f);

        window->DrawList->AddCircle({ position.x, position.y }, radius, ImGui::GetColorU32({ circle_color.r, circle_color.g, circle_color.b, circle_color.a }), segments, thickness);
    }

    void GUI::circleFilled(const glm::vec2& position, float radius,  const glm::vec4& color, uint32_t segments)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const auto circle_color = glm::clamp(color, 0.0f, 1.0f);

        window->DrawList->AddCircleFilled({ position.x, position.y }, radius, ImGui::GetColorU32({ circle_color.r, circle_color.g, circle_color.b, circle_color.a }), segments);
    }

    void GUI::rect(const glm::vec2& from, const glm::vec2& to,  const glm::vec4& color, float rounding, uint32_t roundingCornersFlags, float thickness)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const auto rect_color = glm::clamp(color, 0.0f, 1.0f);

        window->DrawList->AddRect({ from.x, from.y }, { to.x, to.y }, ImGui::GetColorU32({ rect_color.r, rect_color.g, rect_color.b, rect_color.a }), rounding, roundingCornersFlags, thickness);
    }

    void GUI::rectFilled(const glm::vec2& from, const glm::vec2& to,  const glm::vec4& color, float rounding, uint32_t roundingCornersFlags)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const auto rect_color = glm::clamp(color, 0.0f, 1.0f);

        window->DrawList->AddRectFilled({ from.x, from.y }, { to.x, to.y }, ImGui::GetColorU32({ rect_color.r, rect_color.g, rect_color.b, rect_color.a }), rounding, roundingCornersFlags);
    }
}
