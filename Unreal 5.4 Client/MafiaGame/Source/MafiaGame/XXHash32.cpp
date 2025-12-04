// Fill out your copyright notice in the Description page of Project Settings.


#include "XXHash32.h"

// XXHash32 알고리즘 실행
uint32 XXHash32::hash(const void* Input, SIZE_T Length, uint32 Seed)
{
	const uint8* P = static_cast<const uint8*>(Input);
	const uint8* const BEnd = P + Length;
	uint32 H32;
	// 16바이트 이상 처리 루프
	if (Length >= 16)
	{
		const uint8* const Limit = BEnd - 16;
		// 초기 상태 설정
		uint32 V1 = Seed + PRIME32_1 + PRIME32_2;
		uint32 V2 = Seed + PRIME32_2;
		uint32 V3 = Seed + 0;
		uint32 V4 = Seed - PRIME32_1;
		// 16바이트 블록 단위 처리
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
		/ 4개 값 합산
		H32 = Rotl32(V1, 1) + Rotl32(V2, 7) + Rotl32(V3, 12) + Rotl32(V4, 18);
	}
	else
	{
		// 작은 입력 처리
		H32 = Seed + PRIME32_5;
	}

	H32 += static_cast<uint32>(Length);

	// 남은 4바이트씩 처리
	while (P + 4 <= BEnd)
	{
		H32 += Read32(P) * PRIME32_3;
		H32 = Rotl32(H32, 17) * PRIME32_4;
		P += 4;
	}

	// 나머지 바이트 처리
	while (P < BEnd)
	{
		H32 += (*P) * PRIME32_5;
		H32 = Rotl32(H32, 11) * PRIME32_1;
		P++;
	}

	// 최종 난수화 단계
	H32 ^= H32 >> 15;
	H32 *= PRIME32_2;
	H32 ^= H32 >> 13;
	H32 *= PRIME32_3;
	H32 ^= H32 >> 16;

	return H32;
}