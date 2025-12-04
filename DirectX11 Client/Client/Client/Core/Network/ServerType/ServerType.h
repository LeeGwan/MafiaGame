#pragma once
#include<cstdint>
// 서버 상태 타입
enum class ServerType : uint8_t
{
    WAIT,
    ROUTINEAUTHSERVER,
    AUTHSERVER,
    GAMELOBBYSERVER
};