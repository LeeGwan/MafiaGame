/**
 * @file window.cpp
 * @brief Implementation of the Win32 window abstraction and message loop.
 */

#include "window.h"
#include "../guicontrol/GuiControl.h"

// Forward declaration of the ImGui Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

window::window() {}
window::~window() {}

/**
 * @brief Static Window Procedure (Callback) to process Win32 messages.
 * Integrates ImGui input handling and implements custom frameless window dragging.
 */
LRESULT __stdcall window::SWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool isMinimized = false;      // Tracking minimization state
    static bool isDragging = false;       // Flag for custom window dragging
    static POINT dragStartPoint;          // Initial click offset within the window
    static POINT windowStartPoint;        // Initial cursor position

    // Pass messages to ImGui (Mouse, Keyboard, etc.)
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SYSCOMMAND:
        // Disable the ALT menu (System Menu) to prevent unintended focus shifts
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;

        // Custom minimization handling
        if (wParam == SC_MINIMIZE)
        {
            ::ShowWindow(hWnd, SW_MINIMIZE);
            isMinimized = true;
            return 0;
        }
        break;

    case WM_ENTERSIZEMOVE:
        isMinimized = false;
        ::ShowWindow(hWnd, SW_RESTORE);
        return 0;

    case WM_EXITSIZEMOVE:
        return 0;

    /**
     * @section Hit_Testing
     * Ensures the entire top area behaves like a title bar for system drag-and-drop.
     */
    case WM_NCHITTEST:
    {
        LRESULT hitTest = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
        if (hitTest == HTCAPTION || hitTest == HTSYSMENU)
            return HTCAPTION;
        else
            return hitTest;
    }

    case WM_CLOSE:
        ::DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    /**
     * @section Custom_Dragging_Logic
     * Manual window repositioning logic for WS_POPUP (Frameless) style windows.
     */
    case WM_LBUTTONDOWN:
    {
        if (!isMinimized)
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            windowStartPoint.x = cursorPos.x;
            windowStartPoint.y = cursorPos.y;

            RECT windowRect;
            GetWindowRect(hWnd, &windowRect);
            
            // Calculate offset between cursor and window top-left corner
            dragStartPoint.x = cursorPos.x - windowRect.left;
            dragStartPoint.y = cursorPos.y - windowRect.top;

            isDragging = true;
            SetCapture(hWnd); // Ensure mouse events are captured during movement
        }
    }
    return 0;

    case WM_LBUTTONUP:
    {
        isDragging = false;
        ReleaseCapture();
    }
    return 0;

    case WM_MOUSEMOVE:
    {
        if (!isMinimized && isDragging)
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            // Dynamically reposition the window based on cursor movement
            SetWindowPos(hWnd, nullptr,
                cursorPos.x - dragStartPoint.x,
                cursorPos.y - dragStartPoint.y,
                0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    return 0;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

/**
 * @brief High-performance message loop and real-time rendering entry point.
 * @param hInstance Instance handle from WinMain.
 */
void window::Update(HINSTANCE hInstance)
{
    // Bootstrap the Win32 window components
    if (!init(hInstance)) return;
    done.store(true);

    // Monolithic Rendering and Message Loop
    while (done.load())
    {
        MSG msg;

        // Non-blocking message polling for high-throughput rendering
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                done.store(false);
        }

        if (!done.load())
            break;

        // Orchestrate GUI Rendering and D3D11 Presentation
        G_GuiControl->Render();
        G_GuiControl->Presents();
    }
}

/**
 * @brief Configures and spawns the main Win32 window with WS_POPUP style.
 */
bool window::init(HINSTANCE hInstance)
{
    std::wstring ClassName = L"MafiaClient";

    // Initialize WNDCLASSEXW structure
    memset(&Wc, 0, sizeof(WNDCLASSEXW));
    Wc.cbSize = sizeof(WNDCLASSEXW);
    Wc.style = CS_CLASSDC;
    Wc.lpfnWndProc = SWndProc;         
    Wc.cbClsExtra = 0L;
    Wc.cbWndExtra = 0L;
    Wc.hInstance = GetModuleHandle(NULL);
    Wc.hIcon = NULL;
    Wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    Wc.hbrBackground = NULL;
    Wc.lpszMenuName = NULL;
    Wc.lpszClassName = ClassName.c_str();
    Wc.hIconSm = NULL;

    // Register window class with the operating system
    if (!::RegisterClassExW(&Wc)) return false;

    // Standard high-definition resolution for the client
    WindowSize = ImVec2(1280, 800);

    // Create a frameless (WS_POPUP) window centered in screen coordinates
    Hwnd = ::CreateWindowW(Wc.lpszClassName, ClassName.c_str(), WS_POPUP, 
        0, 0,
        static_cast<int>(WindowSize.x),
        static_cast<int>(WindowSize.y), 
        NULL, NULL, Wc.hInstance, NULL);

    if (!Hwnd)
    {
        DWORD error = GetLastError();
        char buffer[256];
        sprintf_s(buffer, "CreateWindowW failed. Error code: %lu", error);
        MessageBoxA(NULL, buffer, "Critical Error", MB_OK);
        return false;
    }

    // Centering Logic: Calculate screen-relative coordinates
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - static_cast<int>(WindowSize.x)) / 2;
    int posY = (screenHeight - static_cast<int>(WindowSize.y)) / 2;

    ::SetWindowPos(Hwnd, NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    ::ShowWindow(Hwnd, SW_SHOWDEFAULT);

    if (!::UpdateWindow(Hwnd)) return false;

    // Initialize the D3D11 and ImGui backends via GuiControl
    if (!G_GuiControl->Initialize(Hwnd))
    {
        MessageBoxA(NULL, "Failed to initialize DirectX/GUI context", "Initialization Error", MB_OK);
        return false;
    }

    return true;
}

/** @brief Cleans up core window resources and terminates the loop. */
void window::Release()
{
    G_GuiControl->Cleanup();
    done.store(false);
}
