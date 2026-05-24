// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// Forward declarations for networking and data structures
enum class PacketType : uint8;
struct TypePacket;
struct TwoStringPacket;
struct ResultPacket;
struct ResultAndHashPacket;
struct HashPacket;
struct ServerInfoPacket;
class CompactBinaryReader;

/**
 * @class OptimizedBinaryPacketSerializer
 * @brief Static utility class for high-performance binary serialization and packet integrity verification.
 * * Provides a suite of functions to transform high-level data structures into compact binary streams
 * and validates incoming packets using a secure header mechanism.
 */
class MAFIAGAME_API OptimizedBinaryPacketSerializer
{
public:
    /** Validation constant to identify legitimate packets and prevent cross-protocol interference. */
    static const uint16 MAGIC_NUMBER = 0x2340;

    /** Protocol versioning to ensure compatibility between client and server. */
    static const uint8 VERSION = 2;

    /**
     * @struct SecurePacketHeader
     * @brief Fixed-size header for all network packets to ensure integrity and correct routing.
     */
    struct SecurePacketHeader
    {
        uint16 magic;      // Protocol identification magic number
        uint8 version;     // Protocol version
        uint8 type;        // Packet type identifier
        uint32 data_size;  // Size of the payload (excluding header)
        uint32 xxhash;     // XXHash32 checksum for payload integrity verification
    };

    /**
     * @brief Finalizes a packet by calculating the XXHash and attaching the SecurePacketHeader.
     */
    static void WrapPacketWithXXHash(uint8 type, TArray<uint8>* data, SIZE_T size);

    /** @brief Internal helper to push a raw byte into the buffer. */
    static void push(TArray<uint8>* buffer, uint8 value, SIZE_T& offset);
    
    /** * @section Serialization_Methods
     * Specialized functions for writing data types into the binary stream in Little-Endian format.
     */
    static void SerializeString(TArray<uint8>* buffer, const FString& str, SIZE_T& offset);
    static void SerializeUInt8(TArray<uint8>* buffer, uint8 value, SIZE_T& offset);
    static void SerializeInt16(TArray<uint8>* buffer, int16 value, SIZE_T& offset);
    static void SerializeUInt16(TArray<uint8>* buffer, uint16 value, SIZE_T& offset);
    static void SerializeInt32(TArray<uint8>* buffer, int32 value, SIZE_T& offset);
    static void SerializeUInt32(TArray<uint8>* buffer, uint32 value, SIZE_T& offset);
    static void SerializeFloat(TArray<uint8>* buffer, float value, SIZE_T& offset);
    static void SerializeBool(TArray<uint8>* buffer, bool value, SIZE_T& offset);
    static void SerializeBitFlags(TArray<uint8>* buffer, uint8 flags, SIZE_T& offset);
    static void SerializeStringVector(TArray<uint8>* buffer, const TArray<FString>& vec, SIZE_T& offset);

    /**
     * @brief Modifies the type field of an existing packet header.
     */
    static void ChangePacket(TArray<uint8>& data, PacketType Change_type);

    /**
     * @brief Validates and parses a raw byte stream into a readable packet format.
     * Checks magic number, version, and hash before initializing the reader.
     * @return True if the packet is authentic and uncorrupted.
     */
    static bool ParseSecurePacket(const TArray<uint8>& data, PacketType& out_type, CompactBinaryReader* out_reader);

    /**
     * @brief Template function for deserializing various packet structures.
     * Specific structures must provide a template specialization.
     */
    template <typename T>
    static void DeserializePacket(CompactBinaryReader& reader, T& out);
};
