/**
 * @file OptimizedBinaryPacketSerializer.cpp
 * @brief Full implementation of the binary serialization engine with integrated security gates.
 */

#include "OptimizedBinaryPacketSerializer.h"
#include "../../../utils/UtilsForString.h"
#include "../CompactBinaryReader/CompactBinaryReader.h"
#include "../PacketStructure/PacketStructure.h"
#include "../XXHash32/XXHash32.h"
#include <cstring>  

/**
 * @brief Finalizes the packet by wrapping it with a secure header and integrity hash.
 */
void OptimizedBinaryPacketSerializer::WrapPacketWithXXHash(uint8_t type, std::vector<uint8_t>* data, size_t size)
{
    SecurePacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = VERSION;
    header.type = type;
    header.data_size = static_cast<uint32_t>(size - sizeof(SecurePacketHeader));
    
    // Hash is calculated only for the payload, excluding the header itself.
    header.xxhash = XXHash32::hash(data->data() + sizeof(SecurePacketHeader), header.data_size);
    std::memcpy(data->data(), &header, sizeof(SecurePacketHeader));
}

void OptimizedBinaryPacketSerializer::push(std::vector<uint8_t>* buffer, uint8_t value, size_t& offset)
{
    (*buffer)[offset] = value;
    offset += 1;
}

// --- Primitive Serialization (Little-Endian) ---

void OptimizedBinaryPacketSerializer::SerializeString(std::vector<uint8_t>* buffer, const std::string& str, size_t& offset)
{
    uint16_t length = static_cast<uint16_t>(str.length());
    push(buffer, length & 0xff, offset);
    push(buffer, (length >> 8) & 0xff, offset);
    for (char c : str) {
        push(buffer, static_cast<uint8_t>(c), offset);
    }
}

void OptimizedBinaryPacketSerializer::SerializeUInt8(std::vector<uint8_t>* buffer, uint8_t value, size_t& offset) {
    push(buffer, value, offset);
}

void OptimizedBinaryPacketSerializer::SerializeInt16(std::vector<uint8_t>* buffer, int16_t value, size_t& offset) {
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
}

void OptimizedBinaryPacketSerializer::SerializeUInt16(std::vector<uint8_t>* buffer, uint16_t value, size_t& offset) {
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
}

void OptimizedBinaryPacketSerializer::SerializeInt32(std::vector<uint8_t>* buffer, int32_t value, size_t& offset) {
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
    push(buffer, (value >> 16) & 0xff, offset);
    push(buffer, (value >> 24) & 0xff, offset);
}

void OptimizedBinaryPacketSerializer::SerializeUInt32(std::vector<uint8_t>* buffer, uint32_t value, size_t& offset) {
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
    push(buffer, (value >> 16) & 0xff, offset);
    push(buffer, (value >> 24) & 0xff, offset);
}

void OptimizedBinaryPacketSerializer::SerializeFloat(std::vector<uint8_t>* buffer, float value, size_t& offset) {
    uint32_t floatAsInt = *reinterpret_cast<uint32_t*>(&value);
    SerializeInt32(buffer, static_cast<int32_t>(floatAsInt), offset);
}

void OptimizedBinaryPacketSerializer::SerializeCompactFloat(std::vector<uint8_t>* buffer, float value, float precision) {
    // Currently implemented as a placeholder for precision-based compression
    uint32_t compact_value = static_cast<uint32_t>(value / precision);
    // SerializeInt32(buffer, compact_value, offset); 
}

void OptimizedBinaryPacketSerializer::SerializeBool(std::vector<uint8_t>* buffer, bool value, size_t& offset) {
    push(buffer, value ? 1 : 0, offset);
}

void OptimizedBinaryPacketSerializer::SerializeBitFlags(std::vector<uint8_t>* buffer, uint8_t flags, size_t& offset) {
    push(buffer, flags ? 1 : 0, offset);
}

void OptimizedBinaryPacketSerializer::SerializeStringVector(std::vector<uint8_t>* buffer, const std::vector<std::string>& vec, size_t& offset) {
    SerializeUInt8(buffer, static_cast<uint8_t>(vec.size()), offset);
    for (const auto& str : vec) {
        SerializeString(buffer, str, offset);
    }
}

void OptimizedBinaryPacketSerializer::ChangePacket(std::vector<uint8_t>& data, PacketType Change_type) {
    SecurePacketHeader* header = reinterpret_cast<SecurePacketHeader*>(data.data());
    header->type = static_cast<uint8_t>(Change_type);
}

