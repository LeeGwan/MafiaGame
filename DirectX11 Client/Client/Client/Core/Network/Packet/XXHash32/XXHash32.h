/**
 * @file XXHash32.h
 * @brief High-performance, non-cryptographic 32-bit hash algorithm implementation.
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 * @class XXHash32
 * @brief Provides a static interface for computing XXHash32 values.
 * * XXHash32 is a fast hash algorithm that works at speeds close to RAM limits.
 * It is highly effective for checksums, hash map keys, and data integrity verification
 * in real-time systems like game engines and high-frequency network servers.
 */
class XXHash32
{
private:
    /** @section Magic_Primes 
     * Specific prime numbers used by the XXHash algorithm to distribute 
     * input bits evenly across the output hash space.
     */
    static const uint32_t PRIME32_1 = 0x9E3779B1U;
    static const uint32_t PRIME32_2 = 0x85EBCA77U;
    static const uint32_t PRIME32_3 = 0xC2B2AE3DU;
    static const uint32_t PRIME32_4 = 0x27D4EB2FU;
    static const uint32_t PRIME32_5 = 0x165667B1U;

    /**
     * @brief Rotates the bits of a 32-bit integer to the left.
     * @param x The value to rotate.
     * @param r The shift count.
     */
    static uint32_t rotl32(uint32_t x, int r);

    /**
     * @brief Reads a 4-byte block from memory and interprets it as a uint32_t.
     * @param ptr The source memory address.
     */
    static uint32_t read32(const uint8_t* ptr);

public:
    /**
     * @brief Computes the 32-bit XXHash of the provided input buffer.
     * * @param input Pointer to the raw data buffer.
     * @param len The size of the input buffer in bytes.
     * @param seed Optional seed value for generating unique hashes for the same input.
     * @return A 32-bit unsigned integer representing the computed hash.
     */
    static uint32_t hash(const void* input, size_t len, uint32_t seed = 0);
};
