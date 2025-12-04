// UI 타입 정의 (화면 전환용)
#pragma once
#include<cstdint>

enum class EUIType : uint8_t
{
    Init,       // 초기 화면 (로그인/회원가입 선택)
    Register,   // 회원가입 화면
    Login,      // 로그인 화면
    Lobby,      // 로비 화면 (게임 찾기)
    Matching,   // 매칭 대기 화면
    Game        // 게임 진행 화면
};