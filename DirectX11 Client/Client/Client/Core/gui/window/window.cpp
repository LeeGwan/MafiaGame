// GUI 윈도우 구현부
#include "window.h"
#include "../guicontrol/GuiControl.h"

window::window()
{

}

window::~window()
{
}

// 윈도우 프로시저 (Win32 메시지 처리)
LRESULT __stdcall window::SWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool isMinimized = false;   // 최소화 상태
    static bool isDragging = false;    // 드래그 중 여부
    static POINT dragStartPoint;       // 드래그 시작 지점
    static POINT windowStartPoint;     // 윈도우 시작 위치

    // ImGui 메시지 처리 (마우스, 키보드 입력 등)
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {

    case WM_SYSCOMMAND:
        // ALT 메뉴 비활성화
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;

        // 최소화 처리
        if (wParam == SC_MINIMIZE)
        {
            ::ShowWindow(hWnd, SW_MINIMIZE);
            isMinimized = true;
            return 0;
        }
        break;

        // 크기 조정 시작
    case WM_ENTERSIZEMOVE:
        isMinimized = false;
        ::ShowWindow(hWnd, SW_RESTORE);
        return 0;

        // 크기 조정 종료
    case WM_EXITSIZEMOVE:
        return 0;

        // 히트 테스트 (리사이즈, 드래그 처리)
    case WM_NCHITTEST:
    {
        LRESULT hitTest = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
        if (hitTest == HTCAPTION || hitTest == HTSYSMENU)
            return HTCAPTION;
        else
            return hitTest;
    }

    // 윈도우 닫기
    case WM_CLOSE:
        ::DestroyWindow(hWnd);
        return 0;

        // 윈도우 파괴
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

        // 마우스 왼쪽 버튼 누름 (드래그 시작)
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
            dragStartPoint.x = cursorPos.x - windowRect.left;
            dragStartPoint.y = cursorPos.y - windowRect.top;

            isDragging = true;
        }
    }
    return 0;

    // 마우스 왼쪽 버튼 떼기 (드래그 종료)
    case WM_LBUTTONUP:
    {
        isDragging = false;
        ReleaseCapture();
    }
    return 0;

    // 마우스 이동 (드래그 중이면 윈도우 이동)
    case WM_MOUSEMOVE:
    {
        if (!isMinimized && isDragging)
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            SetWindowPos(hWnd, nullptr,
                cursorPos.x - dragStartPoint.x,
                cursorPos.y - dragStartPoint.y,
                0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    return 0;
    }

    // 기본 메시지 처리
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// GUI 메시지 루프
void window::Update(HINSTANCE hInstance)
{
    // 윈도우 초기화
    if (!init(hInstance))return;
    done.store(true);

    // 메시지 루프
    while (done.load())
    {
        MSG msg;

        // 메시지 큐에서 메시지 가져오기
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);  // 가상 키 메시지를 문자 메시지로 변환
            ::DispatchMessage(&msg);   // 윈도우 프로시저로 전달

            // WM_QUIT 메시지 처리
            if (msg.message == WM_QUIT)
                done.store(false);
        }

        // 메시지 루프 종료 확인
        if (!done.load())
            break;

        // GUI 렌더링
        G_GuiControl->Render();

        // 화면 표시
        G_GuiControl->Presents();
    }
}

// 윈도우 초기화
bool window::init(HINSTANCE hInstance)
{
    std::wstring ClassName = L"MafiaClient";

    // WNDCLASSEXW 구조체 초기화
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

    // 윈도우 클래스 등록
    ::RegisterClassExW(&Wc);
    ::RegisterClassExW(&Wc);

    // 윈도우 크기 설정
    WindowSize = ImVec2(1280, 800);

    // 윈도우 생성 (팝업 스타일, 테두리 없음)
    Hwnd = ::CreateWindowW(Wc.lpszClassName, ClassName.c_str(), WS_POPUP | CW_USEDEFAULT, 0, 0,
        WindowSize.x,
        WindowSize.y, NULL, NULL, Wc.hInstance, NULL);

    // 윈도우 생성 실패 처리
    if (!Hwnd)
    {
        DWORD error = GetLastError();
        char buffer[256];
        sprintf_s(buffer, "CreateWindowW failed. Error code: %lu", error);
        MessageBoxA(NULL, buffer, "ERROR", MB_OK);
        return false;
    }

    RECT rc = { 0 };

    // 윈도우 크기 가져오기
    if (!GetWindowRect(Hwnd, &rc))
    {
        MessageBoxA(NULL, "Faild to GetWindowRect", "ERROR", MB_ERR_INVALID_CHARS);
        return false;
    }

    // 화면 중앙에 배치
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = static_cast<int>(WindowSize.x);
    int windowHeight = static_cast<int>(WindowSize.y);
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;


    ::SetWindowPos(Hwnd, NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);


    ::ShowWindow(Hwnd, SW_SHOWDEFAULT);

    // 윈도우 업데이트
    if (!::UpdateWindow(Hwnd))
    {
        MessageBoxA(NULL, "Faild to UpdateWindow", "ERROR", MB_ERR_INVALID_CHARS);
        return false;
    }

    // GUI 시스템 초기화 (ImGui, DirectX 등)
    if (!G_GuiControl->Initialize(Hwnd))
    {
        MessageBoxA(NULL, "Failed to initialize GUI", "ERROR", MB_OK);
        return false;
    }
    return true;
}

// 리소스 정리
void window::Release()
{
    // GUI 시스템 정리
    G_GuiControl->Cleanup();
    done.store(false);
}