// 안티치트 시스템 구현부
#include "AntiCheat.h"
#include <fstream>
#include <filesystem>
#include "../Network/Packet/PacketStructure/PacketStructure.h"
#include "../Network/Packet/RoutineProgress/RoutineProgress.h"
#include "../Utils/UtilsForString.h"

// 전역 싱글톤 인스턴스 생성
std::unique_ptr<AntiCheat> G_AntiCheat = std::make_unique<AntiCheat>();

// 생성자: 멤버 변수 초기화
AntiCheat::AntiCheat() : m_hDevice(INVALID_HANDLE_VALUE), m_isConnected(false), m_shouldStop(false) {
	serviceName = L"Flect";  // 서비스 이름
	displayName = L"Flect Anticheat";  // 표시 이름

	// 현재 실행 파일 디렉토리에서 드라이버 경로 설정
	std::filesystem::path currentPath = std::filesystem::current_path();
	driverPath = currentPath.wstring() + L"\\Anticheat\\AntiCheat.sys";
}

AntiCheat::~AntiCheat()
{
	// 소멸자에서 드라이버 연결 종료
	Disconnect();
}

// 드라이버 연결 상태 확인
bool AntiCheat::IsConnected()
{
	return m_isConnected.load();
}

// 드라이버와 통신 채널 생성
bool AntiCheat::Connect()
{
	// 드라이버 상태 확인
	if (!GetDriverStatus())return false;

	// CreateFileW로 드라이버 디바이스 열기
	m_hDevice = CreateFileW(
		DEVICE_PATH.c_str(),        // 디바이스 경로 ("\\\\.\\Flect")
		GENERIC_READ | GENERIC_WRITE,  // 읽기/쓰기 권한
		0,                          // 공유 안 함
		nullptr,
		OPEN_EXISTING,              // 기존 디바이스 열기
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	// 디바이스 열기 실패
	if (m_hDevice == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// ObRegisterCallbacks 활성화
	if (!EnableProtection())return false;

	// 현재 프로세스를 보호 대상으로 추가
	if (!AddProtectedPID(0))return false;

	// 연결 상태 플래그 설정
	m_isConnected.store(true);
	m_shouldStop.store(false);

	// 이벤트 루프 스레드 시작 (공격 시도 모니터링)
	m_EventThread = std::thread(&AntiCheat::EventLoop, this);
	return true;
}

// 드라이버 중지
bool AntiCheat::StopDriver()
{
	if (!hService) {
		return false;
	}

	SERVICE_STATUS serviceStatus;

	// ControlService로 드라이버 중지 요청
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus)) {
		DWORD error = GetLastError();

		// 이미 중지된 경우 성공으로 처리
		if (error == ERROR_SERVICE_NOT_ACTIVE) {
			return true;
		}
		else {
			return false;
		}
	}

	return true;
}

// 하드웨어 정보 수집 및 서버 전송
bool AntiCheat::RequestHardwareInfo(PacketType type, const std::string& hash)
{
	// 드라이버 연결 및 상태 확인
	if (!m_isConnected.load() || !GetDriverStatus()) {
		return false;
	}

	IntegrityCheckPacket Packet;
	HARDWARE_REQUEST request;
	request.RequestType = HW_REQUEST_ALL;  // 모든 하드웨어 정보 요청
	request.Reserved = 0;
	Packet.Type = type;
	Packet.hash = hash;

	UCHAR responseBuffer[4096] = { 0 };  // 응답 버퍼
	DWORD bytesReturned = 0;

	// DeviceIoControl로 하드웨어 정보 요청
	BOOL result = DeviceIoControl(
		m_hDevice,
		IOCTL_HARDWARE_GET_INFO,  // 하드웨어 정보 수집 IOCTL
		&request,
		sizeof(request),
		responseBuffer,
		sizeof(responseBuffer),
		&bytesReturned,
		nullptr
	);

	if (!result) {
		return false;
	}

	// 응답 파싱 (CPU ID, 메인보드 UUID 추출)
	if (!ParseHardwareResponse(responseBuffer, bytesReturned, Packet))return false;

	// CPU ID 유효성 검사
	if (Packet.CPU_ID.empty())
	{

	}

	// 메인보드 ID 유효성 검사
	if (Packet.Mainboard_ID.empty())
	{

	}

	// 서버로 하드웨어 정보 전송
	G_Routine->SendResponseForIntegrityCheckPacket(Packet);
	return true;
}

