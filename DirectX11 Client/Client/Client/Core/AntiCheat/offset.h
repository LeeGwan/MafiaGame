/**
 * @file offset.h
 * @brief Definition of IOCTL codes and data structures for User-Kernel communication.
 * @details This header defines the protocol for the 'Flect' Anti-Cheat system, 
 * covering Hardware ID (HWID) retrieval, process protection, and security alerting.
 */

#pragma once
#include <Windows.h>

// --- Message Type Identifiers (0x01 - 0x04) ---
#define MSG_TYPE_HEARTBEAT          1  /**< Validate driver existence/responsiveness. */
#define MSG_TYPE_DISCONNECT         2  /**< Signal termination of the communication link. */
#define MSG_TYPE_HARDWARE_REQUEST   3  /**< Request unique Hardware Identifiers. */
#define MSG_TYPE_HARDWARE_RESPONSE  4  /**< Driver-to-User hardware data payload. */

// --- Hardware Request Sub-Types (Bitmask/ID) ---
#define HW_REQUEST_ALL          0  /**< Probe all available hardware info. */
#define HW_REQUEST_MAINBOARD    1  /**< SMBIOS based Motherboard UUID. */
#define HW_REQUEST_CPU          2  /**< CPUID based Processor Serial. */
#define HW_REQUEST_STORAGE      4  /**< Disk/SSD Physical Serial. */

// --- Security Orchestration Types (0x05 - 0x0A) ---
#define SECURITY_REQUEST_OB_STATUS      5   /**< Query state of ObRegisterCallbacks. */
#define SECURITY_REQUEST_OB_REGISTER    6   /**< Install process protection callbacks. */
#define SECURITY_REQUEST_OB_UNREGISTER  7   /**< Remove process protection callbacks. */
#define SECURITY_REQUEST_GET_ALERTS      8   /**< Pull security violation logs from kernel. */
#define SECURITY_REQUEST_ADD_PID        9   /**< Whitelist a PID for kernel-level shielding. */
#define SECURITY_REQUEST_REMOVE_PID     10  /**< Remove a PID from the protection shield. */

/**
 * @section IOCTL_Definitions
 * Standard IOCTL codes generated via CTL_CODE macro.
 * DeviceType: FILE_DEVICE_UNKNOWN (Custom driver)
 * Access: FILE_ANY_ACCESS (Read/Write permission required)
 * Method: METHOD_BUFFERED (Safe buffering for small/medium payloads)
 */
#define IOCTL_HARDWARE_GET_INFO    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HARDWARE_HEARTBEAT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SECURITY_CONTROL     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_ALERTS 100 /**< Circular buffer limit for kernel alerts. */

#pragma pack(push, 1) // Ensure zero padding for cross-boundary data integrity

/** @struct MESSAGE_HEADER
 * @brief Fixed-size header for all driver messages.
 */
typedef struct _MESSAGE_HEADER {
    ULONG MessageType;   /**< Identifier (MSG_TYPE_*) */
    ULONG MessageId;     /**< Unique sequence ID. */
    USHORT FieldCount;   /**< Number of TLV fields following the header. */
    USHORT Reserved;     /**< Alignment padding. */
} MESSAGE_HEADER, * PMESSAGE_HEADER;

/** @struct MESSAGE_FIELD
 * @brief TLV (Type-Length-Value) structure for variable length hardware data.
 */
typedef struct _MESSAGE_FIELD {
    USHORT FieldId;    /**< Field category (HW_REQUEST_*) */
    ULONG DataSize;    /**< Length of the Data array. */
    UCHAR Data[1];     /**< Placeholder for variable-length payload. */
} MESSAGE_FIELD, * PMESSAGE_FIELD;

/** @struct HARDWARE_REQUEST
 * @brief Input buffer for IOCTL_HARDWARE_GET_INFO.
 */
typedef struct _HARDWARE_REQUEST {
    ULONG RequestType; 
    ULONG Reserved;     
} HARDWARE_REQUEST, * PHARDWARE_REQUEST;

/** @struct PROTECTION_REQUEST
 * @brief Control buffer for process shielding operations.
 */
typedef struct _PROTECTION_REQUEST {
    ULONG RequestType;  /**< Identifier (SECURITY_REQUEST_*) */
    HANDLE ProcessId;   /**< Target PID (0 = Caller). */
    ULONG Reserved;     
} PROTECTION_REQUEST, * PPROTECTION_REQUEST;

/** @struct PROTECTION_STATUS
 * @brief Output buffer for querying current protection metrics.
 */
typedef struct _PROTECTION_STATUS {
    BOOLEAN ProtectionEnabled;
    ULONG ProtectedCount;
    HANDLE ProtectedPIDs[50];
} PROTECTION_STATUS, * PPROTECTION_STATUS;

/** @struct SECURITY_ALERT
 * @brief Detailed log of a blocked access attempt.
 */
typedef struct _SECURITY_ALERT {
    HANDLE AttackerPID;    /**< PID of the unauthorized process. */
    HANDLE TargetPID;      /**< PID of the shielded process. */
    CHAR AttackerName[16]; /**< Executable name of the attacker. */
    LARGE_INTEGER Timestamp;
    DWORD AttemptedAccess; /**< Desired access mask (e.g., PROCESS_ALL_ACCESS). */
} SECURITY_ALERT, * PSECURITY_ALERT;

#pragma pack(pop)
