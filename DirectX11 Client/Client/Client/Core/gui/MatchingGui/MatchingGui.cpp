/**
 * @file MatchingGui.cpp
 * @brief Implementation of the matchmaking queue overlay.
 */

#include "MatchingGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

/**
 * @brief Renders the matchmaking status window.
 * * This function handles the modal overlay displayed while the client 
 * is waiting for a response from the matchmaking server.
 */
void MatchingGui::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImVec2 matchingWindowSize = ImVec2(400, 200);

    // Dynamic Positioning: Center the matchmaking window on the screen
    ImGui::SetNextWindowPos(ImVec2(center.x - matchingWindowSize.x * 0.5f, center.y - matchingWindowSize.y * 0.5f));
    ImGui::SetNextWindowSize(matchingWindowSize);
    ImGui::SetNextWindowFocus();

    // --- UI Styling Stack ---
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

    // Initialize the Modal Window for the matchmaking queue
    ImGui::Begin("##MatchingWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_Modal);

    // Status Header: Searching for a match
    const char* statusHeader = "Searching for Match...";
    ImGui::SetCursorPosX((matchingWindowSize.x - ImGui::CalcTextSize(statusHeader).x) * 0.5f);
    ImGui::SetCursorPosY(40);
    ImGui::Text("%s", statusHeader);

    /**
     * @section Elapsed_Time_Tracking
     * Calculates and displays the time spent in the current queue.
     * Uses std::chrono::steady_clock to ensure monotonic time progression.
     */
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - parentGui->m_matchingStartTime);
    
    std::string timeStr = std::to_string(elapsedTime.count()) + "s elapsed";
    ImGui::SetCursorPosX((matchingWindowSize.x - ImGui::CalcTextSize(timeStr.c_str()).x) * 0.5f);
    ImGui::SetCursorPosY(80);
    ImGui::Text("%s", timeStr.c_str());

    // --- Interaction Area ---
    ImVec2 cancelButtonSize = ImVec2(120, 40);
    ImGui::SetCursorPosX((matchingWindowSize.x - cancelButtonSize.x) * 0.5f);
    ImGui::SetCursorPosY(130);

    // Button Styling for high-visibility "Cancel" action
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

    // Cancel Request: Notifies the server to remove this client from the queue
    if (ImGui::Button("Cancel", cancelButtonSize)) {
        parentGui->CancleRoom(parentGui->GetUserHash());
    }

    // Pop and Restore Style Stack
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(3);
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}
