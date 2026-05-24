/**
 * @file BaseGui.h
 * @brief Definition of the abstract base class for all GUI components.
 */

#pragma once
#include "../../../Dependencies/Imgui/imgui.h"

class GuiControl;

/**
 * @class BaseGui
 * @brief An abstract interface that defines the lifecycle and shared resources for GUI screens.
 * * This class ensures that every derived GUI component implements a Render() routine 
 * and maintains a reference to the central GuiControl for state transitions and logic execution.
 */
class BaseGui
{
public:
    /**
     * @brief Constructor that establishes the link between the UI component and its controller.
     * @param parent Pointer to the GuiControl instance managing this GUI.
     */
    explicit BaseGui(GuiControl* parent) : parentGui(parent) {}

    /** @brief Virtual destructor to ensure proper cleanup of derived GUI instances. */
    virtual ~BaseGui() = default;

    /**
     * @brief Pure virtual function for rendering the UI.
     * * Must be implemented by all derived classes (e.g., LoginGui, LobbyGui) 
     * to define their specific ImGui drawing logic.
     */
    virtual void Render() = 0;

protected:
    /** Reference to the UI Controller used for triggering state changes (e.g., SetUitype) or network events. */
    GuiControl* parentGui;

    /** Standardized layout constants for maintaining visual consistency across different panels. */
    const ImVec2 panelSize = ImVec2(350, 280);      // Standard panel dimensions
    const ImVec2 InitpanelSize = ImVec2(1020, 680); // Large-scale landing panel dimensions
};
