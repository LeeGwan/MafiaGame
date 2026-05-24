/**
 * @file ControlGui.h
 * @brief Header for the window management and navigation control interface.
 */

#pragma once
#include "../basegui/BaseGui.h"

/**
 * @class ControlGui
 * @brief Specialized UI component for handling platform-level window actions and state navigation.
 * * This class encapsulates the logic for standard window operations (Minimize, Close) 
 * and user-flow navigation (Back button), acting as a top-level overlay.
 */
class ControlGui : public BaseGui
{
public:
    /** * Inherit constructors from BaseGui to maintain consistent initialization 
     * of parentGui and core references.
     */
    using BaseGui::BaseGui;

    /**
     * @brief Renders the control interface using the Dear ImGui framework.
     * * Overrides the BaseGui Render method to draw:
     * - Top-right window management cluster (Minimize/Close).
     * - Bottom-left contextual navigation (Back button).
     */
    void Render() override;
};
