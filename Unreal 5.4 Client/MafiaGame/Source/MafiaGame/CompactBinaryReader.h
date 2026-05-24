// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @class CompactBinaryReader
 * @brief A utility class for efficient sequential reading of binary data.
 * Designed for network packet parsing and compact data deserialization within the MafiaGame framework.
 */
class MAFIAGAME_API CompactBinaryReader
{
private:
    /** Pointer to the start of the binary data buffer */
    const uint8* DataPtr;
    
    /** Total size of the assigned data buffer */
    SIZE_T DataSize;
    
    /** Current read position (offset) within the buffer */
    SIZE_T DataOffset;

public:
    /** Default constructor. Initializes the reader to an empty state. */
    CompactBinaryReader();

    /**
     * @brief Initializes the reader with a data payload.
     * @param PayloadPtr Pointer to the binary source.
     * @param Size Total length of the source data.
     */
    void Init(const uint8* PayloadPtr, SIZE_T Size);

    /**
     * @brief Checks if the requested number of bytes can be safely read.
     * @param Bytes Number of bytes to validate.
     * @return True if sufficient data remains in the buffer.
     */
    bool HasData(SIZE_T Bytes) const;

    /**
     * @brief Clears the internal state and invalidates the data pointer.
     */
    void Clear();
    
    /** @brief Returns the current read offset. */
    FORCEINLINE SIZE_T GetDataOffset() const { return DataOffset; }
    
    /** @brief Returns the total size of the buffer. */
    FORCEINLINE SIZE_T GetDataSize() const { return DataSize; }
    
    /** * Serialization Methods
     * Implementations handle byte-order reconstruction and buffer boundary checks.
     */
    uint8 ReadUInt8();
    int16 ReadInt16();
    uint16 ReadUInt16();
    uint32 ReadUInt32();
    int32 ReadInt32();

    /**
     * @brief Reads a compressed float value by restoring it from a scaled integer.
     * @param Precision The multiplier used during original compression (Default: 0.01f).
     */
    float ReadCompactFloat(float Precision = 0.01f);

    /** @brief Reads an FString prefixed with a 16-bit length value. */
    FString ReadString();

    /** @brief Reads a TArray of FStrings prefixed with an 8-bit count. */
    TArray<FString> ReadStringArray();

    /** @brief Reads a single byte and treats it as a bit-field. */
    uint8 ReadBitFlags();

    /** @brief Reads a byte and returns true if it is non-zero. */
    bool ReadBool();
};
