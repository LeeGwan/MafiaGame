/**
 * @file RegisterGui.cpp
 * @brief Implementation of the Account Registration interface.
 */

#include "RegisterGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

/**
 * @brief Renders the registration form for new account creation.
 * Facilitates the collection of user credentials and initiates the sign-up handshake.
 */
void RegisterGui::Render()
{
    // --- Panel Configuration ---
    // Initialize a fixed, non-interactive background panel for the registration form
    ImGui::Begin("##RegisterPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Adjust vertical layout for centralized positioning
    ImGui::SetCursorPosY(300);

    /** * @section Identification_Field
     * Captures the desired username/ID for the new account.
     */
    ImGui::Text("Username:");
    ImGui::InputText("##ID", parentGui->TempId, ID_PW_SIZE);

    /** * @section Primary_Credential_Field
     * Captures the primary password. 
     * Uses ImGuiInputTextFlags_Password to prevent visual credential exposure.
     */
    ImGui::Text("Password:");
    ImGui::InputText("##Password", parentGui->TempPw, ID_PW_SIZE, ImGuiInputTextFlags_Password);

    /** * @section Credential_Verification_Field
     * Captures a secondary password entry to verify input integrity.
     */
    ImGui::Text("Confirm Password:");
    ImGui::InputText("##CheckPassword", parentGui->TempCheckPw, ID_PW_SIZE, ImGuiInputTextFlags_Password);

    ImGui::Spacing();

    // --- Registration Execution ---
    // Triggers the SignUp logic via the UI controller.
    // Note: Cross-field validation (e.g., matching passwords) is handled within the SignUp method.
    if (ImGui::Button("Sign Up", ImVec2(290, 40))) {
        parentGui->SignUp(parentGui->TempId, parentGui->TempPw, parentGui->TempCheckPw);
    }

    ImGui::End();
}
