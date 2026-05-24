/**
 * @file GetHardware.h
 * @brief Interface declarations for kernel-mode hardware telemetry.
 * @details Provides low-level functions to extract immutable hardware identifiers 
 * (UUID, CPUID) by parsing physical SMBIOS tables.
 */

#pragma once
#include <ntddk.h>

/**
 * @brief Retrieves the System UUID from SMBIOS Type 1 structures.
 * @param Buffer Pointer to a buffer to receive the UUID string.
 * @param BufferSize Size of the destination buffer.
 * @return STATUS_SUCCESS if retrieval is successful.
 */
NTSTATUS GetMainboardSerial(PCHAR Buffer, ULONG BufferSize);

/**
 * @brief Extracts unique Processor IDs from all detected CPU cores.
 * @param Buffer Pointer to the buffer to receive the comma-separated CPU ID list.
 * @param BufferSize Size of the destination buffer.
 * @return STATUS_SUCCESS if retrieval is successful.
 */
NTSTATUS GetCPUInfo(PCHAR Buffer, ULONG BufferSize);

/**
 * @brief Retrieves a specific null-terminated string from an SMBIOS structure.
 * @param StringPtr Pointer to the string section of the SMBIOS structure.
 * @param StringIndex The index of the string to retrieve.
 * @return Pointer to the null-terminated string, or NULL if not found.
 */
PCHAR GetSMBIOSString(PUCHAR StringPtr, UCHAR StringIndex);

/**
 * @brief Constructs a TLV (Type-Length-Value) formatted response for user-mode consumption.
 * @param RequestType The type of hardware data requested (e.g., HW_REQUEST_ALL).
 * @param Message Pointer to receive the allocated kernel buffer.
 * @param MessageSize Pointer to receive the total size of the allocated buffer.
 * @return STATUS_SUCCESS if the packet was successfully constructed.
 */
NTSTATUS BuildHardwareResponseMessage(ULONG RequestType, PUCHAR* Message, PULONG MessageSize);
