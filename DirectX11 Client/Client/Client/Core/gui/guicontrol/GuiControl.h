/**
 * @file GuiControl.h
 * @brief Central Orchestrator for the D3D11-based GUI Subsystem and Application State.
 */

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

// Forward Declarations
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

/** @brief Standard buffer size for credential and identity strings. */
const int ID_PW_SIZE = 256;

/**
 * @class GuiControl
 * @brief Manages the entire lifecycle of the client GUI, from hardware initialization to state-driven rendering.
 * * This class integrates DirectX 11, Dear ImGui, and Windows Imaging Component (WIC) 
 * to provide a high-performance, animated interface for the game client.
 */
class GuiControl
{
public:
    GuiControl();
    ~GuiControl();

    /**
     * @brief Bootstraps all GUI-related subsystems (DirectX, WIC, ImGui, and Panels).
     * @param hwnd Handle to the host application window.
     * @return True if initialization of all components succeeded.
     */
    bool Initialize(HWND hwnd);

    /** @brief Safely releases all allocated hardware resources and software contexts. */
    void Cleanup();

    /**
     * @brief The primary rendering entry point executed once per frame.
     * Orchestrates background animation, UI styles, and the active state panel.
     */
    void Render();

    /** @brief Swaps the back buffer to the front (presents the frame to the user). */
    void Presents();

    /**
     * @brief Executes a UI state transition and manages session-related timers.
     * @param changeType The target UI state to switch to.
     */
    void SetUitype(EUIType changeType);

    /** @brief Returns the D3D11 Resource View for the current background GIF frame. */
    ID3D11ShaderResourceView* GetCurrentFrame() const;

    /** @brief Renders the dynamic GIF animation as a full-screen background overlay. */
    void BackgroundRender();

    /** @brief Stores and protects the session hash received upon successful login. */
    void LoginSuccessedHandle(const std::string in_hash);

    /** @brief Checks if the user is currently authenticated with a valid session hash. */
    bool IsLogin();

    /** @brief Retrieves the active session hash in a thread-safe manner. */
    std::string GetUserHash();

    // --- Event Dispatchers (Network Handshaking) ---

    /** @brief Dispatches a Login request to the EventManager. */
    void SignIn(const std::string& input_id, const std::string& input_Pw);

    /** @brief Dispatches an Account Registration request after credential validation. */
    void SignUp(const std::string& input_id, const std::string& input_Pw, const std::string& check_input_Pw);

    /** @brief Dispatches a Matchmaking Join request. */
    bool JoinRoom(const std::string& hash, const std::string& NickName);

    /** @brief Dispatches a Matchmaking Cancellation request. */
    void CancleRoom(const std::string& hash);

    /** @brief Dispatches a Logout/Session-termination request. */
    bool LogOut(const std::string& hash);

    /** @brief Applies the custom "Mafia Noir" visual theme to the ImGui context. */
    void SetMafiaStyle();

    /** @brief Restores default ImGui styles by popping the style stack. */
    void RestoreStyle();

    // --- UI Input Buffers ---
    char TempId[ID_PW_SIZE] = { 0 };           
    char TempPw[ID_PW_SIZE] = { 0 };           
    char TempCheckPw[ID_PW_SIZE] = { 0 };      
    char TempNickName[ID_PW_SIZE] = { 0 };     
    wchar_t NickName[ID_PW_SIZE * 2] = { 0 };  

private:
    // --- Subsystem Lifecycle Management ---
    bool InitializeDirectX(HWND hwnd);
    void CleanupDirectX();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    bool InitializeWIC();
    void CleanupWIC();

    // --- Asset & Animation Management ---
    bool LoadGIFAnimation(const std::string& GIF_path);
    void UpdateGIFAnimation();
    HRESULT CreateTextureFromWICBitmap(IWICBitmapSource* pBitmapSource, ID3D11ShaderResourceView** ppSRV);

    /** @brief Loads fonts and configures Korean glyph support. */
    bool LoadFont(const std::wstring& Font_path);

public:
    // --- DirectX 11 Core Components ---
    ID3D11Device* g_pd3dDevice;
    ID3D11DeviceContext* g_pd3dDeviceContext;
    IDXGISwapChain* g_pSwapChain;
    ID3D11RenderTargetView* g_mainRenderTargetView;

    // --- WIC & Animated Asset Data ---
    IWICImagingFactory* g_pWICFactory;
    std::vector<ID3D11ShaderResourceView*> m_gifFrames;
    std::vector<unsigned int> m_frameDelays;
    float m_currentFrameFloat;
    double m_lastUpdateTime;

    // --- Application State & UI Panels ---
    /** @brief Container for distinct UI panels, accessed via EUIType keys. */
    std::unordered_map<EUIType, std::unique_ptr<BaseGui>> panels;
    
    /** @brief Top-level overlay for global window controls. */
    std::unique_ptr<ControlGui> ControlPanel;

    /** @brief Thread-safe indicator of the current UI state. */
    std::atomic<EUIType> currentUitype;

    std::chrono::steady_clock::time_point m_matchingStartTime;
    std::chrono::steady_clock::time_point m_GameStartTime;

    bool m_initialized;
    HWND m_mainWindow;

    // --- Secure Session Data ---
    std::string hash;
    std::mutex HashMtx;
};

/** Global access to the GuiControl singleton. */
extern std::unique_ptr<GuiControl> G_GuiControl;
