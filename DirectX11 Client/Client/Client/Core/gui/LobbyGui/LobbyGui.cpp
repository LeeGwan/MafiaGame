/**
 * @file LobbyGui.cpp
 * @brief Implementation of the Game Lobby and Matchmaking interface.
 */

#include "LobbyGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

/**
 * @brief Renders the lobby panel for user identity management and matchmaking.
 * Allows users to update their display name and initiate server-side room requests.
 */
void LobbyGui::Render()
{
    // --- Panel Initialization ---
    ImGui::Begin("##LobbyPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    /**
     * @section Identity_Management
     * Allows the user to modify their display name before entering a match.
     * The input is buffered in TempNickName for persistence during the session.
     */
    ImGui::Text("Player Nickname:");
    ImGui::InputText("##NICKNAME", parentGui->TempNickName, ID_PW_SIZE);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Matchmaking Actions ---

    // Find Match Button: Initiates the JoinRoom handshake with the Lobby Server
    if (ImGui::Button("Find Match", ImVec2(200, 60))) {
        parentGui->JoinRoom(parentGui->GetUserHash(), parentGui->TempNickName);
    }

    ImGui::Spacing();

    /**
     * @section Session_Termination
     * Handles secure logout by invalidating the current session hash on the server.
     */
    if (ImGui::Button("Logout", ImVec2(200, 60))) {
        if (parentGui->IsLogin()) {
            parentGui->LogOut(parentGui->GetUserHash());
        }
    }

    ImGui::End();
}
