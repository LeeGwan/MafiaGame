/**
 * @file LobbyGui.h
 * @brief Header for the game lobby and matchmaking hub interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class LobbyGui
 * @brief UI component responsible for player identity management and matchmaking entry.
 * * This class serves as the primary hub where authenticated users can:
 * - Configure their session-specific display name (Nickname).
 * - Initiate matchmaking requests to the lobby server.
 * - Securely terminate the session through the logout sequence.
 */
class LobbyGui : public BaseGui
{
public:
    /** * @brief Inherits constructors from BaseGui.
     * Synchronizes access to the global UI controller and core systems.
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the lobby interface using Dear ImGui.
     * * Overrides the BaseGui Render method to provide:
     * - A nickname input buffer for session identification.
     * - Matchmaking triggers ("Find Match").
     * - Session invalidation triggers ("Logout").
     */
    void Render() override;
};
