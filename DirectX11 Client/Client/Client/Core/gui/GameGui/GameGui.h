/**
 * @file GameGui.h
 * @brief Header for the active game session interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class GameGui
 * @brief UI component responsible for rendering the active gameplay state and session statistics.
 * * This class handles the visual representation of the game while it is in progress, 
 * focusing on real-time data such as session duration and connection status.
 */
class GameGui : public BaseGui
{
public:
    /** * @brief Inherits constructors from BaseGui for consistent initialization. 
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the in-game HUD and session status window.
     * * Overrides the BaseGui Render method to display:
     * - Active session indicators.
     * - Real-time elapsed time (Session duration).
     * - Contextual action buttons (e.g., Exit/Disconnect).
     */
    void Render() override;
};
