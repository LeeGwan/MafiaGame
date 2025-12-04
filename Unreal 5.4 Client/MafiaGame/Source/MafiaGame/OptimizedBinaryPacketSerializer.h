// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
enum class PacketType : uint8;
struct TypePacket;
struct TwoStringPacket ;
struct ResultPacket;
struct ResultAndHashPacket;
struct HashPacket;
struct ServerInfoPacket;
class CompactBinaryReader;
// 바이너리 직렬화 최적화 처리 클래스
class MAFIAGAME_API OptimizedBinaryPacketSerializer
{
public:
	static const uint16 MAGIC_NUMBER = 0x2340; // 패킷 유효성 검증 값
	static const uint8 VERSION = 2;            // 패킷 버전
	// XXHash 포함 패킷 헤더 구조
	struct SecurePacketHeader
	{
		uint16 magic;
		uint8 version;
		uint8 type;
		uint32 data_size;
		uint32 xxhash;
	};
	// 패킷 헤더에 XXHash 래핑
	static void WrapPacketWithXXHash(uint8 type, TArray<uint8>* data, SIZE_T size);
	// 바이트 추가
	static void push(TArray<uint8>* buffer, uint8 value, SIZE_T& offset);
    
	// 기본 데이터 타입 직렬화 함수들
	static void SerializeString(TArray<uint8>* buffer, const FString& str, SIZE_T& offset);
	static void SerializeUInt8(TArray<uint8>* buffer, uint8 value, SIZE_T& offset);
	static void SerializeInt16(TArray<uint8>* buffer, int16 value, SIZE_T& offset);
	static void SerializeUInt16(TArray<uint8>* buffer, uint16 value, SIZE_T& offset);
	static void SerializeInt32(TArray<uint8>* buffer, int32 value, SIZE_T& offset);
	static void SerializeUInt32(TArray<uint8>* buffer, uint32 value, SIZE_T& offset);
	static void SerializeFloat(TArray<uint8>* buffer, float value, SIZE_T& offset);
	static void SerializeBool(TArray<uint8>* buffer, bool value, SIZE_T& offset);
	static void SerializeBitFlags(TArray<uint8>* buffer, uint8 flags, SIZE_T& offset);
	static void SerializeStringVector(TArray<uint8>* buffer, const TArray<FString>& vec, SIZE_T& offset);

	// 패킷 타입 변경
	static void ChangePacket(TArray<uint8>& data, PacketType Change_type);
	// 패킷 파싱
	static bool ParseSecurePacket(const TArray<uint8>& data, PacketType& out_type, CompactBinaryReader* out_reader);
    

	// 템플릿 구조체 <- 데이터 역직렬화
	template <typename T>
	static void DeserializePacket(CompactBinaryReader& reader, T& out);
};
