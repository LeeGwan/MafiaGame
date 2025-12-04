// 커널 드라이버 기반 안티치트 시스템
// ObRegisterCallbacks를 통한 프로세스 보호 및 하드웨어 지문 수집
#pragma once
#include"offset.h"
#include<thread>
#include <string>
#include <atomic>

enum class PacketType : uint8_t;  // 패킷 타입
struct IntegrityCheckPacket;       // 하드웨어 정보 패킷

class AntiCheat
{
public:
    AntiCheat();
    ~AntiCheat();

    // 안티치트 시스템 시작 (드라이버 등록, 시작, 연결)
    bool Start(const std::string hash);

    // 드라이버 연결 상태 확인
    bool IsConnected();

    // 드라이버 연결 종료 및 리소스 정리
    void Disconnect();

    // 하드웨어 정보 수집 및 서버 전송 (CPU ID, 메인보드 UUID)
    bool RequestHardwareInfo(PacketType type, const std::string& hash);

    // 보호 대상 프로세스 추가 (pid=0이면 현재 프로세스)
    bool AddProtectedPID(DWORD pid);

    // 서버 하트비트 검증 (드라이버 상태, SendHeartbeat 성공 여부)
    void ServerCheckLogic(PacketType type);

private:
    // 드라이버 서비스 등록 (CreateService)
    bool RegisterDriver(DWORD startType = SERVICE_DEMAND_START);

    // 공격 시도 이벤트 수집 (MAX_ALERTS개까지)
    bool GetSecurityAlerts();

    // 드라이버 시작 (StartService)
    bool StartDriver();

    // 드라이버와 통신 채널 생성 (CreateFileW)
    bool Connect();

    // 드라이버 상태 확인 (SERVICE_RUNNING 여부)
    bool GetDriverStatus();

    // 드라이버 중지 (ControlService)
    bool StopDriver();

    // 이벤트 루프 (공격 시도 모니터링)
    void EventLoop();

    // 하드웨어 정보 응답 파싱 (MESSAGE_HEADER + MESSAGE_FIELD)
    bool ParseHardwareResponse(PUCHAR buffer, DWORD bufferSize, IntegrityCheckPacket& Packet);

    // ObRegisterCallbacks 활성화 (IOCTL_SECURITY_CONTROL)
    bool EnableProtection();

    // 드라이버 하트비트 전송 (0x12345678 응답 확인)
    bool SendHeartbeat();

private:
    SC_HANDLE hSCManager;        // 서비스 관리자 핸들
    SC_HANDLE hService;          // 드라이버 서비스 핸들
    std::wstring serviceName;    // 서비스 이름 ("Flect")
    std::wstring displayName;    // 표시 이름 ("Flect Simple Anticheat")
    std::wstring driverPath;     // 드라이버 파일 경로 (.sys)

    HANDLE m_hDevice;                   // 드라이버 디바이스 핸들
    std::atomic<bool> m_isConnected;    // 드라이버 연결 상태
    std::atomic<bool> m_shouldStop;     // 이벤트 루프 종료 플래그
    std::thread m_EventThread;          // 이벤트 루프 스레드
    std::string copy_hash;              // 해시 복사본
    const std::wstring DEVICE_PATH = L"\\\\.\\Flect";  // 드라이버 디바이스 경로
};

// 전역 싱글톤 인스턴스
extern std::unique_ptr<AntiCheat> G_AntiCheat;