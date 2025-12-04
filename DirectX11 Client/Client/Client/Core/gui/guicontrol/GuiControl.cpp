// GUI 컨트롤러 구현부
#include "GuiControl.h"
#include "../EUIType/EUIType.h"
#include "../basegui/BaseGui.h"
#include"../InitGui/InitGui.h"
#include "../ControlGui/ControlGui.h"
#include "../LobbyGui/LobbyGui.h"
#include "../LoginGui/LoginGui.h"
#include "../GameGui/GameGui.h"
#include "../MatchingGui/MatchingGui.h"
#include "../RegisterGui/RegisterGui.h"
#include "../../Utils/UtilsForString.h"
#include "../../Core/Core.h"
#include "../../Event/EventManager/EventManager.h"
#include "../../Event/EventType/EventType.h"
#include "../../Network/Packet/PacketStructure/PacketStructure.h"
#include <fstream>
#include <filesystem>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wincodec.h>
#include <chrono>
#include <algorithm>
#include "../../../Dependencies/Imgui/imgui.h"
#include "../../../Dependencies/Imgui/imgui_impl_win32.h"
#include "../../../Dependencies/Imgui/imgui_impl_dx11.h"

// 전역 싱글톤 인스턴스 생성
std::unique_ptr<GuiControl> G_GuiControl = std::make_unique<GuiControl>();

// 생성자: 멤버 변수 초기화
GuiControl::GuiControl() : g_pd3dDevice(nullptr), g_pd3dDeviceContext(nullptr), g_pSwapChain(nullptr),
g_mainRenderTargetView(nullptr), g_pWICFactory(nullptr),
m_initialized(false),
m_currentFrameFloat(0.0f), m_lastUpdateTime(0.0), m_mainWindow(nullptr)
{
	// 닉네임 기본값 설정
	strcpy_s(TempNickName, ID_PW_SIZE, u8"tt");
	currentUitype = EUIType::Init;  // 초기 화면으로 설정
}

// 소멸자: 리소스 정리
GuiControl::~GuiControl()
{
	try {
		if (m_initialized) {
			Cleanup();
		}
	}
	catch (...) {
		// 예외 무시 (소멸자에서 예외 던지면 안 됨)
	}
}

// DirectX11 및 ImGui 초기화
bool GuiControl::Initialize(HWND hwnd)
{
	m_mainWindow = hwnd;

	// WIC 초기화 (이미지 로드용)
	if (!InitializeWIC()) {
		return false;
	}

	// DirectX11 초기화
	if (!InitializeDirectX(hwnd)) {
		return false;
	}

	// ImGui 컨텍스트 생성
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// 폰트 로드 (한글 지원)
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::wstring font_path = currentPath.wstring() + L"\\fonts\\Thin.ttf";
	if (!LoadFont(font_path)) return false;

	// ImGui 스타일 설정
	ImGui::StyleColorsDark();

	// ImGui Win32 초기화
	ImGui_ImplWin32_Init(hwnd);

	// ImGui DirectX11 초기화
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// 배경 GIF 로드
	std::string GIF_path = currentPath.string() + "\\GIF\\Background.gif";
	if (!LoadGIFAnimation(GIF_path)) return false;

	// 각 UI 타입별 패널 생성
	panels[EUIType::Init] = std::make_unique<InitGui>(G_GuiControl.get());
	panels[EUIType::Login] = std::make_unique<LoginGui>(G_GuiControl.get());
	panels[EUIType::Register] = std::make_unique<RegisterGui>(G_GuiControl.get());
	panels[EUIType::Lobby] = std::make_unique<LobbyGui>(G_GuiControl.get());
	panels[EUIType::Matching] = std::make_unique<MatchingGui>(G_GuiControl.get());
	panels[EUIType::Game] = std::make_unique<GameGui>(G_GuiControl.get());

	// 제어 패널 생성 (최소화, 닫기 버튼)
	ControlPanel = std::make_unique<ControlGui>(G_GuiControl.get());

	m_initialized = true;
	return true;
}

