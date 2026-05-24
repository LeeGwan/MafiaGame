/**
 * @file XXHash32.cpp
 * @brief Implementation of the XXHash32 algorithm for high-speed non-cryptographic hashing.
 */

#include "XXHash32.h"

/**
 * @brief Performs a 32-bit left circular rotation (ROTL).
 * @param x The value to rotate.
 * @param r The number of bits to shift.
 * @return The rotated 32-bit integer.
 */
uint32_t XXHash32::rotl32(uint32_t x, int r)
{
    return (x << r) | (x >> (32 - r));
}

/**
 * @brief Reads a 32-bit unsigned integer from a memory location.
 * @note Uses reinterpret_cast for direct memory access to optimize throughput.
 */
uint32_t XXHash32::read32(const uint8_t* ptr)
{
    return *reinterpret_cast<const uint32_t*>(ptr);
}

/**
 * @brief Computes the 32-bit XXHash value for a given input buffer.
 * @param input Pointer to the data to be hashed.
 * @param len Length of the input data in bytes.
 * @param seed Optional seed value for hash randomization.
 * @return The resulting 32-bit hash value.
 */
uint32_t XXHash32::hash(const void* input, size_t len, uint32_t seed)
{
    const uint8_t* p = static_cast<const uint8_t*>(input);
    const uint8_t* const bEnd = p + len;
    uint32_t h32;

    /**
     * @section Bulk_Processing
     * Processes data in 16-byte blocks using four independent accumulators (v1-v4).
     * This structure leverages Instruction-Level Parallelism (ILP).
     */
    if (len >= 16) {
        const uint8_t* const limit = bEnd - 16;
        uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
        uint32_t v2 = seed + PRIME32_2;
        uint32_t v3 = seed + 0;
        uint32_t v4 = seed - PRIME32_1;

        do {
            v1 = rotl32(v1 + read32(p) * PRIME32_2, 13) * PRIME32_1;
            p += 4;
            v2 = rotl32(v2 + read32(p) * PRIME32_2, 13) * PRIME32_1;
            p += 4;
            v3 = rotl32(v3 + read32(p) * PRIME32_2, 13) * PRIME32_1;
            p += 4;
            v4 = rotl32(v4 + read32(p) * PRIME32_2, 13) * PRIME32_1;
            p += 4;
        } while (p <= limit);

        // Merge accumulators into a single 32-bit state
        h32 = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
    }
    else {
        h32 = seed + PRIME32_5;
    }

    h32 += static_cast<uint32_t>(len);

    /**
     * @section Finalization_Steps
     * Processes remaining data in 4-byte and 1-byte increments.
     */
    while (p + 4 <= bEnd) {
        h32 += read32(p) * PRIME32_3;
        h32 = rotl32(h32, 17) * PRIME32_4;
        p += 4;
    }

    while (p < bEnd) {
        h32 += (*p) * PRIME32_5;
        h32 = rotl32(h32, 11) * PRIME32_1;
        p++;
    }

    /**
     * @section Avalanche_Mixing
     * Final mixing stage (Avalanche effect) to ensure all bits of the input
     * affect all bits of the resulting hash.
     */
    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}
