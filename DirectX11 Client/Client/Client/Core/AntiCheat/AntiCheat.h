/**
 * @file AntiCheat.h
 * @brief Header for the Kernel-Mode Driver Controller and Security Manager.
 * @details Interface for process protection via ObRegisterCallbacks and Hardware ID (HWID) telemetry.
 */

#pragma once
#include "offset.h"
#include <thread>
#include <string>
#include <atomic>
#include <memory>

// Forward declarations for networking and data structures
enum class PacketType : uint8_t;
struct IntegrityCheckPacket;

/**
 * @class AntiCheat
 * @brief Manages the lifecycle and communication of the 'Flect' kernel driver.
 * * This class implements high-level security features including anti-tampering,
 * HWID spoof-detection, and real-time security alert monitoring.
 */
class AntiCheat
{
public:
    AntiCheat();
    ~AntiCheat();

    /**
     * @brief Bootstraps the security stack: Register -> Start -> Connect.
     * @param hash Session hash for server-side verification.
     * @return True if the driver is successfully active and linked.
     */
    bool Start(const std::string hash);

    /** @brief Checks the current atomic connection state of the driver. */
    bool IsConnected();

    /** @brief Gracefully shuts down the monitoring loop and unregisters the kernel service. */
    void Disconnect();

    /**
     * @brief Requests HWID (CPU/Mainboard) from the kernel and dispatches to the server.
     * @return True if hardware probes were successful.
     */
    bool RequestHardwareInfo(PacketType type, const std::string& hash);

    /**
     * @brief Registers a Process ID for kernel-level handle protection.
     * @param pid Target PID. If 0, the current process is protected.
     */
    bool AddProtectedPID(DWORD pid);

    /**
     * @brief Periodically validates driver integrity and heartbeat for the server.
     * @param type PacketType for the response.
     */
    void ServerCheckLogic(PacketType type);

private:
    // --- Driver Service Lifecycle (SCM API) ---
    
    /** @brief Creates the driver entry in the Windows Service Control Manager. */
    bool RegisterDriver(DWORD startType = SERVICE_DEMAND_START);

    /** @brief Initiates the driver service start sequence. */
    bool StartDriver();

    /** @brief Requests the driver service to stop. */
    bool StopDriver();

    /** @brief Verifies the current SCM status (e.g., SERVICE_RUNNING). */
    bool GetDriverStatus();

    // --- Communication & Monitoring ---

    /** @brief Opens a handle to the driver's device object ("\\\\.\\Flect"). */
    bool Connect();

    /** @brief Worker thread: Periodically polls the driver for security violation logs. */
    void EventLoop();

    /** @brief Retrieves the latest security alerts (Attacker PID/Name) from the kernel. */
    bool GetSecurityAlerts();

    /**
     * @brief Deserializes kernel-mode hardware data into a user-mode structure.
     * @param buffer Raw TLV buffer from the driver.
     */
    bool ParseHardwareResponse(PUCHAR buffer, DWORD bufferSize, IntegrityCheckPacket& Packet);

    /** @brief Enables kernel-level ObRegisterCallbacks for process shield. */
    bool EnableProtection();

    /** @brief Sends a magic-value challenge (0x12345678) to verify driver responsiveness. */
    bool SendHeartbeat();

private:
    // --- Service Management Handles ---
    SC_HANDLE hSCManager;        /**< Handle to the Service Control Manager database. */
    SC_HANDLE hService;          /**< Handle to the specific driver service. */
    
    // --- Metadata & Paths ---
    std::wstring serviceName;    /**< Registry name: "Flect". */
    std::wstring displayName;    /**< Friendly name: "Flect Simple Anticheat". */
    std::wstring driverPath;     /**< Absolute disk path to AntiCheat.sys. */

    // --- Communication & Threading ---
    HANDLE m_hDevice;                   /**< Device communication handle (Ring 3 <-> Ring 0). */
    std::atomic<bool> m_isConnected;    /**< Atomic flag for active driver link. */
    std::atomic<bool> m_shouldStop;     /**< Atomic flag to terminate the monitoring loop. */
    std::thread m_EventThread;          /**< Dedicated thread for security event polling. */
    
    std::string copy_hash;              /**< Cached session hash for packet building. */
    const std::wstring DEVICE_PATH = L"\\\\.\\Flect"; /**< Symbolic link to the driver device. */
};

/** @brief Global access point for the AntiCheat orchestrator. */
extern std::unique_ptr<AntiCheat> G_AntiCheat;
