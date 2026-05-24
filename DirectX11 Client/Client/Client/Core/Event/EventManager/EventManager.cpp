/**
 * @file EventManager.cpp
 * @brief Implementation of the centralized event dispatching system with SEH protection.
 */

#include "EventManager.h"
#include "../ProcessHandler/ProcessHandler.h"
#include "../EventType/EventType.h"

/**
 * @brief Executes an event callback within a Structured Exception Handling (SEH) block.
 * @param type The category of the event being triggered.
 * @param fn The functional wrapper containing the callback logic.
 * * This method ensures that even if a specific event handler encounters a 
 * critical failure (e.g., access violation), the main application remains stable.
 */
void EventManager::trigger_seh(EventType type, const std::function<void()>& fn)
{
    // Windows-specific SEH to catch hardware/software exceptions at the thread level
    __try {
        /** Actual execution of the registered lambda/callback */
        fn();
    }
    /** Exception filter: Delegates crash analysis to the ProcessHandler */
    __except (G_ProcessHandler->generate_exception())
    {
        // Entry point for error logging and system recovery logic
        // Prevents the client from terminating unexpectedly
    }
}

/**
 * @brief Constructor: Pre-allocates memory for the callback map.
 * * Uses the EVENTSIZE sentinel from EventType to prevent mid-runtime 
 * dynamic memory reallocations, ensuring O(1) access time.
 */
EventManager::EventManager()
{
    callbacks.reserve(static_cast<size_t>(EventType::EVENTSIZE));
}

/** @brief Destructor: Ensures graceful cleanup of registered callback metadata. */
EventManager::~EventManager()
{
    callbacks.clear();
}

/**
 * @brief Registers a new event handler using raw pointer associations.
 * @param type The EventType to bind to.
 * @param thisPtr The 'this' pointer of the object instance (context).
 * @param func The raw function pointer (QWORD) for the callback.
 * * This low-level registration allows for efficient member function invocation 
 * across different class instances without high overhead.
 */
void EventManager::add_event(EventType type, void* thisPtr, QWORD* func)
{
    // Maps the event type to its execution metadata
    // Overwrites existing bindings to allow for dynamic UI/State transitions
    callbacks[type] = { func, thisPtr };
}
