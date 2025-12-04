// 패킷 처리 엔진 구현부
#include "RoutineProgress.h"
#include"../../../Core/Core.h"
#include "../../../Event/EventManager/EventManager.h"
#include "../../../Event/EventType/EventType.h"
#include"../../../gui/EUIType/EUIType.h"
#include "../../Aes/Aes.h"
#include "../../Network/NetWork.h"
#include "../CompactBinaryReader/CompactBinaryReader.h"
#include "../OptimizedBinaryPacketSerializer/OptimizedBinaryPacketSerializer.h"
#include "../PacketStructure/PacketStructure.h"
#include"../../../gui/guicontrol/GuiControl.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

// 전역 싱글톤 인스턴스 생성 (Worker Thread 1개로 초기화)
std::unique_ptr<RoutineProgress> G_Routine =
std::make_unique<RoutineProgress>(1);

// Thread Pool 초기화
RoutineProgress::RoutineProgress(uint8_t in_threadcount)
	: threadcount(in_threadcount), ProsessThreads_status(true) {
	// Worker Thread 배열 메모리 예약 (동적 재할당 방지)
	ProsessThreads.reserve(threadcount);

	// Worker Thread 생성 및 시작
	for (int i = 0; i < threadcount; ++i) {
		ProsessThreads.emplace_back(&RoutineProgress::RoutineProgressWorkerThread,
			this, i);
	}
}

RoutineProgress::~RoutineProgress() { Release(); }

// 송신 큐에 패킷 추가 (실제 전송은 NetWork 클래스에서 처리)
void RoutineProgress::SendData_to_Sendque(const std::vector<uint8_t>& data) {
	G_network->addToSendQueue(data);
}

// 수신 패킷 AES 복호화 및 타입별 처리
void RoutineProgress::HandleReceivedPacket(const std::vector<uint8_t>& data) {
	try {
		// AES 인스턴스 유효성 검사
		if (!AES)
			return;

		size_t size = data.size();
		CompactBinaryReader* reader = new CompactBinaryReader;  // 이진 역직렬화용
		std::vector<uint8_t> Serialize_data;
		std::vector<uint8_t> encrypted_data;
		std::vector<uint8_t> decrypted_data;

		// AES 복호화 수행
		AES->Aes_Decrypt(&data, &decrypted_data);
		PacketType packet_type;

		// RAII 패턴으로 메모리 자동 해제 (Custom Deleter 사용)
		std::unique_ptr<CompactBinaryReader, std::function<void(CompactBinaryReader*)>>
			readerguardforCompactBinaryReader(reader, [](CompactBinaryReader* p_reader) {
			delete p_reader;
				});

		// 패킷 헤더 파싱 (PacketType 추출)
		if (!OptimizedBinaryPacketSerializer::ParseSecurePacket(decrypted_data, packet_type, reader) || !reader) {
			return;
		}

		// 패킷 타입별 분기 처리
		switch (packet_type) {

			// Routine 서버 응답: Auth 서버 정보 수신
		case PacketType::FindAccountServerResponse:
		{
			ServerInfoPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ServerInfoPacket>(*reader, packet);

			// IP가 비어있으면 서버 정보 없음
			if (packet.IP.empty())
			{

			}
			// Auth 서버 연결 이벤트 트리거
			G_core->get_C_eventmanager()->trigger(EventType::SUCESS_ROUTINEAUTH, false, packet.IP, packet.port);
			break;
		}

		// 회원가입 응답 처리
		case PacketType::RegisterResponse:
		{
			ResultPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader, packet);

			// 회원가입 성공 시 초기 화면으로 전환
			if (packet.ResultTypes == ResultType::SignUp_Succeeded)
			{
				G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Init);
			}

			break;
		}

		// 로그인 응답 처리
		case PacketType::LoginResponse:
		{
			ResultAndHashPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(*reader, packet);

			// 로그인 성공 시
			if (packet.ResultTypes == ResultType::Login_Succeeded)
			{
				// 세션 해시 저장 (로비 서버 인증용)
				G_GuiControl->hash = packet.hash;

				// 안티치트 시스템 초기화 (동기 실행)
				G_core->get_C_eventmanager()->trigger(EventType::SECURITY_Init_EVENT, true, packet.hash);

				// 게임 로비 서버 연결
				G_core->get_C_eventmanager()->trigger(EventType::SUCESS_GAMELOBBY, false);
			}

			break;
		}

		// 로비 서버 검증 응답
		case PacketType::TryConnectLobbyServerResponse:
		{
			ResultAndHashPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ResultAndHashPacket>(*reader, packet);

			// 검증 성공 시 로비 화면으로 전환
			if (packet.ResultTypes == ResultType::CheckSession_Succeeded)
			{

				G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Lobby);
			}

			break;
		}

		// 게임 방 참가 응답
		case PacketType::JoinRoomResponse:
		{
			ResultPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader, packet);

			// 참가 성공 시 매칭 대기 화면으로 전환
			if (packet.ResultTypes == ResultType::JoinRoom_Succeeded)
			{
				G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Matching);
			}

			break;
		}

		// 게임 방 취소 응답
		case PacketType::CancelRoomResponse:
		{
			ResultPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader, packet);

			// 취소 성공 시 로비로 복귀
			if (packet.ResultTypes == ResultType::CancelRoom_Succeeded)
			{
				G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Lobby);
			}

			break;
		}

		// 로그아웃 응답
		case PacketType::LogoutResponse:
		{
			ResultPacket packet;
			OptimizedBinaryPacketSerializer::DeserializePacket<ResultPacket>(*reader, packet);

			// 로그아웃 성공 시 초기 화면으로 전환
			if (packet.ResultTypes == ResultType::LogOut_Succeeded)
			{
				G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, false, EUIType::Init);
			}

			break;
		}

		// 서버 하트비트 요청 (안티치트 검증)
		case PacketType::HeartbeatRequest:
		{

			// 안티치트 드라이버 상태 검증 후 응답 전송
			G_core->get_C_eventmanager()->trigger(EventType::SECURITY_Heartbeat_EVENT, false, PacketType::HeartbeatResponse);


			break;
		}

		// 게임 시작: 게임 서버 정보 수신
		case PacketType::GameCreate:
		{
			ServerInfoPacket INFO;
			OptimizedBinaryPacketSerializer::DeserializePacket<ServerInfoPacket>(*reader, INFO);

			// 게임 서버 정보가 유효한 경우
			if (!INFO.IP.empty())
			{
				// 게임 상태 변경
				G_core->get_C_eventmanager()->trigger(EventType::CHANGE_UI_TYPE, true, EUIType::Game);

				// 언리얼 게임 실행 이벤트 트리거
				G_core->get_C_eventmanager()->trigger(EventType::STARTGAME_EVENT, false, INFO.IP, INFO.port);
			}
			break;
		}
		default:
			break;
		}
	}
	catch (...) {
		// 예외 발생 시 조용히 무시
		return;
	}
}

