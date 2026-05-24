/**
 * @file GameGui.cpp
 * @brief Implementation of the active in-game session interface.
 */

#include "GameGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"
#include <string>
#include <chrono>

/**
 * @brief Renders the HUD and status window during an active game session.
 * Displays real-time session duration and game state information.
 */
void GameGui::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImVec2 matchingWindowSize = ImVec2(400, 200);

    // Dynamic window positioning: Center the game session window on the display
    ImGui::SetNextWindowPos(ImVec2(center.x - matchingWindowSize.x * 0.5f, center.y - matchingWindowSize.y * 0.5f));
    ImGui::SetNextWindowSize(matchingWindowSize);
    ImGui::SetNextWindowFocus();

    // --- UI Styling Stack ---
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

    // Begin Rendering the Modal Game Window
    ImGui::Begin("##GameWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_Modal);

    // Render "In Game..." Status Header
    const char* statusHeader = "In Session...";
    ImGui::SetCursorPosX((matchingWindowSize.x - ImGui::CalcTextSize(statusHeader).x) * 0.5f);
    ImGui::SetCursorPosY(40);
    ImGui::Text("%s", statusHeader);

    /**
     * @section Time_Tracking
     * Calculates the duration elapsed since the game instance was initialized.
     * Uses std::chrono for high-precision time measurement.
     */
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - parentGui->m_GameStartTime);
    
    std::string timeStr = "Elapsed: " + std::to_string(elapsedTime.count()) + "s";
    ImGui::SetCursorPosX((matchingWindowSize.x - ImGui::CalcTextSize(timeStr.c_str()).x) * 0.5f);
    ImGui::SetCursorPosY(80);
    ImGui::Text("%s", timeStr.c_str());

    // --- Interaction Area ---
    ImVec2 cancelButtonSize = ImVec2(120, 40);
    ImGui::SetCursorPosX((matchingWindowSize.x - cancelButtonSize.x) * 0.5f);
    ImGui::SetCursorPosY(130);

    // Styles for action buttons (Exit/Cancel)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

    /** * Note: Insert button interaction logic here (e.g., Leave Match).
     * Style cleanup is performed below to maintain stack integrity.
     */

    // Pop and Restore Style Stack
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(3);
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}