/**
 * @brief Validates the incoming stream against Magic, Version, Size, and XXHash.
 */
bool OptimizedBinaryPacketSerializer::ParseSecurePacket(const std::vector<uint8_t>& data, PacketType& out_type, CompactBinaryReader* out_reader) {
    if (data.size() < sizeof(SecurePacketHeader)) return false;

    const SecurePacketHeader* header = reinterpret_cast<const SecurePacketHeader*>(data.data());

    if (header->magic != MAGIC_NUMBER || header->version != VERSION) return false;
    if (data.size() < sizeof(SecurePacketHeader) + header->data_size) return false;

    const uint8_t* payload_ptr = data.data() + sizeof(SecurePacketHeader);
    size_t payload_size = header->data_size;

    if (XXHash32::hash(payload_ptr, payload_size) != header->xxhash) return false;

    out_reader->init(payload_ptr, payload_size);
    out_type = static_cast<PacketType>(header->type);

    return true;
}

// === Template Specializations: SERIALIZATION ===

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<TypePacket>(const TypePacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader));
    size_t data_offset = sizeof(SecurePacketHeader);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<ResultPacket>(const ResultPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeUInt8(buffer, static_cast<uint8_t>(data.ResultTypes), data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<HashPacket>(const HashPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeString(buffer, data.hash, data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<ResultAndHashPacket>(const ResultAndHashPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeUInt8(buffer, static_cast<uint8_t>(data.ResultTypes), data_offset);
    SerializeString(buffer, data.hash, data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<ServerInfoPacket>(const ServerInfoPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeString(buffer, data.IP, data_offset);
    SerializeUInt16(buffer, static_cast<uint16_t>(data.port), data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<TwoStringPacket>(const TwoStringPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeString(buffer, data.str1, data_offset);
    SerializeString(buffer, data.str2, data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<IntegrityCheckPacket>(const IntegrityCheckPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeString(buffer, data.hash, data_offset);
    SerializeString(buffer, data.Mainboard_ID, data_offset);
    SerializeString(buffer, data.CPU_ID, data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

template <>
void OptimizedBinaryPacketSerializer::SerializePacket<stringforVectorPacket>(const stringforVectorPacket& data, std::vector<uint8_t>* buffer) {
    buffer->resize(sizeof(SecurePacketHeader) + 256);
    size_t data_offset = sizeof(SecurePacketHeader);
    SerializeString(buffer, data.hash, data_offset);
    SerializeStringVector(buffer, data.str, data_offset);
    buffer->resize(data_offset);
    return WrapPacketWithXXHash(static_cast<uint8_t>(data.Type), buffer, data_offset);
}

// === Template Specializations: DESERIALIZATION (With Security Validation) ===

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<stringforVectorPacket>(CompactBinaryReader& reader, stringforVectorPacket& data) {
    data.hash = reader.ReadString();
    data.str = reader.ReadStringVector();
    
    // Sanitize: Verify hash and vector contents immediately after reading
    if (!UtilsForString::IsValidHash(data.hash)) data.hash.clear();
    for (auto& str : data.str) {
        if (!UtilsForString::IsValidETC(str)) str.clear();
    }
}

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<IntegrityCheckPacket>(CompactBinaryReader& reader, IntegrityCheckPacket& data) {
    data.hash = reader.ReadString();
    data.Mainboard_ID = reader.ReadString();
    data.CPU_ID = reader.ReadString();
    if (!UtilsForString::IsValidHash(data.hash)) data.hash.clear();
}

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<TwoStringPacket>(CompactBinaryReader& reader, TwoStringPacket& data) {
    data.str1 = reader.ReadString();
    data.str2 = reader.ReadString();
    // Validate credentials format during deserialization
    if (!UtilsForString::IsValidID(data.str1) || !UtilsForString::IsValidPassword(data.str2)) {
        data.str1.clear();
        data.str2.clear();
    }
}

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<HashPacket>(CompactBinaryReader& reader, HashPacket& data) {
    data.hash = reader.ReadString();
}

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(CompactBinaryReader& reader, ResultAndHashPacket& data) {
    data.ResultTypes = static_cast<ResultType>(reader.ReadUInt8());
    data.hash = reader.ReadString();
}

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(CompactBinaryReader& reader, ResultPacket& data) {
    data.ResultTypes = static_cast<ResultType>(reader.ReadUInt8());
}

template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<ServerInfoPacket>(CompactBinaryReader& reader, ServerInfoPacket& data) {
    data.IP = reader.ReadString();
    data.port = reader.ReadUInt16();
}