// GUI 렌더링 (매 프레임 호출)
void GuiControl::Render()
{
	// 초기화되지 않았으면 리턴
	if (!m_initialized) return;

	// ImGui 프레임 시작
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 배경 GIF 렌더링
	BackgroundRender();

	// 마피아 테마 스타일 적용
	SetMafiaStyle();

	// 제어 패널 렌더링 (최소화, 닫기 버튼)
	ControlPanel->Render();

	// 현재 UI 타입에 해당하는 패널 렌더링
	auto type = currentUitype.load();
	if (panels.count(type)) {
		panels[type]->Render();
	}

	// 스타일 복원
	RestoreStyle();

	// ImGui 렌더링 완료
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// 리소스 정리
void GuiControl::Cleanup()
{
	// 초기화되지 않았으면 리턴
	if (!m_initialized) return;

	m_initialized = false;

	// GUI 패널 정리
	try {
		ControlPanel.reset();
		panels.clear();
	}
	catch (...) {
		// 예외 무시
	}

	// ImGui 종료
	try {
		if (ImGui::GetCurrentContext()) {
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}
	}
	catch (...) {
		// 예외 무시
	}

	// WIC 정리
	CleanupWIC();

	// DirectX11 정리
	CleanupDirectX();
}

// 화면 표시 (SwapChain Present)
void GuiControl::Presents()
{
	if (g_pSwapChain) {
		g_pSwapChain->Present(0, 0);  // vsync 꺼짐
	}
}

// 현재 GIF 프레임 반환
ID3D11ShaderResourceView* GuiControl::GetCurrentFrame() const
{
	if (m_gifFrames.empty()) return nullptr;

	// float를 int로 변환하여 프레임 인덱스 계산
	int frameIndex = (int)m_currentFrameFloat;
	if (frameIndex >= (int)m_gifFrames.size()) frameIndex = 0;

	return m_gifFrames[frameIndex];
}

// 배경 GIF 렌더링
void GuiControl::BackgroundRender()
{
	// GIF 프레임 업데이트
	UpdateGIFAnimation();

	ImGuiIO& io = ImGui::GetIO();

	// 렌더 타겟 클리어 (검은색)
	const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

	// 현재 GIF 프레임 가져오기
	ID3D11ShaderResourceView* currentFrame = GetCurrentFrame();
	if (currentFrame) {
		// 배경에 GIF 프레임 그리기
		ImGui::GetBackgroundDrawList()->AddImage(
			(void*)currentFrame,
			ImVec2(0, 0),           // 시작 위치
			io.DisplaySize,         // 끝 위치 (전체 화면)
			ImVec2(0, 0),           // UV 시작
			ImVec2(1, 1),           // UV 끝
			IM_COL32(255, 255, 255, 255)  // 흰색 (알파 255)
		);
	}
}

// UI 타입 변경 (화면 전환)
void GuiControl::SetUitype(EUIType changeType)
{
	// 매칭 화면으로 전환 시 시작 시간 기록
	if (changeType == EUIType::Matching)
	{
		m_matchingStartTime = std::chrono::steady_clock::now();
	}
	// 게임 화면으로 전환 시 시작 시간 기록
	else if (changeType == EUIType::Game)
	{
		m_GameStartTime = std::chrono::steady_clock::now();
	}

	// 현재 UI 타입 변경 (atomic)
	G_GuiControl->currentUitype.store(changeType);
}

// 로그인 성공 처리 (세션 해시 저장)
void GuiControl::LoginSuccessedHandle(const std::string in_hash)
{
	{
		std::lock_guard<std::mutex> lock(HashMtx);
		hash = in_hash;
	}
}

// 로그인 상태 확인
bool GuiControl::IsLogin()
{
	std::lock_guard<std::mutex> lock(HashMtx);
	return !hash.empty();
}

// 세션 해시 반환
std::string GuiControl::GetUserHash()
{
	std::lock_guard<std::mutex> lock(HashMtx);
	return hash;
}

// 로그인 요청 (EventManager를 통해 패킷 전송)
void GuiControl::SignIn(const std::string& input_id, const std::string& input_Pw)
{
	G_core->get_C_eventmanager()->trigger(EventType::TwoStringPacket_EVNET, false, PacketType::LoginRequest, input_id, input_Pw);
}

// 회원가입 요청 (비밀번호 일치 확인 후 패킷 전송)
void GuiControl::SignUp(const std::string& input_id, const std::string& input_Pw, const std::string& check_input_Pw)
{
	// 비밀번호와 비밀번호 확인이 일치하는지 확인
	if (input_Pw == check_input_Pw)
	{
		G_core->get_C_eventmanager()->trigger(EventType::TwoStringPacket_EVNET, false, PacketType::RegisterRequest, input_id, input_Pw);
		return;
	}

	// 비밀번호 불일치 시 (현재 주석 처리)
//	G_core->get_C_eventmanager()->trigger(EventType::MESSAGE_EVENT, false, ResultType::SignUp_Not_Match);
}

// 게임 방 참가 요청 (매칭 시작)
bool GuiControl::JoinRoom(const std::string& hash, const std::string& NickName)
{
	G_core->get_C_eventmanager()->trigger(EventType::TwoStringPacket_EVNET, false, PacketType::JoinRoomRequest, hash, NickName);
	return false;
}

// 게임 방 취소 요청 (매칭 취소)
void GuiControl::CancleRoom(const std::string& hash)
{
	G_core->get_C_eventmanager()->trigger(EventType::HashPacket_EVNET, false, PacketType::CancelRoomRequest, hash);
}

// 로그아웃 요청
bool GuiControl::LogOut(const std::string& hash)
{
	G_core->get_C_eventmanager()->trigger(EventType::HashPacket_EVNET, false, PacketType::LogoutRequest, hash);
	return false;
}

// DirectX11 초기화
bool GuiControl::InitializeDirectX(HWND hwnd)
{
	// 스왑체인 설정
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;  // 더블 버퍼링
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 8비트 RGBA
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;  // MSAA 끄기
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

	// D3D11 디바이스 및 스왑체인 생성
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
		&g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

	// 하드웨어 가속 실패 시 WARP 드라이버로 재시도
	if (res == DXGI_ERROR_UNSUPPORTED)
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
			createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
			&g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

	if (FAILED(res)) {
		return false;
	}

	// 렌더 타겟 생성
	CreateRenderTarget();
	return true;
}

// DirectX11 정리
void GuiControl::CleanupDirectX()
{
	// 렌더 타겟 정리
	CleanupRenderTarget();

	// 스왑체인 해제
	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}

	// 디바이스 컨텍스트 해제
	if (g_pd3dDeviceContext) {
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = nullptr;
	}

	// 디바이스 해제
	if (g_pd3dDevice) {
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}
}

