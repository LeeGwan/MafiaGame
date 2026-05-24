/**
 * @file RegisterGui.h
 * @brief Header for the account registration and user onboarding interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class RegisterGui
 * @brief UI component responsible for managing the user account creation workflow.
 * * This class facilitates the secure collection of new user credentials, 
 * including redundant password entry for input verification, before 
 * dispatching the data to the authentication backend.
 */
class RegisterGui : public BaseGui
{
public:
    /** * @brief Inherits constructors from BaseGui.
     * Ensures the component has direct access to the global UI controller 
     * and the underlying framework's core systems.
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the registration form using the Dear ImGui framework.
     * * Overrides the BaseGui Render method to provide:
     * - Input buffers for User ID and primary Password.
     * - A secondary verification buffer for Password confirmation.
     * - Action triggers for the server-side "Sign Up" procedure.
     */
    void Render() override;
};