// 하드웨어 정보 응답 파싱
bool AntiCheat::ParseHardwareResponse(PUCHAR buffer, DWORD bufferSize, IntegrityCheckPacket& Packet) {
	// 최소 헤더 크기 확인
	if (bufferSize < sizeof(MESSAGE_HEADER)) {
		return false;
	}

	// 메시지 헤더 파싱
	PMESSAGE_HEADER header = reinterpret_cast<PMESSAGE_HEADER>(buffer);

	// 메시지 타입 확인
	if (header->MessageType != MSG_TYPE_HARDWARE_RESPONSE) {
		return false;
	}

	// 필드 시작 위치
	PUCHAR currentPtr = buffer + sizeof(MESSAGE_HEADER);
	ULONG remainingSize = bufferSize - sizeof(MESSAGE_HEADER);

	// 각 필드 순회
	for (USHORT i = 0; i < header->FieldCount; i++) {
		// 필드 헤더 크기 확인
		if (remainingSize < sizeof(MESSAGE_FIELD) - 1) {
			break;
		}

		PMESSAGE_FIELD field = reinterpret_cast<PMESSAGE_FIELD>(currentPtr);

		// 필드 전체 크기 확인
		if (remainingSize < sizeof(MESSAGE_FIELD) - 1 + field->DataSize) {
			break;
		}

		// 필드 데이터를 문자열로 변환
		std::string fieldData(reinterpret_cast<char*>(field->Data), field->DataSize);

		// 필드 ID에 따라 분기 처리
		switch (field->FieldId) {
		case HW_REQUEST_MAINBOARD:
			Packet.Mainboard_ID = fieldData;  // 메인보드 UUID
			break;
		case HW_REQUEST_CPU:
			Packet.CPU_ID = fieldData;  // CPU 시리얼
			break;
		default:
			break;
		}

		// 다음 필드로 이동
		ULONG fieldSize = sizeof(MESSAGE_FIELD) - 1 + field->DataSize;
		currentPtr += fieldSize;
		remainingSize -= fieldSize;
	}

	return true;
}

// ObRegisterCallbacks 활성화
bool AntiCheat::EnableProtection()
{
	PROTECTION_REQUEST request;
	request.RequestType = SECURITY_REQUEST_OB_REGISTER;  // ObRegisterCallbacks 등록
	request.ProcessId = 0;
	request.Reserved = 0;

	DWORD bytesReturned = 0;

	// DeviceIoControl로 보호 활성화 요청
	BOOL result = DeviceIoControl(
		m_hDevice,
		IOCTL_SECURITY_CONTROL,
		&request,
		sizeof(request),
		nullptr,
		0,
		&bytesReturned,
		nullptr
	);
	return result;
}

// 보호 대상 프로세스 추가
bool AntiCheat::AddProtectedPID(DWORD pid)
{
	// 드라이버 상태 확인
	if (!GetDriverStatus()) {
		return false;
	}

	// pid=0이면 현재 프로세스
	if (pid == 0)
	{
		pid = GetCurrentProcessId();
	}

	PROTECTION_REQUEST request;
	request.RequestType = SECURITY_REQUEST_ADD_PID;  // PID 추가 요청
	request.ProcessId = (HANDLE)(ULONG_PTR)pid;
	request.Reserved = 0;

	DWORD bytesReturned = 0;

	// DeviceIoControl로 PID 추가 요청
	BOOL result = DeviceIoControl(
		m_hDevice,
		IOCTL_SECURITY_CONTROL,
		&request,
		sizeof(request),
		nullptr,
		0,
		&bytesReturned,
		nullptr
	);

	return result;
}

