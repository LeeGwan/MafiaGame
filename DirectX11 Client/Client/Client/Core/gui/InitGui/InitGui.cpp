/**
 * @file InitGui.cpp
 * @brief Implementation of the application entry point UI.
 */

#include "InitGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

/**
 * @brief Renders the initial landing screen for user path selection.
 * Handles navigation to either the Login or Registration interfaces.
 */
void InitGui::Render()
{
    // --- Panel Initialization ---
    // Create a fixed, non-collapsible panel for the landing interface
    ImGui::Begin("##InitPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // --- Layout Constants ---
    // Define standardized button dimensions and spacing for visual consistency
    ImVec2 buttonSize = ImVec2(290, 130);
    float spacing = 40.0f;
    float totalWidth = (buttonSize.x * 2) + spacing;

    // --- Dynamic Positioning ---
    // Calculate and set the centered horizontal and vertical cursor position
    ImGui::SetCursorPosX((1020 - totalWidth) * 0.5f);
    ImGui::SetCursorPosY(340);

    // --- User Path Selection ---

    // Login Button: Triggers state transition to the Authentication screen
    if (ImGui::Button("Login", buttonSize)) {
        parentGui->SetUitype(EUIType::Login);
    }

    ImGui::SameLine(0, spacing);

    // Register Button: Triggers state transition to the Account Creation screen
    if (ImGui::Button("Sign Up", buttonSize)) {
        parentGui->SetUitype(EUIType::Register);
    }

    ImGui::End();
}
