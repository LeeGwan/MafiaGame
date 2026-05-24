// Fill out your copyright notice in the Description page of Project Settings.

#include "OptimizedBinaryPacketSerializer.h"
#include "CompactBinaryReader.h"
#include "PacketStructure.h"
#include "XXHash32.h"

/**
 * @brief Constructs a SecurePacketHeader and calculates the XXHash for the payload.
 * The calculated hash is stored in the header to ensure packet integrity at the destination.
 * * @param type The packet type identifier.
 * @param data Pointer to the buffer containing the packet data.
 * @param size Total size of the data buffer.
 */
void OptimizedBinaryPacketSerializer::WrapPacketWithXXHash(uint8 type, TArray<uint8>* data, SIZE_T size)
{
    SecurePacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = VERSION;
    header.type = type;
    header.data_size = static_cast<uint32>(size - sizeof(SecurePacketHeader));

    // Calculate non-cryptographic hash for fast integrity verification
    header.xxhash = XXHash32::hash(data->GetData() + sizeof(SecurePacketHeader), header.data_size);

    // Write the finalized header to the beginning of the buffer
    FMemory::Memcpy(data->GetData(), &header, sizeof(SecurePacketHeader));
}

/**
 * @brief Writes a single byte to the buffer and increments the offset.
 */
void OptimizedBinaryPacketSerializer::push(TArray<uint8>* buffer, uint8 value, SIZE_T& offset)
{
    (*buffer)[offset] = value;
    offset += 1;
}

/**
 * @brief Serializes an FString into the buffer.
 * Format: [2-byte Length (uint16_t)] + [UTF-8 Data]
 */
void OptimizedBinaryPacketSerializer::SerializeString(TArray<uint8>* buffer, const FString& str, SIZE_T& offset)
{
    FTCHARToUTF8 Convert(*str);
    uint16 length = static_cast<uint16>(Convert.Length());

    // Write length in Little-Endian format
    push(buffer, length & 0xff, offset);
    push(buffer, (length >> 8) & 0xff, offset);
    
    const char* CharArray = Convert.Get();
    for (int32 i = 0; i < Convert.Length(); ++i)
    {
        push(buffer, static_cast<uint8>(CharArray[i]), offset);
    }
}

void OptimizedBinaryPacketSerializer::SerializeUInt8(TArray<uint8>* buffer, uint8 value, SIZE_T& offset)
{
    push(buffer, value, offset);
}

/**
 * @brief Serializes a 16-bit signed integer in Little-Endian format.
 */
void OptimizedBinaryPacketSerializer::SerializeInt16(TArray<uint8>* buffer, int16 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
}

void OptimizedBinaryPacketSerializer::SerializeUInt16(TArray<uint8>* buffer, uint16 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
}

/**
 * @brief Serializes a 32-bit signed integer in Little-Endian format.
 */
void OptimizedBinaryPacketSerializer::SerializeInt32(TArray<uint8>* buffer, int32 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
    push(buffer, (value >> 16) & 0xff, offset);
    push(buffer, (value >> 24) & 0xff, offset);
}

void OptimizedBinaryPacketSerializer::SerializeUInt32(TArray<uint8>* buffer, uint32 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
    push(buffer, (value >> 16) & 0xff, offset);
    push(buffer, (value >> 24) & 0xff, offset);
}

/**
 * @brief Serializes a float by preserving its bit representation (IEEE 754).
 */
void OptimizedBinaryPacketSerializer::SerializeFloat(TArray<uint8>* buffer, float value, SIZE_T& offset)
{
    uint32 floatAsInt = *reinterpret_cast<uint32*>(&value);
    SerializeInt32(buffer, static_cast<int32>(floatAsInt), offset);
}

void OptimizedBinaryPacketSerializer::SerializeBool(TArray<uint8>* buffer, bool value, SIZE_T& offset)
{
    push(buffer, value ? 1 : 0, offset);
}

void OptimizedBinaryPacketSerializer::SerializeBitFlags(TArray<uint8>* buffer, uint8 flags, SIZE_T& offset)
{
    push(buffer, flags ? 1 : 0, offset);
}

/**
 * @brief Serializes an array of strings.
 * Format: [1-byte Count (uint8_t)] + [Serialized Strings...]
 */
void OptimizedBinaryPacketSerializer::SerializeStringVector(TArray<uint8>* buffer, const TArray<FString>& vec, SIZE_T& offset)
{
    SerializeUInt8(buffer, static_cast<uint8>(vec.Num()), offset);
    for (const auto& str : vec)
    {
        SerializeString(buffer, str, offset);
    }
}

/**
 * @brief Directly modifies the packet type in the SecurePacketHeader.
 */
void OptimizedBinaryPacketSerializer::ChangePacket(TArray<uint8>& data, PacketType Change_type)
{
    SecurePacketHeader* header = reinterpret_cast<SecurePacketHeader*>(data.GetData());
    header->type = static_cast<uint8>(Change_type);
}

/**
 * @brief Parses a raw buffer into a SecurePacket while performing rigorous integrity checks.
 * Validates Magic Number, Protocol Version, Data Size, and XXHash.
 * * @return True if the packet is valid and integrity is verified.
 */
bool OptimizedBinaryPacketSerializer::ParseSecurePacket(const TArray<uint8>& data, PacketType& out_type, CompactBinaryReader* out_reader)
{
    // 1. Boundary Check: Header size
    if (data.Num() < sizeof(SecurePacketHeader))
    {
        return false;
    }

    const SecurePacketHeader* header = reinterpret_cast<const SecurePacketHeader*>(data.GetData());

    // 2. Protocol Validation: Magic Number and Version
    if (header->magic != MAGIC_NUMBER || header->version != VERSION)
    {
        return false;
    }

    // 3. Boundary Check: Payload size consistency
    if (data.Num() < sizeof(SecurePacketHeader) + header->data_size)
    {
        return false;
    }

    const uint8* payload_ptr = data.GetData() + sizeof(SecurePacketHeader);
    SIZE_T payload_size = header->data_size;

    // 4. Integrity Check: XXHash verification
    uint32 calculated_hash = XXHash32::hash(payload_ptr, payload_size);
    if (calculated_hash != header->xxhash)
    {
        return false;
    }

    // 5. Semantic Check: Packet Type range
    if (static_cast<PacketType>(header->type) >= PacketType::MaxPacketSize)
    {
        return false;
    }
    
    // Initialize the reader with the verified payload
    out_reader->Init(payload_ptr, payload_size);
    out_type = static_cast<PacketType>(header->type);
    
    return true;
}

/**
 * @brief Specialized deserialization for FUserAuthData.
 */
template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<FUserAuthData>(CompactBinaryReader& reader, FUserAuthData& data)
{
    data.hash = reader.ReadStringArray();
    return;
}