// 이벤트 루프 (공격 시도 모니터링)
void AntiCheat::EventLoop()
{
	// m_shouldStop이 true가 될 때까지 반복
	while (!m_shouldStop.load())
	{
		// 공격 시도 로그 수집
		GetSecurityAlerts();
	}
}

// 공격 시도 이벤트 수집
bool AntiCheat::GetSecurityAlerts()
{
	// 드라이버 연결 및 상태 확인
	if (!m_isConnected.load() || !GetDriverStatus()) {
		return false;
	}

	PROTECTION_REQUEST request;
	request.RequestType = SECURITY_REQUEST_GET_ALERTS;  // 공격 시도 로그 요청
	request.ProcessId = 0;
	request.Reserved = 0;

	SECURITY_ALERT alerts[MAX_ALERTS];  // 최대 100개의 경고 저장
	DWORD bytesReturned = 0;

	// DeviceIoControl로 공격 시도 로그 수집
	BOOL result = DeviceIoControl(
		m_hDevice,
		IOCTL_SECURITY_CONTROL,
		&request,
		sizeof(request),
		alerts,
		sizeof(alerts),
		&bytesReturned,
		nullptr
	);

	if (result) {
		stringforVectorPacket packet;
		packet.Type = PacketType::ANTI_EVENT_REQUEST;
		packet.hash = copy_hash;
		int alertCount = 0;

		// 유효한 공격 시도 로그 수집
		for (int i = 0; i < MAX_ALERTS; i++) {
			if (alerts[i].AttackerPID != 0) {
				// 공격자 프로세스 이름 추가
				packet.str.push_back(alerts[i].AttackerName);
				alertCount++;
			}
		}

		// 서버로 공격 시도 로그 전송
		G_Routine->SendResponseForstringforVectorPacket(packet);
		return true;
	}
	else {
		DWORD error = GetLastError();

		// 더 이상 항목이 없는 경우 성공으로 처리
		if (error == ERROR_NO_MORE_ITEMS) {
			return true;
		}
	}
	return false;
}

// 서버 하트비트 검증
void AntiCheat::ServerCheckLogic(PacketType type)
{
	ResultPacket packet;
	packet.Type = type;

	// 드라이버 상태, SendHeartbeat, 연결 상태, 핸들 유효성 검사
	if (!GetDriverStatus() || !SendHeartbeat() || !m_isConnected.load() || m_hDevice == INVALID_HANDLE_VALUE)
	{
		packet.ResultTypes = ResultType::Flect_Not_Running;  // 드라이버 비정상
	}
	else
	{
		packet.ResultTypes = ResultType::Flect_Running;  // 드라이버 정상
	}

	// 서버로 하트비트 응답 전송
	G_Routine->SendResponseForstringforResultPacket(packet);
}

// 드라이버 하트비트 전송
bool AntiCheat::SendHeartbeat()
{
	// 연결 상태 및 핸들 유효성 검사
	if (!m_isConnected.load() || m_hDevice == INVALID_HANDLE_VALUE || !GetDriverStatus()) {
		return false;
	}

	const ULONG EXPECTED_HEARTBEAT = 0x12345678;  // 예상 응답 값 (테스트용)
	ULONG heartbeatResponse = 0;
	DWORD bytesReturned = 0;

	// DeviceIoControl로 하트비트 전송
	BOOL result = DeviceIoControl(
		m_hDevice,
		IOCTL_HARDWARE_HEARTBEAT,
		nullptr,
		0,
		&heartbeatResponse,
		sizeof(heartbeatResponse),
		&bytesReturned,
		nullptr
	);

	if (!result)
	{
		return false;
	}

	// 응답 크기 확인
	if (bytesReturned != sizeof(ULONG)) {
		return false;
	}

	// 응답 값 검증 (드라이버 무결성 확인)
	if (heartbeatResponse != EXPECTED_HEARTBEAT) {
		return false;
	}

	return true;
}

