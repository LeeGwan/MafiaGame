/**
 * @file main.cpp
 * @brief Primary entry point for the Windows application.
 * @details Orchestrates the global initialization, main execution loop, 
 * and memory leak detection during the development lifecycle.
 */

#include <crtdbg.h>
#include "Core/Core/Core.h"
#include <Windows.h>

/**
 * @brief Windows Application Entry Point (wWinMain).
 * @param hInstance Handle to the current instance of the application.
 * @param hPrevInstance Reserved (always NULL in modern Windows).
 * @param lpCmdLine Command line arguments as a Unicode string.
 * @param nCmdShow Control flag for how the window is to be shown.
 * @return Termination status code.
 */
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    /**
     * @section Memory_Leak_Detection
     * Enables automatic memory leak reporting to the 'Output' window upon exit.
     * Only active in _DEBUG builds to prevent performance overhead in Release.
     */
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    /**
     * @section System_Lifecycle
     * 1. Init(): Registers events, initializes WinSock, and connects to the Routine server.
     * 2. Update(): Launches the UI thread and begins the message pump.
     */
    if (G_core->Init())
    {
        G_core->Update(hInstance);
    }

    /**
     * @note 
     * The process reaches this point only after the GUI thread (C_window) 
     * is closed, signaling a graceful shutdown.
     */
    return 0;
}
