/**
 * @file EventManager.h
 * @brief Header for the centralized event-driven communication system.
 */

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>

// Forward declaration of EventType
enum class EventType : uint8_t;

/** @brief 64-bit function pointer representation for low-level memory mapping. */
using QWORD = unsigned long long;

/**
 * @class EventManager
 * @brief Implements an advanced Observer pattern for inter-module communication.
 * * This manager allows modules to trigger and respond to events without direct 
 * dependencies. It supports both synchronous and asynchronous execution 
 * with hardware-level exception protection (SEH).
 */
class EventManager
{
private:
    /**
     * @struct callback
     * @brief Metadata for registered event handlers.
     */
    struct callback
    {
        QWORD* funcPtr;  ///< Pointer to the member function logic.
        void* thisPtr;   ///< Instance context (the 'this' pointer) for the call.
    };

    /** Hash map for O(1) lookup of event handlers. */
    std::unordered_map<EventType, callback> callbacks;

private:
    /**
     * @brief Internal wrapper to execute events within a Protected Exception block.
     * @param type The EventType category.
     * @param fn The wrapped execution logic.
     */
    void trigger_seh(EventType type, const std::function<void()>& fn);

public:
    EventManager();
    ~EventManager();

    /**
     * @brief Registers a class member function as an event listener.
     * @param type The event category to subscribe to.
     * @param thisPtr Pointer to the instance owning the member function.
     * @param func Raw address of the member function.
     */
    void add_event(EventType type, void* thisPtr, QWORD* func);

    /**
     * @brief Dispatches an event to the registered listener.
     * @tparam Args Variadic template arguments to pass to the callback.
     * @param type The event category to trigger.
     * @param wait If true, joins the thread (Sync); if false, detaches (Async).
     * @param args The actual data arguments.
     */
    template <typename... Args>
    void trigger(EventType type, bool wait, const Args&... args)
    {
        // O(1) Search for the registered handler
        auto it = callbacks.find(type);
        if (it != callbacks.end())
        {
            auto& cb = it->second;

            /**
             * Dedicated execution thread to prevent blocking the caller.
             * Captures callback metadata and variadic arguments.
             */
            std::thread triggerthread([this, cb, type, args...]
                {
                    // Execute within SEH protection gate
                    trigger_seh(type, [cb, &args...]()
                        {
                            /**
                             * @section Low_Level_Invocation
                             * Reinterprets the raw pointer as a __thiscall member function.
                             * Format: void(Instance* this, Arguments...)
                             */
                            using callFunc = void(__thiscall*)(void*, Args...);
                            auto func = reinterpret_cast<callFunc>(cb.funcPtr);

                            // Execute member function with provided context
                            func(cb.thisPtr, args...);
                        });
                });

            // Synchronicity management
            if (wait)
            {
                triggerthread.join();  // Wait for completion (Blocking)
            }
            else
            {
                triggerthread.detach(); // Fire and forget (Non-blocking)
            }
        }
    }
};
