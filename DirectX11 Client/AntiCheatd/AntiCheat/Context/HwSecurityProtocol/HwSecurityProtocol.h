/**
 * @file Protocol.h
 * @brief Communication protocol and IOCTL definitions for User-Kernel interaction.
 * @details This header defines the control codes and request types used by 
 * DeviceIoControl to interface with the 'Flect' Anti-Cheat driver.
 */

#pragma once

// --- Message Type Identifiers (0x01 - 0x04) ---
#define MSG_TYPE_HEARTBEAT          1  /**< Verify driver presence and responsiveness. */
#define MSG_TYPE_DISCONNECT         2  /**< Signal termination of the session. */
#define MSG_TYPE_HARDWARE_REQUEST   3  /**< Request hardware fingerprinting data. */
#define MSG_TYPE_HARDWARE_RESPONSE  4  /**< Driver-to-User hardware data payload. */

// --- Hardware Request Component Types ---
#define HW_REQUEST_ALL          0  /**< Query all available hardware identifiers. */
#define HW_REQUEST_MAINBOARD    1  /**< Request Motherboard UUID via SMBIOS. */
#define HW_REQUEST_CPU          2  /**< Request CPUID based Serial Number. */
#define HW_REQUEST_STORAGE      4  /**< Reserved for Disk/Storage Serial. */

// --- Security Orchestration Request Types (0x05 - 0x0A) ---
#define SECURITY_REQUEST_OB_STATUS      5   /**< Query the current state of ObRegisterCallbacks. */
#define SECURITY_REQUEST_OB_REGISTER    6   /**< Install process protection/stripping callbacks. */
#define SECURITY_REQUEST_OB_UNREGISTER  7   /**< Remove active security callbacks. */
#define SECURITY_REQUEST_GET_ALERTS      8   /**< Pull security violation logs from the kernel buffer. */
#define SECURITY_REQUEST_ADD_PID        9   /**< Add a process ID to the driver's protection whitelist. */
#define SECURITY_REQUEST_REMOVE_PID     10  /**< Remove a process ID from the protection whitelist. */

/**
 * @section IOCTL_Definitions
 * Generated via the CTL_CODE macro for DeviceIoControl.
 * * - DeviceType: FILE_DEVICE_UNKNOWN (Custom driver category)
 * - Method: METHOD_BUFFERED (I/O Manager manages memory copying for safety)
 * - Access: FILE_ANY_ACCESS (Requires generalized Read/Write handle)
 */
#define IOCTL_HARDWARE_GET_INFO    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_HARDWARE_HEARTBEAT   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SECURITY_CONTROL     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
