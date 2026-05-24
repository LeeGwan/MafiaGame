/**
 * @file ProcessHandler.cpp
 * @brief Implementation of global exception filtering and user feedback orchestration.
 */

#include "ProcessHandler.h"
#include <fstream>
#include "../EventManager/EventManager.h"
#include "../EventType/EventType.h"
#include "../../Network/Packet/PacketStructure/PacketStructure.h"

/** Global Singleton Instance for Process Lifecycle and Safety Management */
std::unique_ptr<ProcessHandler> G_ProcessHandler = std::make_unique<ProcessHandler>();

/**
 * @brief Constructor: Registers the top-level exception filter.
 * * This ensures that any unhandled exception in the process is captured
 * by the custom filter before the application crashes.
 */
ProcessHandler::ProcessHandler()
{
    // Register custom top-level exception handler
    SetUnhandledExceptionFilter(ProcessHandler::MyUnhandledExceptionFilter);
}

ProcessHandler::~ProcessHandler() = default;

/**
 * @brief Global Unhandled Exception Filter.
 * @param ExceptionInfo Pointers to exception records and context.
 * @return EXCEPTION_EXECUTE_HANDLER to terminate the faulting block gracefully.
 */
LONG __stdcall ProcessHandler::MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    /**
     * @todo Implement crash dump generation (Minidump) or error reporting.
     * G_core->get_C_eventmanager()->trigger(EventType::Exception_Error, true);
     */
    
    MessageBoxA(NULL, 
        "A critical process error has occurred. The application will now terminate.", 
        "System Exception", 
        MB_ICONERROR);

    return EXCEPTION_EXECUTE_HANDLER;
}

/**
 * @brief Displays server response results using a standard Win32 MessageBox.
 * @param result The ResultType received from the server.
 * * Automatically determines the appropriate icon and title based on success/failure.
 */
void ProcessHandler::MsgHandler(ResultType result)
{
    // Evaluate success criteria for UI feedback logic
    bool succeeded = (result == ResultType::SignUp_Succeeded ||
                      result == ResultType::Login_Succeeded ||
                      result == ResultType::CheckSession_Succeeded);

    std::string msg = ConversationResult(result);
    
    // UI Notification with contextual icons (Information for success, Warning for failure)
    MessageBoxA(NULL, 
        msg.c_str(), 
        succeeded ? "SUCCESS" : "FAILED",
        MB_OK | (succeeded ? MB_ICONINFORMATION : MB_ICONWARNING));
}

/**
 * @brief Maps internal ResultType enumerations to human-readable string descriptors.
 * @param result The enumeration value to translate.
 * @return A descriptive string representation of the operation result.
 */
std::string ProcessHandler::ConversationResult(ResultType result)
{
    switch (result)
    {
    case ResultType::SignUp_Failed:           return "Account registration failed.";
    case ResultType::SignUp_AlreadyExists:    return "Account already exists.";
    case ResultType::SignUp_Succeeded:         return "Registration successful.";
    case ResultType::Login_Failed:            return "Authentication failed.";
    case ResultType::Login_InvalidCredentials: return "Invalid username or password.";
    case ResultType::Login_AlreadyLoggedIn:    return "This account is already logged in.";
    case ResultType::Login_Succeeded:         return "Authentication successful.";
    case ResultType::CheckSession_Succeeded:   return "Session validated. Joining lobby...";
    case ResultType::CheckSession_Failed:      return "Session expired or invalid.";
    default:                                   return "An unknown error has occurred.";
    }
}

/**
 * @brief Provides a standard SEH filter return value.
 * @return EXCEPTION_EXECUTE_HANDLER
 */
DWORD ProcessHandler::generate_exception()
{
    return EXCEPTION_EXECUTE_HANDLER;
}
