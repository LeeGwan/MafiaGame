/**
 * @file ProcessManager.cpp
 * @brief Implementation of the game process orchestration and security integration.
 */

#include "ProcessManager.h"
#include "../Utils/UtilsForString.h"
#include "../gui/guicontrol/GuiControl.h"
#include "../AntiCheat/AntiCheat.h"
#include <windows.h>
#include <fstream>
#include <filesystem>

/** Global Singleton Instance for Process Lifecycle Management */
std::unique_ptr<ProcessManager> G_ProcessManager = std::make_unique<ProcessManager>();

/**
 * @brief Launches the Unreal Engine game client with secure credential injection.
 * @param IP Target server IP address.
 * @param port Target server port number.
 * * This function constructs a command-line string containing the session hash,
 * user nickname, and network endpoints, then spawns the process in a protected state.
 */
void ProcessManager::LaunchGameClient(const std::string& IP, uint16_t port)
{
    // --- Path and Command-Line Construction ---
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::wstring GameEXE_path = currentPath.wstring() + L"\\MafiaGame\\MafiaGameClient.exe";
    
    // Configure Unreal Engine execution flags: Windowed mode at 480p resolution
    std::wstring GameEXE_BaseArgs = GameEXE_path + L" -game -windowed -ResX=640 -ResY=480";
    
    /**
     * @section Parameter_Injection
     * Unreal Engine uses a URL-style parameter format for command-line arguments.
     * Format: Executable.exe [Map]?Param1=Value1?Param2=Value2
     */
    std::wstring GameEXE_Full = GameEXE_BaseArgs + L" Hash=" + UtilsForString::UTF8ToWString(G_GuiControl->GetUserHash(), CP_ACP)
        + L"?NickName=" + UtilsForString::UTF8ToWString(G_GuiControl->TempNickName, CP_ACP)
        + L"?IP=" + UtilsForString::UTF8ToWString(IP, CP_ACP)
        + L":" + std::to_wstring(port);

    // Prepare buffer for CreateProcessW (requires a non-const character pointer)
    std::vector<wchar_t> commandLine(GameEXE_Full.begin(), GameEXE_Full.end());
    commandLine.push_back(L'\0');

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    // --- Process Creation ---
    if (!CreateProcessW(NULL, commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        MessageBoxA(NULL, "Failed to launch the game client.", "Process Error", MB_ICONERROR);
        return;
    }

    /**
     * @section Security_Integration
     * Immediately register the newly created PID to the Anti-Cheat module.
     * This ensures the process is monitored/protected from the moment of inception.
     */
    G_AntiCheat->AddProtectedPID(pi.dwProcessId);

    // Wait for the process to terminate before continuing launcher execution
    WaitForSingleObject(pi.hProcess, INFINITE);

    // --- Post-Process Analysis ---
    DWORD exitCode;
    if (GetExitCodeProcess(pi.hProcess, &exitCode))
    {
        // Handle specific exit codes (e.g., crash, intentional close, or anti-cheat kick)
        switch (exitCode)
        {
        case 0:           /* Normal Exit */ break;
        case 1:           /* General Error */ break;
        case STILL_ACTIVE: /* Unexpected State */ break;
        default:          /* Log specific crash/error codes */ break;
        }
    }

    // Securely close handles to prevent resource leaks
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
