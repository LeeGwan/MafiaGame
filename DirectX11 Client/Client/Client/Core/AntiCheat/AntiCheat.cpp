/**
 * @file AntiCheat.cpp
 * @brief Full implementation of the Kernel-Mode Driver Interface and Security Orchestration.
 * @details Manages driver service lifecycle, secure IOCTL communication, and HWID integrity verification.
 */

#include "AntiCheat.h"
#include <fstream>
#include <filesystem>
#include "../Network/Packet/PacketStructure/PacketStructure.h"
#include "../Network/Packet/RoutineProgress/RoutineProgress.h"
#include "../Utils/UtilsForString.h"

/** Global Singleton Instance for Security Orchestration */
std::unique_ptr<AntiCheat> G_AntiCheat = std::make_unique<AntiCheat>();

/**
 * @brief Constructor: Initializes state and dynamically builds the driver path.
 */
AntiCheat::AntiCheat() : m_hDevice(INVALID_HANDLE_VALUE), m_isConnected(false), m_shouldStop(false) {
    serviceName = L"Flect";
    displayName = L"Flect Anticheat";

    // Build absolute path for the .sys file
    std::filesystem::path currentPath = std::filesystem::current_path();
    driverPath = currentPath.wstring() + L"\\Anticheat\\AntiCheat.sys";
}

AntiCheat::~AntiCheat() {
    Disconnect();
}

bool AntiCheat::IsConnected() {
    return m_isConnected.load();
}

/**
 * @brief Establishes a link with the driver and enables kernel-level protection.
 */
bool AntiCheat::Connect() {
    if (!GetDriverStatus()) return false;

    m_hDevice = CreateFileW(
        DEVICE_PATH.c_str(), 
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
    );

    if (m_hDevice == INVALID_HANDLE_VALUE) return false;

    // Trigger Security Gateways: ObRegisterCallbacks & Process Shielding
    if (!EnableProtection()) return false;
    if (!AddProtectedPID(0)) return false;

    m_isConnected.store(true);
    m_shouldStop.store(false);

    // Launch background security monitoring loop
    m_EventThread = std::thread(&AntiCheat::EventLoop, this);
    return true;
}

/**
 * @brief Stops the driver service via Service Control Manager (SCM).
 */
bool AntiCheat::StopDriver() {
    if (!hService) return false;

    SERVICE_STATUS serviceStatus;
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus)) {
        if (GetLastError() == ERROR_SERVICE_NOT_ACTIVE) return true;
        return false;
    }
    return true;
}

/**
 * @brief Probes Hardware IDs (CPU/Mainboard) directly from kernel space.
 */
bool AntiCheat::RequestHardwareInfo(PacketType type, const std::string& hash) {
    if (!m_isConnected.load() || !GetDriverStatus()) return false;

    IntegrityCheckPacket Packet;
    HARDWARE_REQUEST request = { HW_REQUEST_ALL, 0 };
    Packet.Type = type;
    Packet.hash = hash;

    UCHAR responseBuffer[4096] = { 0 };
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        m_hDevice, IOCTL_HARDWARE_GET_INFO,
        &request, sizeof(request), responseBuffer, sizeof(responseBuffer),
        &bytesReturned, nullptr
    );

    if (!result || !ParseHardwareResponse(responseBuffer, bytesReturned, Packet)) return false;

    G_Routine->SendResponseForIntegrityCheckPacket(Packet);
    return true;
}

/**
 * @brief Parses TLV (Type-Length-Value) formatted hardware data from the kernel.
 */
bool AntiCheat::ParseHardwareResponse(PUCHAR buffer, DWORD bufferSize, IntegrityCheckPacket& Packet) {
    if (bufferSize < sizeof(MESSAGE_HEADER)) return false;

    PMESSAGE_HEADER header = reinterpret_cast<PMESSAGE_HEADER>(buffer);
    if (header->MessageType != MSG_TYPE_HARDWARE_RESPONSE) return false;

    PUCHAR currentPtr = buffer + sizeof(MESSAGE_HEADER);
    ULONG remainingSize = bufferSize - sizeof(MESSAGE_HEADER);

    for (USHORT i = 0; i < header->FieldCount; i++) {
        if (remainingSize < sizeof(MESSAGE_FIELD) - 1) break;

        PMESSAGE_FIELD field = reinterpret_cast<PMESSAGE_FIELD>(currentPtr);
        if (remainingSize < sizeof(MESSAGE_FIELD) - 1 + field->DataSize) break;

        std::string fieldData(reinterpret_cast<char*>(field->Data), field->DataSize);

        switch (field->FieldId) {
            case HW_REQUEST_MAINBOARD: Packet.Mainboard_ID = fieldData; break;
            case HW_REQUEST_CPU:       Packet.CPU_ID = fieldData;       break;
            default: break;
        }

        ULONG fieldSize = sizeof(MESSAGE_FIELD) - 1 + field->DataSize;
        currentPtr += fieldSize;
        remainingSize -= fieldSize;
    }
    return true;
}

