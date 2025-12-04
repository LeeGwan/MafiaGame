
#pragma once
#include <cstdint>
#include <cstring>
// 간단한 XXHash32 구현 헤더
class XXHash32
{
private:
    static const uint32_t PRIME32_1 = 0x9E3779B1U;
    static const uint32_t PRIME32_2 = 0x85EBCA77U;
    static const uint32_t PRIME32_3 = 0xC2B2AE3DU;
    static const uint32_t PRIME32_4 = 0x27D4EB2FU;
    static const uint32_t PRIME32_5 = 0x165667B1U;

    static uint32_t rotl32(uint32_t x, int r);
    static uint32_t read32(const uint8_t* ptr);
public:
    // 입력 버퍼과 길이로부터 32비트 해시 계산
    static uint32_t hash(const void* input, size_t len, uint32_t seed = 0);
};
