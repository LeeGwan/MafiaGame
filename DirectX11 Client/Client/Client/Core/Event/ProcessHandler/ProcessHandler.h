/**
 * @file ProcessHandler.h
 * @brief Header for global exception filtering and user-facing message orchestration.
 */

#pragma once
#include <Windows.h>
#include <memory>
#include <string>

// Forward declaration for optimized compilation
enum class ResultType : uint8_t;

/**
 * @class ProcessHandler
 * @brief Manages the process lifecycle safety and standardizes user feedback.
 * * This class implements a top-level Unhandled Exception Filter to capture 
 * critical crashes and provides a mapping service to translate server-side 
 * ResultType enums into human-readable notifications.
 */
class ProcessHandler
{
public:
    /**
     * @brief Constructor: Registers the custom Unhandled Exception Filter.
     * * Automatically hooks into the Windows Error Reporting pipeline upon instantiation.
     */
    ProcessHandler();
    ~ProcessHandler();

    /**
     * @brief Global callback for Structured Exception Handling (SEH).
     * @param ExceptionInfo Pointers to the exception record and processor context.
     * @return LONG Execution status (e.g., EXCEPTION_EXECUTE_HANDLER).
     * @note Must be static to satisfy the Win32 API callback signature.
     */
    static LONG __stdcall MyUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

    /**
     * @brief Dispatches a contextual UI notification based on server results.
     * @param result The ResultType enumeration received from the network layer.
     * * This method handles icon selection (Info/Warning) and title generation.
     */
    void MsgHandler(ResultType result);

    /**
     * @brief Provides a standard filter expression for __except blocks.
     * @return EXCEPTION_EXECUTE_HANDLER constant.
     * @details Used primarily by EventManager::trigger_seh for localized protection.
     */
    DWORD generate_exception();

private:
    /**
     * @brief Internal dictionary to convert enum values to descriptive strings.
     * @param result The ResultType to translate.
     * @return A user-friendly std::string.
     */
    static std::string ConversationResult(ResultType result);
};

/** Global access point for the ProcessHandler singleton. */
extern std::unique_ptr<ProcessHandler> G_ProcessHandler;
