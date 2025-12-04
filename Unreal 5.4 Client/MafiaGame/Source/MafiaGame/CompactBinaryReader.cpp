// Fill out your copyright notice in the Description page of Project Settings.


#include "CompactBinaryReader.h"
// 기본 생성자 (오브젝트 초기화)
CompactBinaryReader::CompactBinaryReader()
    : DataPtr(nullptr)
    , DataSize(0)
    , DataOffset(0)
{
}
// 내부 상태 초기화
void CompactBinaryReader::Clear()
{
    DataPtr = nullptr;
    DataSize = 0;
    DataOffset = 0;
}
// 내부 포인터 초기화
void CompactBinaryReader::Init(const uint8* PayloadPtr, SIZE_T Size)
{
    DataPtr = PayloadPtr;
    DataSize = Size;
    DataOffset = 0;
}


// 지정된 바이트만큼 읽을 수 있는지 확인
bool CompactBinaryReader::HasData(SIZE_T Bytes) const
{
    return DataOffset + Bytes <= DataSize;
}
// 단일 바이트 읽기
uint8 CompactBinaryReader::ReadUInt8()
{
    if (!HasData(1))
    {
        return 0;
    }
    
    return DataPtr[DataOffset++];
}
// 2바이트 unsigned 읽기 (리틀 엔디안)
uint16 CompactBinaryReader::ReadUInt16()
{
    if (!HasData(2))
    {
        return 0;
    }
    
    uint16 Result = static_cast<uint16>(DataPtr[DataOffset] |
                                       (DataPtr[DataOffset + 1] << 8));
    DataOffset += 2;
    return Result;
}
// 2바이트 signed 읽기 (리틀 엔디안)
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
// 4바이트 unsigned 읽기 (리틀 엔디안)
uint32 CompactBinaryReader::ReadUInt32()
{
    if (!HasData(4))
    {
        return 0;
    }
    
    uint32 Result = static_cast<uint32>(
        DataPtr[DataOffset] | 
        (DataPtr[DataOffset + 1] << 8) |
        (DataPtr[DataOffset + 2] << 16) | 
        (DataPtr[DataOffset + 3] << 24));
    DataOffset += 4;
    return Result;
}
// 4바이트 signed 읽기
int32 CompactBinaryReader::ReadInt32()
{
    return static_cast<int32>(ReadUInt32());
}
// 압축된 float 읽기 (정밀도 적용)
float CompactBinaryReader::ReadCompactFloat(float Precision)
{
    return static_cast<float>(ReadInt32()) * Precision;
}
// 문자열 읽기 (uint16 length + data)
// 길이 검사: 요청된 길이가 없거나 100초과이면 빈 문자열 반환
FString CompactBinaryReader::ReadString()
{
    if (!HasData(2))
    {
        return TEXT("");
    }
    
    uint16 Length = DataPtr[DataOffset] | (DataPtr[DataOffset + 1] << 8);
    DataOffset += 2;
    
    if (!HasData(Length))
    {
        return TEXT("");
    }
    
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
// 문자열 벡터 읽기 (uint8 count + 각 문자열)
TArray<FString> CompactBinaryReader::ReadStringArray()
{
    uint8 Count = ReadUInt8();
    TArray<FString> Result;
    Result.Reserve(Count);
    
    for (uint8 i = 0; i < Count; ++i)
    {
        Result.Add(ReadString());
    }
    
    return Result;
}
// 비트 플래그 읽기
uint8 CompactBinaryReader::ReadBitFlags()
{
    return ReadUInt8();
}
// bool 읽기
bool CompactBinaryReader::ReadBool()
{
    return ReadUInt8() != 0;
}