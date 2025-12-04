// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class MAFIAGAME_API CompactBinaryReader
{
private:
	const uint8* DataPtr;
	SIZE_T DataSize;
	SIZE_T DataOffset;

public:
	CompactBinaryReader();
	// 페이로드 초기화
	void Init(const uint8* PayloadPtr, SIZE_T Size);
	// 주어진 바이트를 읽을 수 있는지 확인
	bool HasData(SIZE_T Bytes) const;
	// 초기화 해제
	void Clear();
	
    //데이터 오프셋 읽기
	FORCEINLINE SIZE_T GetDataOffset() const { return DataOffset; }
	//데이터 사이즈 읽기
	FORCEINLINE SIZE_T GetDataSize() const { return DataSize; }
	
	// 읽기 함수들
	uint8 ReadUInt8();
	int16 ReadInt16();
	uint16 ReadUInt16();
	uint32 ReadUInt32();
	int32 ReadInt32();
	float ReadCompactFloat(float Precision = 0.01f);
	FString ReadString();
	TArray<FString> ReadStringArray();
	uint8 ReadBitFlags();
	bool ReadBool();
};
