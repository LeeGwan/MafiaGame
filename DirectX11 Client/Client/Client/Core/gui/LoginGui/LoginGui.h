/**
 * @file LoginGui.h
 * @brief Header for the user authentication (Login) interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class LoginGui
 * @brief UI component responsible for capturing user credentials and initiating authentication.
 * * This class provides the interface for:
 * - Secure input of User Identification (ID).
 * - Masked input of User Credentials (Password).
 * - Triggering the server-side authentication handshake.
 */
class LoginGui : public BaseGui
{
public:
    /** * @brief Inherits constructors from BaseGui.
     * Maintains centralized references to the UI Controller and global Core systems.
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the login interface using Dear ImGui.
     * * Overrides the BaseGui Render method to provide:
     * - Dedicated input buffers for ID and Password strings.
     * - Visual feedback for credential entry.
     * - The "Sign In" action trigger to transition to the Lobby state.
     */
    void Render() override;
};
