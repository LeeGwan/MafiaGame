/**
 * @file Core.h
 * @brief Header for the central orchestrator that manages system lifecycle and module integration.
 */

#pragma once

#include <memory>
#include <thread>

/** @brief 64-bit function pointer representation for low-level memory mapping. */
using QWORD = unsigned long long; 

// --- Forward Declarations for Optimized Compilation ---
enum class EventType : uint8_t;   
class NetWork;                   
class window;                    
class EventManager;               
class RoutineProgress;            
class AntiCheat;                  

/** @brief Opaque handle to an instance of a Windows application. */
using HINSTANCE = struct HINSTANCE__*;  

/**
 * @class Core
 * @brief The central backbone of the application responsible for bootstrapping and cleanup.
 * * This class implements the Facade pattern, providing a unified interface to initialize 
 * Network, GUI, and Anti-Cheat modules. It also manages the primary Event Bus.
 */
class Core
{
public:
    /** @brief Constructs the core and prepares baseline communication managers. */
    Core();
    ~Core();

    /** @brief Provides access to the centralized Event Dispatcher. */
    EventManager* get_C_eventmanager();

    /**
     * @brief Bootstraps the entire client infrastructure.
     * @return True if event registration and network discovery are successful.
     */
    bool Init();

    /**
     * @brief Launches the primary GUI thread and blocks until termination.
     * @param hInstance Handle to the current application instance.
     */
    void Update(HINSTANCE hInstance);

    /** @brief Performs graceful shutdown and releases all allocated system resources. */
    void Release();

private:
    /**
     * @brief Maps all inter-module signals to their respective handler functions.
     * @return True if all mandatory events are bound correctly.
     */
    bool Init_EventManager();

    /**
     * @brief Initializes WinSock and establishes initial contact with the discovery server.
     * @return True if the network layer is ready for communication.
     */
    bool Nework_Init();

    /**
     * @brief Generic template for binding EventTypes to class member functions.
     * @tparam Func Signature of the member function pointer.
     * @param type The EventType to listen for.
     * @param memberFunc The actual function logic to execute.
     */
    template<typename Func>
    bool RegisterEvent(EventType type, Func memberFunc);

private:
    /** @brief Central message bus for decoupled communication. */
    std::unique_ptr<EventManager> C_eventmanager;  

    /** @brief Primary Win32 window management instance. */
    std::unique_ptr<window> C_window;              

    /** @brief Dedicated thread for the GUI message pump to ensure logic responsiveness. */
    std::thread GuiThread;                         
};

/** @brief Global singleton instance for system-wide access. */
extern std::unique_ptr<Core> G_core;
