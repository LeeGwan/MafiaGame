/**
 * @file InitGui.h
 * @brief Header for the initial landing screen interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class InitGui
 * @brief UI component for the application's entry point.
 * * This class serves as the primary landing interface where users can choose
 * between the Authentication (Login) and Account Creation (Sign Up) pathways.
 */
class InitGui : public BaseGui
{
public:
    /** * @brief Inherits constructors from BaseGui.
     * Ensures centralized access to the UI manager (parentGui) and global core.
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the landing screen interface using Dear ImGui.
     * * Overrides the BaseGui Render method to provide:
     * - A central navigation hub for users.
     * - State transition triggers to Login or Register screens.
     */
    void Render() override;
};
