/**
 * @file Core.cpp
 * @brief Implementation of the central system orchestrator and module bootstrap.
 * @details This class manages the lifecycle of all core modules including Network, GUI, 
 * and Anti-Cheat, using a centralized EventManager for decoupled communication.
 */

#include "Core.h"
#include "../gui/window/window.h"
#include "../Network/Network/NetWork.h"
#include "../gui/guicontrol/GuiControl.h"
#include "../Network/Packet/PacketStructure/PacketStructure.h"
#include "../ProcessManager/ProcessManager.h"
#include "../Network/Packet/RoutineProgress/RoutineProgress.h"
#include "../Event/ProcessHandler/ProcessHandler.h"
#include "../Event/EventManager/EventManager.h"
#include "../Event/EventType/EventType.h"
#include "../AntiCheat/AntiCheat.h"
#include <functional>
#include <Windows.h>

/** Global Singleton Instance for Central Orchestration */
std::unique_ptr<Core> G_core = std::make_unique<Core>();

/**
 * @brief Constructor: Initializes the primary backbone components.
 * Creates the EventManager for message routing and the primary Window instance.
 */
Core::Core()
{
    C_eventmanager = std::make_unique<EventManager>();
    C_window = std::make_unique<window>();
}

Core::~Core() = default;

/** @brief Returns the pointer to the centralized EventManager. */
EventManager* Core::get_C_eventmanager()
{
    return C_eventmanager.get();
}

/**
 * @brief Bootstraps the entire client infrastructure.
 * @return True if event registration and initial network discovery succeed.
 */
bool Core::Init()
{
    // 1. Register all inter-module event listeners
    if (!Init_EventManager()) return false;

    // 2. Setup WinSock and initiate discovery via the Routine Server
    if (!Nework_Init()) return false;

    return true;
}

/**
 * @brief Launches the GUI rendering loop on a dedicated thread.
 * @param hInstance Windows application instance handle.
 */
void Core::Update(HINSTANCE hInstance)
{
    /** * Decouples the UI update loop from the main logic thread 
     * to ensure background security tasks remain responsive.
     */
    GuiThread = std::thread(std::bind(&window::Update, C_window.get(), hInstance));

    // Wait for the GUI thread to terminate before process exit
    if (GuiThread.joinable()) {
        GuiThread.join();
    }
}

/**
 * @brief Configures the global Event-to-Module mapping.
 * Binds specific EventTypes to their respective singleton instance member functions.
 */
bool Core::Init_EventManager()
{
    if (!C_eventmanager || !G_network || !C_window) return false;

    // --- System & UI Message Bindings ---
    if (!RegisterEvent(EventType::MESSAGE_EVENT, &ProcessHandler::MsgHandler)) return false;

    // --- Network Lifecycle Bindings ---
    if (!RegisterEvent(EventType::SUCESS_ROUTINEAUTH, &NetWork::ConnectToAuthServer)) return false;  
    if (!RegisterEvent(EventType::SUCESS_GAMELOBBY, &NetWork::ConnectToGameLobbyServer)) return false; 

    // --- Outbound Packet Dispatch Bindings ---
    if (!RegisterEvent(EventType::TwoStringPacket_EVNET, &RoutineProgress::SendResponseForTwoStringPacket)) return false;  
    if (!RegisterEvent(EventType::HashPacket_EVNET, &RoutineProgress::SendResponseForHashPacket)) return false;  

    // --- GUI & UX State Bindings ---
    if (!RegisterEvent(EventType::CHANGE_UI_TYPE, &GuiControl::SetUitype)) return false;

    // --- Process & Game Control Bindings ---
    if (!RegisterEvent(EventType::TERMINATE_PROCESSEVENT, &Core::Release)) return false; 
    if (!RegisterEvent(EventType::STARTGAME_EVENT, &ProcessManager::ProcessRunner)) return false;  

    // --- Security & Anti-Cheat Integration ---
    if (G_AntiCheat)
    {
        if (!RegisterEvent(EventType::SECURITY_Init_EVENT, &AntiCheat::Start)) return false;  
        if (!RegisterEvent(EventType::HWID_DATA_EVENT, &AntiCheat::RequestHardwareInfo)) return false;  
        if (!RegisterEvent(EventType::SECURITY_Heartbeat_EVENT, &AntiCheat::ServerCheckLogic)) return false;  
    }
    return true;
}

/**
 * @brief Initializes network resources and attempts connection to the Routing server.
 */
bool Core::Nework_Init()
{
    if (!G_network->Initialize()) return false;

    // Connect to the Routine server to obtain the primary Auth server endpoint
    if (!G_network->ConnectToRoutinAuthServer())
    {
        G_network->CleanUp();
        return false;
    }
    return true;
}

/** @brief Gracefully releases all networking and system resources. */
void Core::Release()
{
    G_network->CleanUp();
}

/**
 * @brief Bridges EventTypes to class member functions using raw pointers.
 * @tparam Func Signature of the member function.
 * @param type The EventType to register.
 * @param memberFunc The member function address.
 * @return True if the binding was successful.
 */
template<typename Func>
bool Core::RegisterEvent(EventType type, Func memberFunc)
{
    // Cast the member function pointer to a 64-bit address for storage
    QWORD funcAddr = *reinterpret_cast<QWORD*>(&memberFunc);
    void* thisptr = nullptr;

    if (!funcAddr) return false;

    /**
     * @section Dispatch_Logic
     * Maps the event category to the appropriate global singleton instance.
     */
    if ((type >= EventType::PRIORITY_PACKET && type <= EventType::SUCESS_GAMELOBBY) ||
        (type >= EventType::TypePacketREQUEST_EVNET && type <= EventType::CANCLEROOM_EVENT))
    {
        thisptr = G_network.get();
    }
    else if (type == EventType::MESSAGE_EVENT || type == EventType::Exception_Error)
    {
        thisptr = G_ProcessHandler.get();
    }
    else if (type == EventType::CHANGE_UI_TYPE)
    {
        thisptr = G_GuiControl.get();
    }
    else if (type == EventType::TERMINATE_PROCESSEVENT)
    {
        thisptr = G_core.get();
    }
    else if (type == EventType::SECURITY_Init_EVENT ||
             type == EventType::HWID_DATA_EVENT ||
             type == EventType::SECURITY_Heartbeat_EVENT)
    {
        thisptr = G_AntiCheat.get();
    }
    else if (type == EventType::STARTGAME_EVENT)
    {
        thisptr = G_ProcessManager.get();
    }
    else return false;

    // Register the instance pointer and function address to the Event Bus
    C_eventmanager->add_event(type, thisptr, reinterpret_cast<QWORD*>(funcAddr));
    return true;
}

/** @section Template_Instantiation Explicitly instantiate common event signatures. */
template bool Core::RegisterEvent<void(NetWork::*)(const std::string&, const std::string&, const std::string&)>(EventType, void(NetWork::*)(const std::string&, const std::string&, const std::string&));
