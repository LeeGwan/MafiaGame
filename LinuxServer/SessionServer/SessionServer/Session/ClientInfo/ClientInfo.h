#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#pragma once
#include <vector>
#include <string>

// 클라이언트 세션 정보 (로그인 시간, 하드웨어 정보)
class ClientInfo
{
public:
    ClientInfo();
    ~ClientInfo();
    void Register();
    void Release();

    // 하드웨어 정보 등록 (안티치트용)
    void RegisterInfo(const std::string In_MainboardUUID, const std::string& In_CPUID);

private:
    std::string Logintime; // 로그인 시간
    std::string MainboardUUID; // 메인보드 UUID (안티치트용)
    std::string CPUID; // CPU ID (안티치트용)
};

#endif