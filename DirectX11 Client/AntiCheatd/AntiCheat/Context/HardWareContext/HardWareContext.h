/**
 * @file SMBIOS.h
 * @brief Definitions for SMBIOS table parsing and hardware telemetry protocols.
 * @details This header defines the structures required to map and parse SMBIOS tables 
 * located in physical memory, used for high-integrity HWID collection.
 */

#pragma once
#include <ntddk.h>

// --- Communication Protocol Structures ---

/** @struct MESSAGE_HEADER
 * @brief Fixed-size header for all driver-to-user messages.
 */
typedef struct _MESSAGE_HEADER {
    ULONG MessageType;   /**< Identifier (MSG_TYPE_*) */
    ULONG MessageId;     /**< Unique sequence ID. */
    USHORT FieldCount;   /**< Number of TLV fields in the payload. */
    USHORT Reserved;     /**< Alignment padding. */
} MESSAGE_HEADER, * PMESSAGE_HEADER;

/** @struct MESSAGE_FIELD
 * @brief TLV (Type-Length-Value) field for variable length data.
 */
typedef struct _MESSAGE_FIELD {
    USHORT FieldId;    /**< Field category (HW_REQUEST_*) */
    ULONG DataSize;    /**< Length of the Data array. */
    UCHAR Data[1];     /**< Variable length data placeholder. */
} MESSAGE_FIELD, * PMESSAGE_FIELD;

// --- SMBIOS Specific Structures ---

/** @struct SMBIOS_ENTRY_POINT
 * @brief The anchor structure located in physical memory (0xF0000 - 0xFFFFF).
 * Contains the address and length of the actual SMBIOS table.
 */
typedef struct _SMBIOS_ENTRY_POINT {
    UCHAR Anchor[4];              /**< "_SM_" signature. */
    UCHAR Checksum;               
    UCHAR Length;                 /**< Structure length (typically 0x1F). */
    UCHAR MajorVersion;           
    UCHAR MinorVersion;           
    USHORT MaxStructureSize;      
    UCHAR EntryPointRevision;     
    UCHAR FormattedArea[5];       
    UCHAR IntermediateAnchor[5];  /**< "_DMI_" signature. */
    UCHAR IntermediateChecksum;   
    USHORT TableLength;           /**< Total size of the SMBIOS table. */
    ULONG TableAddress;           /**< Physical address of the table. */
    USHORT NumberOfStructures;    /**< Total count of SMBIOS structures. */
    UCHAR BCDRevision;            
} SMBIOS_ENTRY_POINT, * PSMBIOS_ENTRY_POINT;

/** @struct SMBIOS_HEADER
 * @brief Common header present at the start of every SMBIOS structure.
 */
typedef struct _SMBIOS_HEADER {
    UCHAR Type;      /**< Structure type (e.g., Type 2 = Baseboard). */
    UCHAR Length;    /**< Length of the formatted portion. */
    USHORT Handle;   /**< Unique handle for the structure. */
} SMBIOS_HEADER, * PSMBIOS_HEADER;

/** @struct SMBIOS_SYSTEM_INFO (Type 1)
 * @brief Contains the unique System UUID used for hardware identity.
 */
typedef struct _SMBIOS_SYSTEM_INFO {
    SMBIOS_HEADER Header;
    UCHAR Manufacturer;   
    UCHAR ProductName;    
    UCHAR Version;        
    UCHAR SerialNumber;   
    UCHAR UUID[16];       /**< High-integrity System Fingerprint. */
    UCHAR WakeupType;     
} SMBIOS_SYSTEM_INFO, * PSMBIOS_SYSTEM_INFO;

/** @struct SMBIOS_BASEBOARD_INFO (Type 2)
 * @brief Contains motherboard specific identification strings.
 */
typedef struct _SMBIOS_BASEBOARD_INFO {
    SMBIOS_HEADER Header;
    UCHAR Manufacturer;   
    UCHAR Product;        
    UCHAR Version;        
    UCHAR SerialNumber;   /**< Motherboard Serial String Index. */
    UCHAR AssetTag;       
    UCHAR FeatureFlags;   
    UCHAR LocationInChassis; 
    USHORT ChassisHandle;  
    UCHAR BoardType;      
    UCHAR NumberOfContainedObjectHandles;
} SMBIOS_BASEBOARD_INFO, * PSMBIOS_BASEBOARD_INFO;

/** @struct SMBIOS_PROCESSOR_INFO (Type 4)
 * @brief Contains hardware-level CPU ID and manufacturing details.
 */
typedef struct _SMBIOS_PROCESSOR_INFO {
    SMBIOS_HEADER Header;
    UCHAR SocketDesignation; 
    UCHAR ProcessorType;     
    UCHAR ProcessorFamily;   
    UCHAR ProcessorManufacturer; 
    UCHAR ProcessorID[8];    /**< Raw CPU Identifier. */
    UCHAR ProcessorVersion;  
    UCHAR Voltage;           
    USHORT ExternalClock;    
    USHORT MaxSpeed;         
    USHORT CurrentSpeed;     
    UCHAR Status;            
    UCHAR ProcessorUpgrade;  
    USHORT L1CacheHandle;    
    USHORT L2CacheHandle;    
    USHORT L3CacheHandle;    
    UCHAR SerialNumber;      
    UCHAR AssetTag;          
    UCHAR PartNumber;        
    UCHAR CoreCount;         
    UCHAR CoreEnabled;       
    UCHAR ThreadCount;       
    USHORT ProcessorCharacteristics;
} SMBIOS_PROCESSOR_INFO, * PSMBIOS_PROCESSOR_INFO;

/** @struct HARDWARE_REQUEST
 * @brief IOCTL input structure for requesting hardware telemetry.
 */
typedef struct _HARDWARE_REQUEST {
    ULONG RequestType;  /**< Target component (HW_REQUEST_*). */
    ULONG Reserved;     
} HARDWARE_REQUEST, * PHARDWARE_REQUEST;
