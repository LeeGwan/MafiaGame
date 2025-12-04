#include "XXHash32.h"

// 32비트 좌회전
uint32_t XXHash32::rotl32(uint32_t x, int r)
{
	return (x << r) | (x >> (32 - r));
}
// 메모리에서 32비트 읽기 
uint32_t XXHash32::read32(const uint8_t* ptr)
{
	return *reinterpret_cast<const uint32_t*>(ptr);
}
// XXHash32 해시 계산 구현
uint32_t XXHash32::hash(const void* input, size_t len, uint32_t seed)
{
	const uint8_t* p = static_cast<const uint8_t*>(input);
	const uint8_t* const bEnd = p + len;
	uint32_t h32;

	if (len >= 16) {
		const uint8_t* const limit = bEnd - 16;
		uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
		uint32_t v2 = seed + PRIME32_2;
		uint32_t v3 = seed + 0;
		uint32_t v4 = seed - PRIME32_1;

		do {
			v1 = rotl32(v1 + read32(p) * PRIME32_2, 13) * PRIME32_1;
			p += 4;
			v2 = rotl32(v2 + read32(p) * PRIME32_2, 13) * PRIME32_1;
			p += 4;
			v3 = rotl32(v3 + read32(p) * PRIME32_2, 13) * PRIME32_1;
			p += 4;
			v4 = rotl32(v4 + read32(p) * PRIME32_2, 13) * PRIME32_1;
			p += 4;
		} while (p <= limit);

		h32 = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
	}
	else {
		h32 = seed + PRIME32_5;
	}

	h32 += static_cast<uint32_t>(len);

	while (p + 4 <= bEnd) {
		h32 += read32(p) * PRIME32_3;
		h32 = rotl32(h32, 17) * PRIME32_4;
		p += 4;
	}

	while (p < bEnd) {
		h32 += (*p) * PRIME32_5;
		h32 = rotl32(h32, 11) * PRIME32_1;
		p++;
	}

	h32 ^= h32 >> 15;
	h32 *= PRIME32_2;
	h32 ^= h32 >> 13;
	h32 *= PRIME32_3;
	h32 ^= h32 >> 16;

	return h32;
}