// 렌더 타겟 생성
void GuiControl::CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;

	// 스왑체인에서 백버퍼 가져오기
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

	// 렌더 타겟 뷰 생성
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

// 렌더 타겟 정리
void GuiControl::CleanupRenderTarget()
{
	if (g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

// WIC 초기화 (Windows Imaging Component)
bool GuiControl::InitializeWIC()
{
	// COM 초기화
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (hr == S_OK) {
		// 초기화 성공
	}
	else if (hr == S_FALSE) {
		// 이미 초기화됨
	}
	else if (hr == RPC_E_CHANGED_MODE) {
		// 다른 모드로 이미 초기화됨
	}
	else {
		// 초기화 실패
		return false;
	}

	// WIC 팩토리 생성
	hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g_pWICFactory));

	if (SUCCEEDED(hr)) {
		// WIC 팩토리 생성 성공
	}
	else {
		// WIC 팩토리 생성 실패
	}

	return SUCCEEDED(hr);
}

// WIC 정리
void GuiControl::CleanupWIC()
{
	// GIF 프레임 텍스처 해제
	for (auto& frame : m_gifFrames) {
		if (frame) {
			frame->Release();
			frame = nullptr;
		}
	}
	m_gifFrames.clear();
	m_frameDelays.clear();

	// WIC 팩토리 해제
	if (g_pWICFactory) {
		try {
			g_pWICFactory->Release();
		}
		catch (...) {
			// 예외 무시
		}
		g_pWICFactory = nullptr;
	}
}

// GIF 애니메이션 로드
bool GuiControl::LoadGIFAnimation(const std::string& GIF_path)
{
	// WIC와 D3D11 디바이스 유효성 검사
	if (!g_pWICFactory || !g_pd3dDevice) return false;

	// 기존 GIF 프레임 해제
	for (auto& frame : m_gifFrames) {
		if (frame) frame->Release();
	}
	m_gifFrames.clear();
	m_frameDelays.clear();

	// UTF-8 -> wstring 변환
	std::wstring wGIF_path = UtilsForString::UTF8ToWString(GIF_path, CP_ACP);

	// GIF 디코더 생성
	IWICBitmapDecoder* pDecoder = nullptr;
	HRESULT hr = g_pWICFactory->CreateDecoderFromFilename(wGIF_path.c_str(), nullptr, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &pDecoder);
	if (FAILED(hr)) return false;

	// 프레임 개수 가져오기
	UINT frameCount = 0;
	hr = pDecoder->GetFrameCount(&frameCount);
	if (FAILED(hr)) {
		pDecoder->Release();
		return false;
	}

	// 각 프레임 로드
	for (UINT i = 0; i < frameCount; i++) {
		IWICBitmapFrameDecode* pFrame = nullptr;
		hr = pDecoder->GetFrame(i, &pFrame);
		if (FAILED(hr)) continue;

		// 프레임 딜레이 가져오기 (메타데이터에서)
		unsigned int frameDelay = 100;  // 기본값
		IWICMetadataQueryReader* pMetadataReader = nullptr;
		if (SUCCEEDED(pFrame->GetMetadataQueryReader(&pMetadataReader))) {
			PROPVARIANT propValue;
			PropVariantInit(&propValue);

			if (SUCCEEDED(pMetadataReader->GetMetadataByName(L"/grctlext/Delay", &propValue))) {
				if (propValue.vt == VT_UI2) {
					frameDelay = propValue.uiVal * 10;  // 1/100초 -> 밀리초
					if (frameDelay < 20) frameDelay = 100;  // 너무 빠르면 100ms로 제한
				}
			}
			PropVariantClear(&propValue);
			pMetadataReader->Release();
		}

		// 프레임을 D3D11 텍스처로 변환
		ID3D11ShaderResourceView* pSRV = nullptr;
		if (SUCCEEDED(CreateTextureFromWICBitmap(pFrame, &pSRV)) && pSRV) {
			m_gifFrames.push_back(pSRV);
			m_frameDelays.push_back(frameDelay);
		}
		if (pFrame) pFrame->Release();
	}

	pDecoder->Release();

	// 프레임이 하나도 없으면 실패
	if (m_gifFrames.empty()) return false;

	// 프레임 인덱스 및 시간 초기화
	m_currentFrameFloat = 0.0f;
	auto now = std::chrono::high_resolution_clock::now();
	m_lastUpdateTime = std::chrono::duration<double, std::milli>(now.time_since_epoch()).count();
	return true;
}

