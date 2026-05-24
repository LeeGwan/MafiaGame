// Fill out your copyright notice in the Description page of Project Settings.

#include "CompactBinaryReader.h"

/**
 * @brief Default constructor. Initializes the reader state to null.
 */
CompactBinaryReader::CompactBinaryReader()
    : DataPtr(nullptr)
    , DataSize(0)
    , DataOffset(0)
{
}

/**
 * @brief Resets the internal state and invalidates the data pointer.
 */
void CompactBinaryReader::Clear()
{
    DataPtr = nullptr;
    DataSize = 0;
    DataOffset = 0;
}

/**
 * @brief Initializes the reader with a payload and its size.
 * @param PayloadPtr Pointer to the binary data buffer.
 * @param Size Total size of the buffer in bytes.
 */
void CompactBinaryReader::Init(const uint8* PayloadPtr, SIZE_T Size)
{
    DataPtr = PayloadPtr;
    DataSize = Size;
    DataOffset = 0;
}

/**
 * @brief Validates if the requested number of bytes can be read from the current offset.
 * @param Bytes Number of bytes to check.
 * @return True if data is available, false if it would cause an overflow.
 */
bool CompactBinaryReader::HasData(SIZE_T Bytes) const
{
    return DataOffset + Bytes <= DataSize;
}

/**
 * @brief Reads a single unsigned byte (8-bit) from the buffer.
 */
uint8 CompactBinaryReader::ReadUInt8()
{
    if (!HasData(1))
    {
        return 0;
    }
    
    return DataPtr[DataOffset++];
}

/**
 * @brief Reads a 16-bit unsigned integer (Little-Endian).
 */
uint16 CompactBinaryReader::ReadUInt16()
{
    if (!HasData(2))
    {
        return 0;
    }
    
    // Explicit Little-Endian reconstruction
    uint16 Result = static_cast<uint16>(DataPtr[DataOffset] |
                                        (DataPtr[DataOffset + 1] << 8));
    DataOffset += 2;
    return Result;
}

/**
 * @brief Reads a 16-bit signed integer (Little-Endian).
 */
int16 CompactBinaryReader::ReadInt16()
{
    if (!HasData(2))
    {
        return 0;
    }
    
    int16 Result = static_cast<int16>(DataPtr[DataOffset] |
                                     (DataPtr[DataOffset + 1] << 8));
    DataOffset += 2;
    return Result;
}

/**
 * @brief Reads a 32-bit unsigned integer (Little-Endian).
 */
uint32 CompactBinaryReader::ReadUInt32()
{
    if (!HasData(4))
    {
        return 0;
    }
    
    // Reconstruct 32-bit value from bytes to ensure cross-platform compatibility
    uint32 Result = static_cast<uint32>(
        DataPtr[DataOffset] | 
        (DataPtr[DataOffset + 1] << 8) |
        (DataPtr[DataOffset + 2] << 16) | 
        (DataPtr[DataOffset + 3] << 24));
    DataOffset += 4;
    return Result;
}

/**
 * @brief Reads a 32-bit signed integer.
 */
int32 CompactBinaryReader::ReadInt32()
{
    return static_cast<int32>(ReadUInt32());
}

/**
 * @brief Decompresses and reads a float value using a predefined precision multiplier.
 * @param Precision The scaling factor used to restore the float from an integer.
 */
float CompactBinaryReader::ReadCompactFloat(float Precision)
{
    return static_cast<float>(ReadInt32()) * Precision;
}

/**
 * @brief Reads a string prefixed with a 16-bit length.
 * Includes security checks for maximum length and buffer boundaries.
 * @return FString containing the read data, or empty string on failure.
 */
FString CompactBinaryReader::ReadString()
{
    // Check if length prefix is available
    if (!HasData(2))
    {
        return TEXT("");
    }
    
    uint16 Length = DataPtr[DataOffset] | (DataPtr[DataOffset + 1] << 8);
    DataOffset += 2;
    
    // Validate string body size against remaining data
    if (!HasData(Length))
    {
        return TEXT("");
    }
    
    // Anti-Spam/DoS check: Limit maximum string length
    if (Length > 100)
    {
        UE_LOG(LogTemp, Warning, TEXT("String length exceeds maximum allowed: %d"), Length);
        return TEXT("");
    }

    FString Result;
    Result.Reserve(Length + 1);
    
    for (uint16 i = 0; i < Length; ++i)
    {
        Result.AppendChar(static_cast<TCHAR>(DataPtr[DataOffset + i]));
    }
    DataOffset += Length;

    return Result;
}

/**
 * @brief Reads an array of strings, prefixed with an 8-bit element count.
 */
TArray<FString> CompactBinaryReader::ReadStringArray()
{
    uint8 Count = ReadUInt8();
    TArray<FString> Result;
    Result.Reserve(Count);
    
    for (uint8 i =
