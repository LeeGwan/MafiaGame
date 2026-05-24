// Fill out your copyright notice in the Description page of Project Settings.

#include "XXHash32.h"

/**
 * @brief Executes the XXHash32 algorithm on a given input.
 * Optimized for high-throughput hashing by using 4 parallel internal lanes for blocks >= 16 bytes.
 * * @param Input Pointer to the data to be hashed.
 * @param Length Size of the input data in bytes.
 * @param Seed Optional seed value for randomization.
 * @return uint32 The resulting 32-bit hash value.
 */
uint32 XXHash32::hash(const void* Input, SIZE_T Length, uint32 Seed)
{
    const uint8* P = static_cast<const uint8*>(Input);
    const uint8* const BEnd = P + Length;
    uint32 H32;

    // Process blocks of 16 bytes using 4 parallel state accumulators
    if (Length >= 16)
    {
        const uint8* const Limit = BEnd - 16;
        
        // Initialize internal state vectors
        uint32 V1 = Seed + PRIME32_1 + PRIME32_2;
        uint32 V2 = Seed + PRIME32_2;
        uint32 V3 = Seed + 0;
        uint32 V4 = Seed - PRIME32_1;

        // Main processing loop: 16 bytes per iteration
        do
        {
            V1 = Rotl32(V1 + Read32(P) * PRIME32_2, 13) * PRIME32_1;
            P += 4;
            V2 = Rotl32(V2 + Read32(P) * PRIME32_2, 13) * PRIME32_1;
            P += 4;
            V3 = Rotl32(V3 + Read32(P) * PRIME32_2, 13) * PRIME32_1;
            P += 4;
            V4 = Rotl32(V4 + Read32(P) * PRIME32_2, 13) * PRIME32_1;
            P += 4;
        } while (P <= Limit);

        // Merge the 4 internal lanes into a single 32-bit value
        H32 = Rotl32(V1, 1) + Rotl32(V2, 7) + Rotl32(V3, 12) + Rotl32(V4, 18);
    }
    else
    {
        // For inputs smaller than 16 bytes, start with a base state
        H32 = Seed + PRIME32_5;
    }

    // Add total length to the hash state for additional entropy
    H32 += static_cast<uint32>(Length);

    // Process remaining 4-byte chunks (32-bit remaining units)
    while (P + 4 <= BEnd)
    {
        H32 += Read32(P) * PRIME32_3;
        H32 = Rotl32(H32, 17) * PRIME32_4;
        P += 4;
    }

    // Process final remaining bytes (1 to 3 bytes)
    while (P < BEnd)
    {
        H32 += (*P) * PRIME32_5;
        H32 = Rotl32(H32, 11) * PRIME32_1;
        P++;
    }

    /**
     * @section Finalization_Mix
     * Avalanche step: Distributes entropy throughout the bits to ensure high quality.
     */
    H32 ^= H32 >> 15;
    H32 *= PRIME32_2;
    H32 ^= H32 >> 13;
    H32 *= PRIME32_3;
    H32 ^= H32 >> 16;

    return H32;
}
