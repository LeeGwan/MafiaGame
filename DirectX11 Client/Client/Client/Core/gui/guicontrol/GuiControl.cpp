/**
 * @file GuiControl.cpp
 * @brief Central controller for DX11-based ImGui rendering and application state management.
 */

#include "GuiControl.h"
#include "../EUIType/EUIType.h"
#include "../basegui/BaseGui.h"
#include "../InitGui/InitGui.h"
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

/** Global Singleton Instance for UI Orchestration */
std::unique_ptr<GuiControl> G_GuiControl = std::make_unique<GuiControl>();

GuiControl::GuiControl() 
    : g_pd3dDevice(nullptr), g_pd3dDeviceContext(nullptr), g_pSwapChain(nullptr),
      g_mainRenderTargetView(nullptr), g_pWICFactory(nullptr),
      m_initialized(false),
      m_currentFrameFloat(0.0f), m_lastUpdateTime(0.0), m_mainWindow(nullptr)
{
    // Default placeholder for rapid testing
    strcpy_s(TempNickName, ID_PW_SIZE, u8"Player");
    currentUitype = EUIType::Init;
}

GuiControl::~GuiControl()
{
    try {
        if (m_initialized) {
            Cleanup();
        }
    }
    catch (...) {
        // Suppress exceptions in destructor to prevent termination
    }
}

/**
 * @brief Bootstraps the UI system: DX11, WIC, ImGui, and specific UI panels.
 * @param hwnd Handle to the host window.
 * @return True if all subsystems initialized successfully.
 */
bool GuiControl::Initialize(HWND hwnd)
{
    m_mainWindow = hwnd;

    // Initialize Windows Imaging Component (WIC) for asset decoding
    if (!InitializeWIC()) return false;

    // Setup DirectX 11 Hardware Acceleration
    if (!InitializeDirectX(hwnd)) return false;

    // ImGui Context Initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Load System Fonts with Korean Glyph Range Support
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::wstring font_path = currentPath.wstring() + L"\\fonts\\Thin.ttf";
    if (!LoadFont(font_path)) return false;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Cinematic Background Assets (GIF)
    std::string GIF_path = currentPath.string() + "\\GIF\\Background.gif";
    if (!LoadGIFAnimation(GIF_path)) return false;

    // Instantiate Specialized UI Panels
    panels[EUIType::Init] = std::make_unique<InitGui>(G_GuiControl.get());
    panels[EUIType::Login] = std::make_unique<LoginGui>(G_GuiControl.get());
    panels[EUIType::Register] = std::make_unique<RegisterGui>(G_GuiControl.get());
    panels[EUIType::Lobby] = std::make_unique<LobbyGui>(G_GuiControl.get());
    panels[EUIType::Matching] = std::make_unique<MatchingGui>(G_GuiControl.get());
    panels[EUIType::Game] = std::make_unique<GameGui>(G_GuiControl.get());

    // Initialize the Master Control Panel (Overlay)
    ControlPanel = std::make_unique<ControlGui>(G_GuiControl.get());

    m_initialized = true;
    return true;
}

/**
 * @brief Core Render Loop: Executed every frame to draw the UI stack.
 */