/** @brief Enables ObRegisterCallbacks via kernel IOCTL. */
bool AntiCheat::EnableProtection() {
    PROTECTION_REQUEST request = { SECURITY_REQUEST_OB_REGISTER, 0, 0 };
    DWORD bytesReturned = 0;
    return DeviceIoControl(m_hDevice, IOCTL_SECURITY_CONTROL, &request, sizeof(request), nullptr, 0, &bytesReturned, nullptr);
}

/** @brief Protects the current process from handle stripping/memory access. */
bool AntiCheat::AddProtectedPID(DWORD pid) {
    if (!GetDriverStatus()) return false;
    if (pid == 0) pid = GetCurrentProcessId();

    PROTECTION_REQUEST request = { SECURITY_REQUEST_ADD_PID, (HANDLE)(ULONG_PTR)pid, 0 };
    DWORD bytesReturned = 0;
    return DeviceIoControl(m_hDevice, IOCTL_SECURITY_CONTROL, &request, sizeof(request), nullptr, 0, &bytesReturned, nullptr);
}

/** @brief Monitoring loop to pull security alerts from the driver log buffer. */
void AntiCheat::EventLoop() {
    while (!m_shouldStop.load()) {
        GetSecurityAlerts();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Throttling
    }
}

/** @brief Collects blocked access logs and reports them to the server. */
bool AntiCheat::GetSecurityAlerts() {
    if (!m_isConnected.load() || !GetDriverStatus()) return false;

    PROTECTION_REQUEST request = { SECURITY_REQUEST_GET_ALERTS, 0, 0 };
    SECURITY_ALERT alerts[MAX_ALERTS];
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(m_hDevice, IOCTL_SECURITY_CONTROL, &request, sizeof(request), alerts, sizeof(alerts), &bytesReturned, nullptr);

    if (result) {
        stringforVectorPacket packet;
        packet.Type = PacketType::ANTI_EVENT_REQUEST;
        packet.hash = copy_hash;

        for (int i = 0; i < MAX_ALERTS; i++) {
            if (alerts[i].AttackerPID != 0) packet.str.push_back(alerts[i].AttackerName);
        }
        if(!packet.str.empty()) G_Routine->SendResponseForstringforVectorPacket(packet);
        return true;
    }
    return (GetLastError() == ERROR_NO_MORE_ITEMS);
}

/** @brief Validates the heartbeat of the driver for session integrity. */
void AntiCheat::ServerCheckLogic(PacketType type) {
    ResultPacket packet = { type, (GetDriverStatus() && SendHeartbeat() && m_isConnected.load()) ? ResultType::Flect_Running : ResultType::Flect_Not_Running };
    G_Routine->SendResponseForstringforResultPacket(packet);
}

bool AntiCheat::SendHeartbeat() {
    if (!m_isConnected.load() || m_hDevice == INVALID_HANDLE_VALUE || !GetDriverStatus()) return false;

    const ULONG EXPECTED_HEARTBEAT = 0x12345678;
    ULONG heartbeatResponse = 0;
    DWORD bytesReturned = 0;

    DeviceIoControl(m_hDevice, IOCTL_HARDWARE_HEARTBEAT, nullptr, 0, &heartbeatResponse, sizeof(heartbeatResponse), &bytesReturned, nullptr);
    return (heartbeatResponse == EXPECTED_HEARTBEAT);
}

bool AntiCheat::GetDriverStatus() {
    if (!hService) return false;
    SERVICE_STATUS status;
    if (!QueryServiceStatus(hService, &status)) return false;
    return (status.dwCurrentState == SERVICE_START_PENDING || status.dwCurrentState == SERVICE_RUNNING);
}

/** @brief Orchestrates full security sequence: Register -> Start -> Connect. */
bool AntiCheat::Start(const std::string hash) {
    if (hash.empty() || !UtilsForString::IsValidHash(hash)) return false;
    copy_hash = hash;

    if (!RegisterDriver() || !StartDriver() || !Connect()) return false;
    return true;
}

/** @brief Tears down the security stack and clears SCM service entries. */
void AntiCheat::Disconnect() {
    if (!m_isConnected.load()) return;

    m_shouldStop.store(true);
    if (m_EventThread.joinable()) m_EventThread.join();

    if (m_hDevice != INVALID_HANDLE_VALUE) { CloseHandle(m_hDevice); m_hDevice = INVALID_HANDLE_VALUE; }

    m_isConnected.store(false);
    StopDriver();

    if (hService) { DeleteService(hService); CloseServiceHandle(hService); hService = NULL; }
    if (hSCManager) { CloseServiceHandle(hSCManager); hSCManager = NULL; }
}

/** @brief Registers the .sys file as a Kernel Driver Service. */
bool AntiCheat::RegisterDriver(DWORD startType) {
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) return false;

    hService = CreateService(
        hSCManager, serviceName.c_str(), displayName.c_str(),
        SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, startType, SERVICE_ERROR_NORMAL,
        driverPath.c_str(), NULL, NULL, NULL, NULL, NULL
    );

    if (!hService && GetLastError() == ERROR_SERVICE_EXISTS) {
        hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_ALL_ACCESS);
    }
    return (hService != NULL);
}

bool AntiCheat::StartDriver() {
    if (!hService) return false;
    if (!StartService(hService, 0, NULL)) {
        return (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING);
    }
    return true;
}