// 패킷 처리 큐에 추가 (네트워크 수신 스레드에서 호출)
void RoutineProgress::addToProgressQueue(const std::vector<uint8_t>& data) {
	{
		// 큐 접근 동기화
		std::lock_guard<std::mutex> lock(routine_queue_mutex);
		data_queue.push(data);
	}
	// 대기 중인 Worker Thread 하나 깨우기
	wakeUpthread.notify_one();
}

// Worker Thread 정리 및 종료
void RoutineProgress::Release() {
	// 종료 플래그 설정
	ProsessThreads_status.store(false);

	// 모든 Worker Thread 깨우기
	wakeUpthread.notify_all();

	// 모든 Worker Thread가 종료될 때까지 대기
	for (auto& thread : ProsessThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

// Worker Thread 메인 루프
void RoutineProgress::RoutineProgressWorkerThread(int threadId) {
	std::vector<uint8_t> data;

	// 종료 플래그가 false가 될 때까지 반복
	while (ProsessThreads_status.load()) {

		{
			std::unique_lock<std::mutex> lock(routine_queue_mutex);

			// 큐에 데이터가 들어오거나 종료 신호가 올 때까지 대기
			wakeUpthread.wait(lock, [this]() {
				return !data_queue.empty() || !ProsessThreads_status.load();
				});

			// 종료 신호 확인
			if (!ProsessThreads_status.load())
				break;

			// 큐에서 패킷 꺼내기
			data = data_queue.front();
			data_queue.pop();
		}

		// 패킷 처리
		HandleReceivedPacket(data);
	}
}

// TypePacket 전송 (타입만 포함)
void RoutineProgress::SendResponseForTypePacket(PacketType type)
{
	TypePacket packet;
	packet.Type = type;
	return SerializeAndSendResponse<TypePacket>(packet);
}

// TwoStringPacket 전송 (로그인/회원가입)
void RoutineProgress::SendResponseForTwoStringPacket(PacketType type, const std::string& str1, const std::string& str2)
{
	TwoStringPacket packet;
	packet.Type = type;
	packet.str1 = str1;  // 아이디
	packet.str2 = str2;  // 비밀번호
	return SerializeAndSendResponse<TwoStringPacket>(packet);
}

// HashPacket 전송 (세션 해시)
void RoutineProgress::SendResponseForHashPacket(PacketType type, const std::string& str1)
{
	HashPacket packet;
	packet.Type = type;
	packet.hash = str1;
	return SerializeAndSendResponse<HashPacket>(packet);
}

// IntegrityCheckPacket 전송 (하드웨어 ID)
void RoutineProgress::SendResponseForIntegrityCheckPacket(const IntegrityCheckPacket& packet)
{

	return SerializeAndSendResponse<IntegrityCheckPacket>(packet);
}

// stringforVectorPacket 전송 (문자열 배열)
void RoutineProgress::SendResponseForstringforVectorPacket(const stringforVectorPacket& packet)
{

	return SerializeAndSendResponse<stringforVectorPacket>(packet);
}

// ResultPacket 전송 (작업 결과)
void RoutineProgress::SendResponseForstringforResultPacket(const ResultPacket& packet)
{

	return SerializeAndSendResponse<ResultPacket>(packet);
}

// 우선순위 패킷 (즉시 암호화하여 반환)
std::vector<uint8_t> RoutineProgress::SendResponseForpriorityPacket(PacketType type, const std::string& str1)
{
	HashPacket packet;
	packet.Type = type;
	packet.hash = str1;
	std::vector<uint8_t> encrypted_data;
	std::vector<uint8_t> data;


	OptimizedBinaryPacketSerializer::SerializePacket<HashPacket>(packet, &data);


	AES->Aes_Encrypt(&data, &encrypted_data);

	// 송신 큐를 거치지 않고 즉시 반환
	return encrypted_data;
}

// 패킷 직렬화 + AES 암호화 + 송신 큐 추가 (템플릿 함수)
template<typename T>
void RoutineProgress::SerializeAndSendResponse(const T& response_packet) {

	std::vector<uint8_t> encrypted_data;
	std::vector<uint8_t> data;

	// 이진 직렬화
	OptimizedBinaryPacketSerializer::SerializePacket<T>(response_packet, &data);

	// AES 암호화
	AES->Aes_Encrypt(&data, &encrypted_data);

	// 송신 큐에 추가
	SendData_to_Sendque(encrypted_data);
}