// 드라이버 상태 확인
bool AntiCheat::GetDriverStatus()
{
	DWORD currentState;
	if (!hService) {
		return false;
	}

	SERVICE_STATUS status;

	// QueryServiceStatus로 드라이버 상태 조회
	if (!QueryServiceStatus(hService, &status)) {
		return false;
	}

	currentState = status.dwCurrentState;

	// SERVICE_START_PENDING 또는 SERVICE_RUNNING이면 정상
	if (currentState == SERVICE_START_PENDING || currentState == SERVICE_RUNNING)
	{
		return true;
	}
	return false;
}

// 안티치트 시스템 시작
bool AntiCheat::Start(const std::string hash)
{
	// 해시 유효성 검사
	if (hash.empty() || !UtilsForString::IsValidHash(hash))return false;

	// 세션 해시 복사
	copy_hash = hash;

	// 드라이버 서비스 등록
	if (!RegisterDriver())
	{
		return false;
	}

	// 드라이버 시작
	if (!StartDriver())
	{
		return false;
	}

	// 드라이버와 통신 채널 생성
	if (!Connect())
	{
		return false;
	}
	return true;
}

// 드라이버 연결 종료 및 리소스 정리
void AntiCheat::Disconnect()
{
	// 연결 상태 확인
	if (!m_isConnected.load())return;

	// 이벤트 루프 종료 플래그 설정
	m_shouldStop.store(true);

	// 이벤트 루프 스레드 종료 대기
	if (m_EventThread.joinable()) {
		m_EventThread.join();
	}

	// 드라이버 핸들 닫기
	if (m_hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hDevice);
		m_hDevice = INVALID_HANDLE_VALUE;
	}

	// 연결 상태 플래그 초기화
	m_isConnected.store(false);

	// 드라이버 중지
	StopDriver();

	// 드라이버 서비스 삭제
	if (hService) {
		DeleteService(hService);
		CloseServiceHandle(hService);
		hService = NULL;
	}

	// 서비스 관리자 핸들 닫기
	if (hSCManager) {
		CloseServiceHandle(hSCManager);
		hSCManager = NULL;
	}
}

// 드라이버 서비스 등록
bool AntiCheat::RegisterDriver(DWORD startType)
{
	// 서비스 관리자 열기
	hSCManager = OpenSCManager(
		NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (!hSCManager)
	{
		return false;
	}

	// 드라이버 서비스 생성
	hService = CreateService(
		hSCManager,
		serviceName.c_str(),           // 서비스 이름 ("Flect")
		displayName.c_str(),            // 표시 이름
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,          // 커널 드라이버
		startType,                      // 시작 타입 (SERVICE_DEMAND_START)
		SERVICE_ERROR_NORMAL,
		driverPath.c_str(),             // 드라이버 파일 경로
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (!hService) {
		DWORD error = GetLastError();

		// 이미 존재하는 경우 서비스 열기
		if (error == ERROR_SERVICE_EXISTS) {
			hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_ALL_ACCESS);
			if (!hService) {
				return false;
			}
			return true;
		}
		else {
			return false;
		}
	}

	return true;
}

// 드라이버 시작
bool AntiCheat::StartDriver()
{
	if (!hService) {
		return false;
	}

	// StartService로 드라이버 시작
	if (!StartService(hService, 0, NULL)) {
		DWORD error = GetLastError();

		// 이미 실행 중인 경우 성공으로 처리
		if (error == ERROR_SERVICE_ALREADY_RUNNING) {
			return true;
		}
		else {
			return false;
		}
	}

	return true;
}