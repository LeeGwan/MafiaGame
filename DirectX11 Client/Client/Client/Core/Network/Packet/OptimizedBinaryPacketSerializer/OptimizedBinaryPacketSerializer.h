/**
 * @file OptimizedBinaryPacketSerializer.h
 * @brief Header for the high-performance binary serialization engine with XXHash32 integrity checks.
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Explicit size_t definition for architecture consistency
typedef unsigned __int64 size_t;

// Forward Declarations
enum class PacketType : uint8_t;
struct TypePacket;
struct TwoStringPacket;
struct ResultPacket;
struct ResultAndHashPacket;
struct HashPacket;
struct ServerInfoPacket;
class CompactBinaryReader;

/**
 * @class OptimizedBinaryPacketSerializer
 * @brief Static utility class for secure and optimized binary data serialization.
 * * Implements a strict binary protocol including a magic number for validation,
 * versioning for backward compatibility, and XXHash32 for payload integrity.
 */
class OptimizedBinaryPacketSerializer
{
public:
    /** @brief Unique identifier to verify the protocol's legitimacy. */
    static const uint16_t MAGIC_NUMBER = 0x2340; 
    
    /** @brief Current packet version to manage protocol updates. */
    static const uint8_t VERSION = 2;            

    /**
     * @struct SecurePacketHeader
     * @brief 12-byte fixed header for every network packet.
     * * [Magic(2)][Version(1)][Type(1)][DataSize(4)][XXHash(4)]
     */
#pragma pack(push, 1) // Ensure 1-byte alignment for network transmission
    struct SecurePacketHeader {
        uint16_t magic;      // Security gate: Magic Number
        uint8_t version;     // Compatibility gate: Protocol Version
        uint8_t type;        // Logic gate: PacketType
        uint32_t data_size;  // Boundary gate: Payload length
        uint32_t xxhash;     // Integrity gate: Payload Checksum
    };
#pragma pack(pop)

    /** @brief Finalizes the packet by wrapping it with a SecurePacketHeader and XXHash32. */
    static void WrapPacketWithXXHash(uint8_t type, std::vector<uint8_t>* data, size_t size);

    /** @brief Low-level utility to push a single byte and increment the offset. */
    static void push(std::vector<uint8_t>* buffer, uint8_t value, size_t& offset);

    // --- Specialized Primitive Serializers ---
    
    static void SerializeString(std::vector<uint8_t>* buffer, const std::string& str, size_t& offset);
    static void SerializeUInt8(std::vector<uint8_t>* buffer, uint8_t value, size_t& offset);
    static void SerializeInt16(std::vector<uint8_t>* buffer, int16_t value, size_t& offset);
    static void SerializeUInt16(std::vector<uint8_t>* buffer, uint16_t value, size_t& offset);
    static void SerializeInt32(std::vector<uint8_t>* buffer, int32_t value, size_t& offset);
    static void SerializeUInt32(std::vector<uint8_t>* buffer, uint32_t value, size_t& offset);
    static void SerializeFloat(std::vector<uint8_t>* buffer, float value, size_t& offset);
    
    /** @brief Serializes a float with custom precision for bandwidth optimization. */
    static void SerializeCompactFloat(std::vector<uint8_t>* buffer, float value, float precision = 0.01f);
    
    static void SerializeBool(std::vector<uint8_t>* buffer, bool value, size_t& offset);
    static void SerializeBitFlags(std::vector<uint8_t>* buffer, uint8_t flags, size_t& offset);
    static void SerializeStringVector(std::vector<uint8_t>* buffer, const std::vector<std::string>& vec, size_t& offset);

    /** @brief Reinterprets the packet buffer to update its Type field. */
    static void ChangePacket(std::vector<uint8_t>& data, PacketType Change_type);

    /**
     * @brief Parses an incoming raw stream, performing all security and integrity checks.
     * @return True if the packet is valid and the XXHash matches.
     */
    static bool ParseSecurePacket(const std::vector<uint8_t>& data, PacketType& out_type, CompactBinaryReader* out_reader);

    /**
     * @brief Generic template to serialize specialized packet structures.
     */
    template <typename T>
    static void SerializePacket(const T& data, std::vector<uint8_t>* buffer);

    /**
     * @brief Generic template to deserialize binary data back into structures.
     */
    template <typename T>
    static void DeserializePacket(CompactBinaryReader& reader, T& out);
};
