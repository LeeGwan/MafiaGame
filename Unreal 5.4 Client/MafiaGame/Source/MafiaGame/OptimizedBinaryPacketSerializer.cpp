// Fill out your copyright notice in the Description page of Project Settings.


#include "OptimizedBinaryPacketSerializer.h"
#include "CompactBinaryReader.h"
#include "PacketStructure.h"
#include "XXHash32.h"
// SecurePacketHeader를 구성하고 XXHash를 계산해 헤더에 채워 넣음
void OptimizedBinaryPacketSerializer::WrapPacketWithXXHash(uint8 type, TArray<uint8>* data, SIZE_T size)
{
    SecurePacketHeader header;
    header.magic = MAGIC_NUMBER;
    header.version = VERSION;
    header.type = type;
    header.data_size = static_cast<uint32>(size - sizeof(SecurePacketHeader));
    header.xxhash = XXHash32::hash(data->GetData() + sizeof(SecurePacketHeader), header.data_size);
    FMemory::Memcpy(data->GetData(), &header, sizeof(SecurePacketHeader));
}
// 버퍼에 단일 바이트를 쓰고 오프셋 증가
void OptimizedBinaryPacketSerializer::push(TArray<uint8>* buffer, uint8 value, SIZE_T& offset)
{
    (*buffer)[offset] = value;
    offset += 1;
}
// 문자열 직렬화: 길이(uint16_t) + 데이터
void OptimizedBinaryPacketSerializer::SerializeString(TArray<uint8>* buffer, const FString& str, SIZE_T& offset)
{
    FTCHARToUTF8 Convert(*str);
    uint16 length = static_cast<uint16>(Convert.Length());
    push(buffer, length & 0xff, offset);
    push(buffer, (length >> 8) & 0xff, offset);
    
    const char* CharArray = Convert.Get();
    for (int32 i = 0; i < Convert.Length(); ++i)
    {
        push(buffer, static_cast<uint8>(CharArray[i]), offset);
    }
}
// uint8 직렬화
void OptimizedBinaryPacketSerializer::SerializeUInt8(TArray<uint8>* buffer, uint8 value, SIZE_T& offset)
{
    push(buffer, value, offset);
}
// int16 직렬화 (리틀 엔디안)
void OptimizedBinaryPacketSerializer::SerializeInt16(TArray<uint8>* buffer, int16 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
}
// uint16 직렬화 (리틀 엔디안)
void OptimizedBinaryPacketSerializer::SerializeUInt16(TArray<uint8>* buffer, uint16 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
}
// int32 직렬화 (리틀 엔디안)
void OptimizedBinaryPacketSerializer::SerializeInt32(TArray<uint8>* buffer, int32 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
    push(buffer, (value >> 16) & 0xff, offset);
    push(buffer, (value >> 24) & 0xff, offset);
}
// uint32 직렬화 (리틀 엔디안)
void OptimizedBinaryPacketSerializer::SerializeUInt32(TArray<uint8>* buffer, uint32 value, SIZE_T& offset)
{
    push(buffer, value & 0xff, offset);
    push(buffer, (value >> 8) & 0xff, offset);
    push(buffer, (value >> 16) & 0xff, offset);
    push(buffer, (value >> 24) & 0xff, offset);
}
// float 직렬화 (비트 동일 저장)
void OptimizedBinaryPacketSerializer::SerializeFloat(TArray<uint8>* buffer, float value, SIZE_T& offset)
{
    uint32 floatAsInt = *reinterpret_cast<uint32*>(&value);
    SerializeInt32(buffer, static_cast<int32>(floatAsInt), offset);
}
// bool 직렬화
void OptimizedBinaryPacketSerializer::SerializeBool(TArray<uint8>* buffer, bool value, SIZE_T& offset)
{
    push(buffer, value ? 1 : 0, offset);
}
// 비트 플래그 직렬화
void OptimizedBinaryPacketSerializer::SerializeBitFlags(TArray<uint8>* buffer, uint8 flags, SIZE_T& offset)
{
    push(buffer, flags ? 1 : 0, offset);
}
// 문자열 벡터 직렬화: 개수(uint8) + 각 문자열
void OptimizedBinaryPacketSerializer::SerializeStringVector(TArray<uint8>* buffer, const TArray<FString>& vec, SIZE_T& offset)
{
    SerializeUInt8(buffer, static_cast<uint8>(vec.Num()), offset);
    for (const auto& str : vec)
    {
        SerializeString(buffer, str, offset);
    }
}
// 패킷 타입 변경: SecurePacketHeader의 type 필드만 변경
void OptimizedBinaryPacketSerializer::ChangePacket(TArray<uint8>& data, PacketType Change_type)
{
    SecurePacketHeader* header = reinterpret_cast<SecurePacketHeader*>(data.GetData());
    header->type = static_cast<uint8>(Change_type);
}
// SecurePacket 파싱 및 무결성 검증(매직, 버전, 사이즈, XXHash)
bool OptimizedBinaryPacketSerializer::ParseSecurePacket(const TArray<uint8>& data, PacketType& out_type, CompactBinaryReader* out_reader)
{
    if (data.Num() < sizeof(SecurePacketHeader))
    {
        return false;
    }

    const SecurePacketHeader* header = reinterpret_cast<const SecurePacketHeader*>(data.GetData());

    if (header->magic != MAGIC_NUMBER || header->version != VERSION)
    {
        return false;
    }

    if (data.Num() < sizeof(SecurePacketHeader) + header->data_size)
    {
        return false;
    }

    const uint8* payload_ptr = data.GetData() + sizeof(SecurePacketHeader);
    SIZE_T payload_size = header->data_size;

    uint32 calculated_hash = XXHash32::hash(payload_ptr, payload_size);

    if (calculated_hash != header->xxhash)
    {
        return false;
    }

    if (static_cast<PacketType>(header->type) >= PacketType::MaxPacketSize)
    {
        return false;
    }
    
    out_reader->Init(payload_ptr, payload_size);
    out_type = static_cast<PacketType>(header->type);
    
    return true;
}
// FUserAuthData 역직렬화
template <>
void OptimizedBinaryPacketSerializer::DeserializePacket<FUserAuthData>(CompactBinaryReader& reader, FUserAuthData& data)
{
    data.hash=reader.ReadStringArray();
    return;
}
