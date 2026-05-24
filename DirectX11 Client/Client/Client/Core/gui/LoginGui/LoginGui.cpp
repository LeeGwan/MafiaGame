/**
 * @file LoginGui.cpp
 * @brief Implementation of the User Authentication (Login) interface.
 */

#include "LoginGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

/**
 * @brief Renders the login interface for user credential input.
 * Facilitates the secure transmission of ID and Password to the authentication controller.
 */
void LoginGui::Render()
{
    // --- Panel Configuration ---
    ImGui::Begin("##LoginPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Adjust vertical alignment for a centered aesthetic
    ImGui::SetCursorPosY(300);

    /** * @section Identification_Input 
     * Captures the unique user identifier.
     */
    ImGui::Text("Username:");
    ImGui::InputText("##ID", parentGui->TempId, ID_PW_SIZE);

    /** * @section Credential_Security 
     * Secure input field for passwords. 
     * Utilizes ImGuiInputTextFlags_Password to mask characters for data privacy.
     */
    ImGui::Text("Password:");
    ImGui::InputText("##Password", parentGui->TempPw, ID_PW_SIZE, ImGuiInputTextFlags_Password);

    ImGui::Spacing();

    // --- Authentication Trigger ---
    // Initiates the SignIn handshake via the UI controller
    if (ImGui::Button("Sign In", ImVec2(290, 40))) {
        parentGui->SignIn(parentGui->TempId, parentGui->TempPw);
    }

    ImGui::End();
}
