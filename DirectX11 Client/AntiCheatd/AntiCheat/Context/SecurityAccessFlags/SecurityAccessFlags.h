/**
 * @file Access.h
 * @brief Definition of process and thread access mask constants for ObRegisterCallbacks.
 * @details This header defines the specific access rights that the driver will strip 
 * or block when an external process attempts to open a handle to the protected game client.
 */

#pragma once

// --- Process Access Rights (Standard Windows Flags) ---
#define PROCESS_TERMINATE                0x0001  /**< Right to terminate the process. */
#define PROCESS_CREATE_THREAD            0x0002  /**< Right to inject/create remote threads. */
#define PROCESS_VM_OPERATION             0x0008  /**< Right to manage virtual memory (e.g., VirtualAllocEx). */
#define PROCESS_VM_READ                  0x0010  /**< Right to read memory (ReadProcessMemory). */
#define PROCESS_VM_WRITE                 0x0020  /**< Right to write memory (WriteProcessMemory). */
#define PROCESS_DUP_HANDLE               0x0040  /**< Right to duplicate handles (Targeting anti-cheat protection). */
#define PROCESS_CREATE_PROCESS           0x0080  
#define PROCESS_SET_QUOTA                0x0100  
#define PROCESS_SET_INFORMATION          0x0200  /**< Right to modify process metadata. */
#define PROCESS_SUSPEND_RESUME           0x0800  /**< Right to freeze the process logic. */

/**
 * @section PROTECT_FULL_ACCESS
 * All critical permissions that must be stripped from external handles.
 * Includes memory tampering, thread injection, and process termination rights.
 */
#define PROTECT_FULL_ACCESS (PROCESS_TERMINATE | \
                             PROCESS_CREATE_THREAD | \
                             PROCESS_VM_OPERATION | \
                             PROCESS_VM_WRITE | \
                             PROCESS_DUP_HANDLE | \
                             PROCESS_SET_QUOTA | \
                             PROCESS_SET_INFORMATION | \
                             PROCESS_SUSPEND_RESUME)

/**
 * @section PROTECT_THREAD_ACCESS
 * Critical thread-level permissions to prevent hijacking or context manipulation.
 */
#define PROTECT_THREAD_ACCESS (THREAD_SUSPEND_RESUME | \
                               THREAD_TERMINATE | \
                               THREAD_SET_CONTEXT | \
                               THREAD_SET_INFORMATION | \
                               THREAD_GET_CONTEXT)
