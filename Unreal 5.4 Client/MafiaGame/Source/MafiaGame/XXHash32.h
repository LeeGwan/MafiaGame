// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @class XXHash32
 * @brief A lightweight and high-performance implementation of the XXHash32 algorithm.
 * Optimized for rapid data identification and integrity verification within the MafiaGame framework.
 */
class MAFIAGAME_API XXHash32
{
private:
    /** XXHash internal constants (Primes) used to maintain high dispersion and low collision rates. */
    static constexpr uint32 PRIME32_1 = 0x9E3779B1U;
    static constexpr uint32 PRIME32_2 = 0x85EBCA77U;
    static constexpr uint32 PRIME32_3 = 0xC2B2AE3DU;
    static constexpr uint32 PRIME32_4 = 0x27D4EB2FU;
    static constexpr uint32 PRIME32_5 = 0x165667B1U;

    /**
     * @brief Performs a 32-bit left circular shift (Bitwise Rotation).
     * @param X The value to rotate.
     * @param R The number of bits to shift.
     * @return The rotated 32-bit value.
     */
    FORCEINLINE static uint32 Rotl32(uint32 X, int32 R)
    {
        return (X << R) | (X >> (32 - R));
    }

    /**
     * @brief Reads a 32-bit unsigned integer directly from raw memory.
     * Caution: This assumes the input pointer is valid and properly aligned for the target architecture.
     */
    FORCEINLINE static uint32 Read32(const uint8* Ptr)
    {
        return *reinterpret_cast<const uint32*>(Ptr);
    }

public:
    /**
     * @brief Core hashing function for raw memory buffers.
     * @param Input Pointer to the source data.
     * @param Length Size of the data in bytes.
     * @param Seed Optional seed value for hash randomization.
     * @return Resulting 32-bit hash.
     */
    static uint32 hash(const void* Input, SIZE_T Length, uint32 Seed = 0);

    /**
     * @brief Helper function to calculate a hash for an Unreal Engine FString.
     * Automatically converts the string to a UTF-8 byte stream before hashing.
     */
    FORCEINLINE static uint32 HashString(const FString& String, uint32 Seed = 0)
    {
        return hash(TCHAR_TO_UTF8(*String), String.Len(), Seed);
    }

    /**
     * @brief Template helper for hashing Unreal Engine TArray containers.
     * @tparam T The element type within the array.
     * @param Array The source TArray to hash.
     * @param Seed Optional seed value.
     * @return Hash of the raw contiguous memory held by the TArray.
     */
    template<typename T>
    FORCEINLINE static uint32 HashArray(const TArray<T>& Array, uint32 Seed = 0)
    {
        return hash(Array.GetData(), Array.Num() * sizeof(T), Seed);
    }
};
