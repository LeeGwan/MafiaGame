/**
 * @file window.h
 * @brief Header for the Win32 window wrapper and message loop orchestration.
 */

#pragma once
#include <memory>
#include <atomic>
#include <string>
#include "../../../Dependencies/Imgui/imgui_impl_win32.h"
#include "../../../Dependencies/Imgui/imgui.h"

class GuiControl;

/**
 * @class window
 * @brief Manages the lifecycle of the host Win32 window and the high-performance message loop.
 * * This class encapsulates the boilerplate code for window creation, registration, 
 * and the polling mechanism required for real-time ImGui rendering.
 */
class window
{
public:
    window();
    ~window();

    /**
     * @brief Static Window Procedure (WndProc) callback to handle Win32 system messages.
     * * Processes mouse/keyboard input for ImGui and implements custom dragging logic 
     * for frameless window styles (WS_POPUP).
     */
    static LRESULT WINAPI SWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief The primary execution loop for the GUI thread.
     * * Handles message dispatching and orchestrates the rendering calls to GuiControl 
     * in a non-blocking manner to maintain a high frame rate.
     * @param hInstance Handle to the current application instance.
     */
    void Update(HINSTANCE hInstance);

    /** @brief Safely terminates the message loop and triggers resource cleanup in GuiControl. */
    void Release();

private:
    /**
     * @brief Internal bootstrapping function for window class registration and creation.
     * @param hInstance Handle to the current application instance.
     * @return True if the Win32 window and ImGui backend were successfully initialized.
     */
    bool init(HINSTANCE hInstance);

private:
    /** Thread-safe flag to control the execution state of the message loop. */
    std::atomic<bool> done;

    /** Internal layout and coordinate tracking (Viewport size). */
    ImVec2 Size;

    /** The low-level Win32 window handle. */
    HWND Hwnd;

    /** Target dimensions for the client application window. */
    ImVec2 WindowSize;

    /** Extended window class structure for OS registration. */
    WNDCLASSEXW Wc;
};
