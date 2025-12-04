// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 간단한 XXHash32 구현 헤더
class MAFIAGAME_API XXHash32
{
private:
	// XXHash 알고리즘 상수
	static constexpr uint32 PRIME32_1 = 0x9E3779B1U;
	static constexpr uint32 PRIME32_2 = 0x85EBCA77U;
	static constexpr uint32 PRIME32_3 = 0xC2B2AE3DU;
	static constexpr uint32 PRIME32_4 = 0x27D4EB2FU;
	static constexpr uint32 PRIME32_5 = 0x165667B1U;
	// 32비트 좌회전
	FORCEINLINE static uint32 Rotl32(uint32 X, int32 R)
	{
		return (X << R) | (X >> (32 - R));
	}
	// 메모리에서 32비트 읽기 
	FORCEINLINE static uint32 Read32(const uint8* Ptr)
	{
		return *reinterpret_cast<const uint32*>(Ptr);
	}
public:
	// XXHash32 해시 계산
	static uint32 hash(const void* Input, SIZE_T Length, uint32 Seed = 0);

	// 문자열 해시 계산
	FORCEINLINE static uint32 HashString(const FString& String, uint32 Seed = 0)
	{
		return hash(TCHAR_TO_UTF8(*String), String.Len(), Seed);
	}
	// 배열 기반 해시 계산
	template<typename T>
   FORCEINLINE static uint32 HashArray(const TArray<T>& Array, uint32 Seed = 0)
	{
		return hash(Array.GetData(), Array.Num() * sizeof(T), Seed);
	}
};
