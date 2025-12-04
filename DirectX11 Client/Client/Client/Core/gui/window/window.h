// GUI 윈도우 관리 (Win32 API + ImGui)
#pragma once
#include <memory>
#include <atomic>
#include <string>
#include"../../../Dependencies/Imgui/imgui_impl_win32.h"
#include"../../../Dependencies/Imgui/imgui.h"

class GuiControl;  // GUI 제어 클래스

class window
{
public:
	window();
	~window();

	// 윈도우 프로시저 (Win32 메시지 처리)
	static LRESULT WINAPI SWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// GUI 메시지 루프 (별도 스레드에서 실행)
	void Update(HINSTANCE hInstance);

	// 리소스 정리
	void Release();

private:
	// 윈도우 초기화 (CreateWindowW, ImGui 초기화)
	bool init(HINSTANCE hInstance);

private:
	std::atomic<bool> done;  // 메시지 루프 실행 플래그
	ImVec2 Size;
	HWND Hwnd;               
	ImVec2 WindowSize;      
	WNDCLASSEXW Wc;          
};