void GuiControl::Render()
{
    if (!m_initialized) return;

    // Begin New Frame Handshake
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 1. Draw Background Layer (GIF Animation)
    BackgroundRender();

    // 2. Apply Custom Mafia/Noir Theme
    SetMafiaStyle();

    // 3. Render Top-Level Control Overlays
    ControlPanel->Render();

    // 4. Render Active UI State Panel
    auto type = currentUitype.load();
    if (panels.count(type)) {
        panels[type]->Render();
    }

    RestoreStyle();

    // Finalize Frame Rendering
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief Safely releases all hardware and software resources.
 */
void GuiControl::Cleanup()
{
    if (!m_initialized) return;
    m_initialized = false;

    try {
        ControlPanel.reset();
        panels.clear();
    } catch (...) {}

    try {
        if (ImGui::GetCurrentContext()) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
    } catch (...) {}

    CleanupWIC();
    CleanupDirectX();
}

void GuiControl::Presents()
{
    if (g_pSwapChain) {
        // Present without V-Sync for minimal latency
        g_pSwapChain->Present(0, 0);
    }
}

/**
 * @brief Retrieves the current texture frame of the background animation.
 * @return D3D11 SRV for the current frame index.
 */
ID3D11ShaderResourceView* GuiControl::GetCurrentFrame() const
{
    if (m_gifFrames.empty()) return nullptr;

    int frameIndex = (int)m_currentFrameFloat;
    if (frameIndex >= (int)m_gifFrames.size()) frameIndex = 0;

    return m_gifFrames[frameIndex];
}

void GuiControl::BackgroundRender()
{
    UpdateGIFAnimation();

    ImGuiIO& io = ImGui::GetIO();

    // Clear Render Target to black before background draw
    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

    ID3D11ShaderResourceView* currentFrame = GetCurrentFrame();
    if (currentFrame) {
        // Draw the background frame as an immersive overlay
        ImGui::GetBackgroundDrawList()->AddImage(
            (void*)currentFrame,
            ImVec2(0, 0),
            io.DisplaySize,
            ImVec2(0, 0),
            ImVec2(1, 1),
            IM_COL32(255, 255, 255, 255)
        );
    }
}

/**
 * @brief Transitions the UI state and initializes state-specific timers.
 * @param changeType The target UI state.
 */
void GuiControl::SetUitype(EUIType changeType)
{
    if (changeType == EUIType::Matching)
    {
        m_matchingStartTime = std::chrono::steady_clock::now();
    }
    else if (changeType == EUIType::Game)
    {
        m_GameStartTime = std::chrono::steady_clock::now();
    }

    currentUitype.store(changeType);
}

/**
 * @brief Securely updates the session hash upon successful authentication.
 */
void GuiControl::LoginSuccessedHandle(const std::string in_hash)
{
    std::lock_guard<std::mutex> lock(HashMtx);
    hash = in_hash;
}

bool GuiControl::IsLogin()
{
    std::lock_guard<std::mutex> lock(HashMtx);
    return !hash.empty();
}

std::string GuiControl::GetUserHash()
{
    std::lock_guard<std::mutex> lock(HashMtx);
    return hash;
}

// --- Request Dispatchers via EventManager ---

void GuiControl::SignIn(const std::string& input_id, const std::string& input_Pw)
{
    G_core->get_C_eventmanager()->trigger(EventType::TwoStringPacket_EVNET, false, PacketType::LoginRequest, input_id, input_Pw);
}

void GuiControl::SignUp(const std::string& input_id, const std::string& input_Pw, const std::string& check_input_Pw)
{
    // Client-side verification for credential integrity
    if (input_Pw == check_input_Pw)
    {
        G_core->get_C_eventmanager()->trigger(EventType::TwoStringPacket_EVNET, false, PacketType::RegisterRequest, input_id, input_Pw);
        return;
    }
}

bool GuiControl::JoinRoom(const std::string& hash, const std::string& NickName)
{
    G_core->get_C_eventmanager()->trigger(EventType::TwoStringPacket_EVNET, false, PacketType::JoinRoomRequest, hash, NickName);
    return false;
}

void GuiControl::CancleRoom(const std::string& hash)
{
    G_core->get_C_eventmanager()->trigger(EventType::HashPacket_EVNET, false, PacketType::CancelRoomRequest, hash);
}

bool GuiControl::LogOut(const std::string& hash)
{
    G_core->get_C_eventmanager()->trigger(EventType::HashPacket_EVNET, false, PacketType::LogoutRequest, hash);
    return false;
}

/**
 * @brief Configures DirectX 11 SwapChain and Hardware Device.
 */
bool GuiControl::InitializeDirectX(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    // Fallback to WARP (Software Rasterizer) if hardware acceleration is unsupported
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (FAILED(res)) return false;

    CreateRenderTarget();
    return true;
}

void GuiControl::CleanupDirectX()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void GuiControl::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void GuiControl::CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

bool GuiControl::InitializeWIC()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) return false;

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_pWICFactory));
    return SUCCEEDED(hr);
}

void GuiControl::CleanupWIC()
{
    for (auto& frame : m_gifFrames) {
        if (frame) { frame->Release(); frame = nullptr; }
    }
    m_gifFrames.clear();
    m_frameDelays.clear();

    if (g_pWICFactory) {
        try { g_pWICFactory->Release(); } catch (...) {}
        g_pWICFactory = nullptr;
    }
}