// GIF 애니메이션 업데이트 (프레임 전환)
void GuiControl::UpdateGIFAnimation()
{
	if (m_gifFrames.empty()) return;

	static auto lastTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();

	// 70ms 경과 시 다음 프레임으로 전환
	if (deltaTime >= 70) {
		m_currentFrameFloat += 1.0f;
		if (m_currentFrameFloat >= (float)m_gifFrames.size()) {
			m_currentFrameFloat = 0.0f;  // 처음으로 되돌림 (루프)
		}
		lastTime = currentTime;
	}
}

// WIC 비트맵을 D3D11 텍스처로 변환
HRESULT GuiControl::CreateTextureFromWICBitmap(IWICBitmapSource* pBitmapSource, ID3D11ShaderResourceView** ppSRV)
{
	// 매개변수 유효성 검사
	if (!g_pd3dDevice || !g_pWICFactory || !pBitmapSource || !ppSRV) {
		return E_INVALIDARG;
	}

	// 비트맵 크기 가져오기
	UINT width, height;
	pBitmapSource->GetSize(&width, &height);

	// 포맷 컨버터 생성 (RGBA로 변환)
	IWICFormatConverter* pConverter = nullptr;
	HRESULT hr = g_pWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// RAII 패턴으로 컨버터 자동 해제
	auto converterCleanup = [&pConverter]() {
		if (pConverter) {
			pConverter->Release();
		}
		};

	// 포맷 변환 초기화 (32비트 RGBA)
	hr = pConverter->Initialize(pBitmapSource, GUID_WICPixelFormat32bppRGBA,
		WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		converterCleanup();
		return hr;
	}

	// 픽셀 데이터 복사
	UINT stride = width * 4;  // RGBA = 4바이트
	UINT bufferSize = stride * height;
	std::vector<BYTE> pixels(bufferSize);

	hr = pConverter->CopyPixels(nullptr, stride, bufferSize, pixels.data());
	converterCleanup();
	if (FAILED(hr)) return hr;

	// D3D11 텍스처 생성
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;  // 불변 텍스처
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixels.data();
	initData.SysMemPitch = stride;

	ID3D11Texture2D* pTexture = nullptr;
	hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, &pTexture);
	if (FAILED(hr)) return hr;

	// 셰이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;

	hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, ppSRV);
	if (pTexture) pTexture->Release();

	return hr;
}

// 폰트 로드 (한글 지원)
bool GuiControl::LoadFont(const std::wstring& Font_path)
{
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // 키보드 네비게이션 활성화

	// wstring -> UTF-8 변환
	std::string font_utf8 = UtilsForString::WStringToUTF8(Font_path);

	// 폰트 추가 (한글 글리프 포함)
	ImFont* font = io.Fonts->AddFontFromFileTTF(font_utf8.c_str(), 18.0f, NULL, io.Fonts->GetGlyphRangesKorean());
	return font != nullptr;
}

// ImGui 스타일 설정 (마피아 테마)
void GuiControl::SetMafiaStyle()
{
	// 색상 설정 (어두운 배경 + 빨간색 강조)
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 0.15f));      // 윈도우 배경
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 0.15f));       // 자식 윈도우 배경
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.1f, 0.1f, 0.0f));            // 테두리
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));              // 텍스트 (흰색)
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 0.8f));            // 버튼
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 0.9f));     // 버튼 호버
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.05f, 0.05f, 1.0f));    // 버튼 눌림
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));           // 입력 박스 배경
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.9f));    // 입력 박스 호버
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));  // 입력 박스 활성

	// 스타일 변수 설정
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);            // 자식 윈도우 둥글기
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);           // 자식 윈도우 테두리 크기
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);             // 프레임 둥글기
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));  // 버튼 텍스트 중앙 정렬
}

// ImGui 스타일 복원
void GuiControl::RestoreStyle()
{
	ImGui::PopStyleVar(4);   // 스타일 변수 4개 복원
	ImGui::PopStyleColor(10);  // 색상 10개 복원
}