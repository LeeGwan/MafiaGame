/**
 * @file CompactBinaryReader.h
 * @brief Header for the secure and stateful binary data parser.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @class CompactBinaryReader
 * @brief A stream-oriented reader for parsing binary payloads with integrated safety checks.
 * * This class maintains an internal cursor to allow sequential extraction of typed data 
 * from a raw byte buffer. It is designed to be architecture-agnostic (Little-Endian) 
 * and resilient against buffer overflow attacks.
 */
class CompactBinaryReader {
private:
    const uint8_t* p_data;      ///< Pointer to the underlying raw data buffer.
    size_t data_size;           ///< Total size of the initialized buffer.
    size_t data_offset;         ///< Current reading position (cursor).

public:
    /** @brief Constructs an uninitialized reader. */
    CompactBinaryReader();

    /**
     * @brief Hooks the reader to a specific memory payload.
     * @param payload_ptr Pointer to the start of the binary data.
     * @param size Total capacity of the provided payload.
     */
    void init(const uint8_t* payload_ptr, size_t size);

    /**
     * @brief Validates if the requested number of bytes can be safely read.
     * @param bytes Number of bytes required for the next operation.
     * @return True if the operation will not exceed buffer boundaries.
     */
    bool HasData(size_t bytes) const;

    /** @brief Resets the reader's state and detaches from the current payload. */
    void clear();

    // --- Typed Extraction Methods ---

    /** @brief Reads a 1-byte unsigned integer. */
    uint8_t ReadUInt8();

    /** @brief Reads a 2-byte signed integer. */
    int16_t ReadInt16();

    /** @brief Reads a 2-byte unsigned integer. */
    uint16_t ReadUInt16();

    /** @brief Reads a 4-byte unsigned integer. */
    uint32_t ReadUInt32();

    /** @brief Reads a 4-byte signed integer. */
    int32_t ReadInt32();

    /**
     * @brief Reads an integer-encoded float with fixed precision scaling.
     * @param precision The scaling factor (default: 0.01f).
     */
    float ReadCompactFloat(float precision = 0.01f);

    /**
     * @brief Parses a length-prefixed string from the buffer.
     * @return The extracted string, or an empty string if validation fails.
     */
    std::string ReadString();

    /** @brief Parses a collection of length-prefixed strings. */
    std::vector<std::string> ReadStringVector();

    /** @brief Reads a single byte representing multiple bit-level flags. */
    uint8_t ReadBitFlags();

    /** @brief Reads a 1-byte value as a boolean. */
    bool ReadBool();
};
