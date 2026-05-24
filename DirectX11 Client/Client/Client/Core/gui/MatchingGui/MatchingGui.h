/**
 * @file MatchingGui.h
 * @brief Header for the matchmaking queue status interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class MatchingGui
 * @brief UI component responsible for providing feedback during the matchmaking process.
 * * This class manages the visual state of the client while it is in the queue, 
 * including real-time duration tracking and the ability to abort the request.
 */
class MatchingGui : public BaseGui
{
public:
    /** * @brief Inherits constructors from BaseGui for standardized initialization. 
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the matchmaking queue overlay using Dear ImGui.
     * * Overrides the BaseGui Render method to display:
     * - Current queue status ("Searching...").
     * - Live elapsed time using the m_matchingStartTime timestamp.
     * - Interaction logic for cancelling the matchmaking request.
     */
    void Render() override;
};
