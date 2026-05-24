/**
 * @file CompactBinaryReader.cpp
 * @brief Implementation of a secure and efficient binary data parser (Little-Endian).
 */

#include "CompactBinaryReader.h"
#include <cstdint>

/** @brief Default constructor for initializing the reader state. */
CompactBinaryReader::CompactBinaryReader() : p_data(nullptr), data_size(0), data_offset(0) {}

/** @brief Resets the internal state and pointers. */
void CompactBinaryReader::clear() { 
    p_data = nullptr; 
    data_size = 0; 
    data_offset = 0; 
}

/**
 * @brief Initializes the reader with a source buffer.
 * @param payload_ptr Pointer to the raw binary payload.
 * @param size Total size of the payload in bytes.
 */
void CompactBinaryReader::init(const uint8_t* payload_ptr, size_t size)
{
    p_data = payload_ptr;
    data_size = size;
    data_offset = 0;
}

/**
 * @brief Performs a bounds check to ensure safe memory access.
 * @param bytes Number of bytes requested to be read.
 * @return True if the requested bytes are within the buffer's capacity.
 */
bool CompactBinaryReader::HasData(size_t bytes) const {
    return data_offset + bytes <= data_size;
}

/** @section Primitive_Readers Implementation of Little-Endian decoding. */

uint8_t CompactBinaryReader::ReadUInt8() {
    return HasData(1) ? (p_data[data_offset++]) : 0;
}

uint16_t CompactBinaryReader::ReadUInt16() {
    if (!HasData(2)) return 0;
    // Manual bitwise reconstruction ensures Little-Endian compliance across architectures.
    uint16_t result = static_cast<uint16_t>(p_data[data_offset] | (p_data[data_offset + 1] << 8));
    data_offset += 2;
    return result;
}

int16_t CompactBinaryReader::ReadInt16() {
    return static_cast<int16_t>(ReadUInt16());
}

uint32_t CompactBinaryReader::ReadUInt32() {
    if (!HasData(4)) return 0;
    uint32_t result = static_cast<uint32_t>(
        p_data[data_offset] | 
        (p_data[data_offset + 1] << 8) |
        (p_data[data_offset + 2] << 16) | 
        (p_data[data_offset + 3] << 24));
    data_offset += 4;
    return result;
}

int32_t CompactBinaryReader::ReadInt32() {
    return static_cast<int32_t>(ReadUInt32());
}

/**
 * @brief Reads a floating-point value encoded as an integer with fixed precision.
 * @param precision Scaling factor applied to the stored integer.
 */
float CompactBinaryReader::ReadCompactFloat(float precision) {
    return static_cast<float>(ReadInt32()) * precision;
}

/**
 * @brief Securely parses a length-prefixed string.
 * @details Includes a security policy that rejects strings longer than 100 characters
 * to mitigate memory exhaustion (DoS) risks.
 */
std::string CompactBinaryReader::ReadString() {
    if (!HasData(2)) return "";

    uint16_t length = p_data[data_offset] | (p_data[data_offset + 1] << 8);
    data_offset += 2;

    // Security Gate: Ensure the requested length is available and within reasonable limits.
    if (!HasData(length) || length > 100) {
        return "";
    }

    std::string result;
    result.reserve(length); // Efficient memory allocation based on known size.
    for (uint16_t i = 0; i < length; ++i) {
        result.push_back(static_cast<char>(p_data[data_offset + i]));
    }
    data_offset += length;

    return result;
}

/**
 * @brief Reads a collection of strings prefixed by a count byte.
 */
std::vector<std::string> CompactBinaryReader::ReadStringVector() {
    uint8_t count = ReadUInt8();
    std::vector<std::string> result;
    result.reserve(count);
    for (uint8_t i = 0; i < count; ++i) {
        result.push_back(ReadString());
    }
    return result;
}

uint8_t CompactBinaryReader::ReadBitFlags() { return ReadUInt8(); }

bool CompactBinaryReader::ReadBool() { return ReadUInt8() != 0; }
