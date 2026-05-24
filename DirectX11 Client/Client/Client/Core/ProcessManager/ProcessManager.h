/**
 * @file ProcessManager.h
 * @brief Header for the game process orchestration and lifecycle management.
 */

#pragma once
#include <memory>
#include <string>
#include <cstdint>

/**
 * @class ProcessManager
 * @brief Handles the execution and monitoring of the external Unreal Engine game process.
 * * This class provides a high-level interface to launch the game client with 
 * secure credentials and network parameters, ensuring seamless transition 
 * from the lobby/launcher to the active game session.
 */
class ProcessManager
{
public:
    /**
     * @brief Spawns the Unreal Engine game client process.
     * @param IP The target server IP address for network synchronization.
     * @param port The target server port for the game session.
     * * The method constructs a secure command-line string including the 
     * session hash and user identity, then registers the new PID to the security layer.
     */
    void ProcessRunner(const std::string& IP, uint16_t port);

private:
    // Lifecycle and process monitoring variables can be added here
};

/** Global access point for the ProcessManager singleton instance. */
extern std::unique_ptr<ProcessManager> G_ProcessManager;
