/**
 * @file ControlGui.cpp
 * @brief Implementation of the Window Control and Navigation GUI.
 */

#include "ControlGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

/**
 * @brief Renders window management buttons and navigation controls.
 * Handles Minimize, Close (WM_CLOSE), and UI state transitions.
 */
void ControlGui::Render()
{
    ImGuiIO& io = ImGui::GetIO();

    // --- Window Control Buttons (Top-Right) ---
    ImVec2 controlButtonPos = ImVec2(io.DisplaySize.x - 80, 10);
    ImGui::SetNextWindowPos(controlButtonPos);
    ImGui::SetNextWindowSize(ImVec2(70, 35));

    // Create a transparent overlay for management buttons
    ImGui::Begin("##ControlButtons", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    // Apply custom styling for control buttons
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 0.9f));

    // Minimize Button Handler
    if (ImGui::Button("_", ImVec2(25, 25))) {
        if (parentGui->m_mainWindow) {
            ShowWindow(parentGui->m_mainWindow, SW_MINIMIZE);
        }
    }

    ImGui::SameLine();

    // Close Button Handler: Triggers logout sequence and sends WM_CLOSE message
    if (ImGui::Button("X", ImVec2(25, 25))) {
        if (parentGui->m_mainWindow) {
            // Trigger logout event if currently authenticated
            if (parentGui->IsLogin()) {
                // G_core->get_C_eventmanager()->trigger(EventType::LOGOUT_EVENT, false, parentGui->GetUserHash());
            }
            PostMessage(parentGui->m_mainWindow, WM_CLOSE, 0, 0);
        }
    }

    ImGui::PopStyleColor(2);
    ImGui::End();

    // --- Navigation Controls (Bottom-Left) ---
    // The 'Back' button is disabled on Init and Lobby screens to prevent invalid state transitions.
    if (parentGui->currentUitype.load() != EUIType::Init &&
        parentGui->currentUitype.load() != EUIType::Lobby) {

        ImVec2 backButtonPos = ImVec2(10, io.DisplaySize.y - 50);
        ImGui::SetNextWindowPos(backButtonPos);
        ImGui::SetNextWindowSize(ImVec2(90, 40));

        ImGui::Begin("##BackButton", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        // Return to the Initial screen
        if (ImGui::Button("Back", ImVec2(80, 30))) {
            parentGui->SetUitype(EUIType::Init);
        }

        ImGui::End();
    }
}