/**
 * @brief Decodes a GIF file and converts each frame into a D3D11 Shader Resource View.
 */
bool GuiControl::LoadGIFAnimation(const std::string& GIF_path)
{
    if (!g_pWICFactory || !g_pd3dDevice) return false;

    for (auto& frame : m_gifFrames) if (frame) frame->Release();
    m_gifFrames.clear();
    m_frameDelays.clear();

    std::wstring wGIF_path = UtilsForString::UTF8ToWString(GIF_path, CP_ACP);

    IWICBitmapDecoder* pDecoder = nullptr;
    HRESULT hr = g_pWICFactory->CreateDecoderFromFilename(wGIF_path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr)) return false;

    UINT frameCount = 0;
    pDecoder->GetCount(&frameCount);

    for (UINT i = 0; i < frameCount; i++) {
        IWICBitmapFrameDecode* pFrame = nullptr;
        if (FAILED(pDecoder->GetFrame(i, &pFrame))) continue;

        unsigned int frameDelay = 100;
        IWICMetadataQueryReader* pMetadataReader = nullptr;
        if (SUCCEEDED(pFrame->GetMetadataQueryReader(&pMetadataReader))) {
            PROPVARIANT propValue;
            PropVariantInit(&propValue);
            if (SUCCEEDED(pMetadataReader->GetMetadataByName(L"/grctlext/Delay", &propValue))) {
                if (propValue.vt == VT_UI2) frameDelay = propValue.uiVal * 10;
            }
            PropVariantClear(&propValue);
            pMetadataReader->Release();
        }

        ID3D11ShaderResourceView* pSRV = nullptr;
        if (SUCCEEDED(CreateTextureFromWICBitmap(pFrame, &pSRV)) && pSRV) {
            m_gifFrames.push_back(pSRV);
            m_frameDelays.push_back(frameDelay);
        }
        pFrame->Release();
    }
    pDecoder->Release();

    if (m_gifFrames.empty()) return false;
    m_currentFrameFloat = 0.0f;
    return true;
}

void GuiControl::UpdateGIFAnimation()
{
    if (m_gifFrames.empty()) return;
    static auto lastTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();

    // Constant frame rate for consistent UI experience
    if (deltaTime >= 70) {
        m_currentFrameFloat += 1.0f;
        if (m_currentFrameFloat >= (float)m_gifFrames.size()) m_currentFrameFloat = 0.0f;
        lastTime = currentTime;
    }
}

/**
 * @brief Converts WIC Bitmap data into an immutable D3D11 Texture.
 */
HRESULT GuiControl::CreateTextureFromWICBitmap(IWICBitmapSource* pBitmapSource, ID3D11ShaderResourceView** ppSRV)
{
    if (!g_pd3dDevice || !g_pWICFactory || !pBitmapSource || !ppSRV) return E_INVALIDARG;

    UINT width, height;
    pBitmapSource->GetSize(&width, &height);

    IWICFormatConverter* pConverter = nullptr;
    g_pWICFactory->CreateFormatConverter(&pConverter);
    
    HRESULT hr = pConverter->Initialize(pBitmapSource, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { pConverter->Release(); return hr; }

    UINT stride = width * 4;
    std::vector<BYTE> pixels(stride * height);
    hr = pConverter->CopyPixels(nullptr, stride, (UINT)pixels.size(), pixels.data());
    pConverter->Release();
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width; desc.Height = height;
    desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = { pixels.data(), stride, 0 };
    ID3D11Texture2D* pTexture = nullptr;
    hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, &pTexture);
    if (FAILED(hr)) return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, ppSRV);
    pTexture->Release();
    return hr;
}

bool GuiControl::LoadFont(const std::wstring& Font_path)
{
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    std::string font_utf8 = UtilsForString::WStringToUTF8(Font_path);
    ImFont* font = io.Fonts->AddFontFromFileTTF(font_utf8.c_str(), 18.0f, NULL, io.Fonts->GetGlyphRangesKorean());
    return font != nullptr;
}

void GuiControl::SetMafiaStyle()
{
    // Define the "Mafia Noir" Visual Identity: Deep Charcoal and Crimson Blood accents
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.1f, 0.1f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
}

void GuiControl::RestoreStyle()
{
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(10);
}
