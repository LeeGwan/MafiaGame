// GUI 컨트롤러 (DirectX11 + ImGui 기반 GUI 시스템 관리)
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <wincodec.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <string>

enum class EUIType : uint8_t;  
class BaseGui;                  
class ControlGui;             
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct IWICImagingFactory;
struct IWICBitmapSource;

const int ID_PW_SIZE = 256;  // 입력 버퍼 크기

class GuiControl
{
public:
    GuiControl();
    ~GuiControl();

    // DirectX11 및 ImGui 초기화
    bool Initialize(HWND hwnd);

    // 리소스 정리
    void Cleanup();

    // GUI 렌더링 (매 프레임 호출)
    void Render();

    // 화면 표시 (SwapChain Present)
    void Presents();

    // UI 타입 변경 (화면 전환)
    void SetUitype(EUIType changeType);

    // 현재 GIF 프레임 반환
    ID3D11ShaderResourceView* GetCurrentFrame() const;

    // 배경 GIF 렌더링
    void BackgroundRender();

    // 로그인 성공 처리 (세션 해시 저장)
    void LoginSuccessedHandle(const std::string in_hash);

    // 로그인 상태 확인
    bool IsLogin();

    // 세션 해시 반환
    std::string GetUserHash();

    // 로그인 요청 (EventManager를 통해 패킷 전송)
    void SignIn(const std::string& input_id, const std::string& input_Pw);

    // 회원가입 요청 (비밀번호 일치 확인 후 패킷 전송)
    void SignUp(const std::string& input_id, const std::string& input_Pw, const std::string& check_input_Pw);

    // 게임 방 참가 요청 (매칭 시작)
    bool JoinRoom(const std::string& hash, const std::string& NickName);

    // 게임 방 취소 요청 (매칭 취소)
    void CancleRoom(const std::string& hash);

    // 로그아웃 요청
    bool LogOut(const std::string& hash);

    // ImGui 스타일 설정 (마피아 테마)
    void SetMafiaStyle();

    // ImGui 스타일 복원
    void RestoreStyle();

    // 입력 버퍼 (ImGui::InputText용)
    char TempId[ID_PW_SIZE] = { 0 };           // 아이디 입력
    char TempPw[ID_PW_SIZE] = { 0 };           // 비밀번호 입력
    char TempCheckPw[ID_PW_SIZE] = { 0 };      // 비밀번호 확인 입력
    char TempNickName[ID_PW_SIZE] = { 0 };     // 닉네임 입력
    wchar_t NickName[ID_PW_SIZE * 2] = { 0 };    // 와이드 문자 닉네임

private:
    // DirectX11 초기화 및 정리
    bool InitializeDirectX(HWND hwnd);
    void CleanupDirectX();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    // WIC (Windows Imaging Component) 초기화 및 정리
    bool InitializeWIC();
    void CleanupWIC();

    // GIF 애니메이션 로드 및 업데이트
    bool LoadGIFAnimation(const std::string& GIF_path);
    void UpdateGIFAnimation();
    HRESULT CreateTextureFromWICBitmap(IWICBitmapSource* pBitmapSource, ID3D11ShaderResourceView** ppSRV);

    // 폰트 로드 (한글 지원)
    bool LoadFont(const std::wstring& Font_path);

public:
    // DirectX11 관련
    ID3D11Device* g_pd3dDevice;                   // D3D11 디바이스
    ID3D11DeviceContext* g_pd3dDeviceContext;     // D3D11 디바이스 컨텍스트
    IDXGISwapChain* g_pSwapChain;                 // 스왑체인
    ID3D11RenderTargetView* g_mainRenderTargetView;  // 렌더 타겟 뷰

    // WIC 및 GIF 관련
    IWICImagingFactory* g_pWICFactory;                  // WIC 팩토리
    std::vector<ID3D11ShaderResourceView*> m_gifFrames; // GIF 프레임 텍스처 배열
    std::vector<unsigned int> m_frameDelays;            // 각 프레임 딜레이 (밀리초)
    float m_currentFrameFloat;                          // 현재 프레임 인덱스 (float)
    double m_lastUpdateTime;                            // 마지막 업데이트 시간

    // GUI 관련
    std::unordered_map<EUIType, std::unique_ptr<BaseGui>> panels;  // UI 타입별 패널 맵
    std::unique_ptr<ControlGui> ControlPanel;                      // 제어 패널 (최소화, 닫기 등)
    std::atomic<EUIType> currentUitype;                            // 현재 UI 타입 (스레드 안전)
    std::chrono::steady_clock::time_point m_matchingStartTime;     // 매칭 시작 시간
    std::chrono::steady_clock::time_point m_GameStartTime;         // 게임 시작 시간

    bool m_initialized;  // 초기화 완료 여부

    HWND m_mainWindow;     // 메인 윈도우 핸들
    std::string hash;      // 세션 해시 (로그인 후 저장)
    std::mutex HashMtx;    // 세션 해시 동기화용 뮤텍스
};

// 전역 싱글톤 인스턴스
extern std::unique_ptr<GuiControl> G_GuiControl;