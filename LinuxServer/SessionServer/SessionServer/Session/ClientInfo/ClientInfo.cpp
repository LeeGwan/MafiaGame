#include "ClientInfo.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

ClientInfo::ClientInfo() {}

ClientInfo::~ClientInfo() {}

// 로그인 시간 등록
void ClientInfo::Register() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    // 시간 포맷팅 (YYYY-MM-DD HH:MM:SS)
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    Logintime = ss.str();
}

// 하드웨어 정보 등록 (안티치트용: 메인보드 UUID, CPU ID)
void ClientInfo::RegisterInfo(const std::string In_MainboardUUID,
    const std::string& In_CPUID) {
    MainboardUUID = In_MainboardUUID;
    CPUID = In_CPUID;
}

// 정보 초기화
void ClientInfo::Release() { Logintime.clear(); }