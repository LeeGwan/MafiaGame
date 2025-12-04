// 언리얼 게임 프로세스 실행 구현
#include "ProcessManager.h"
#include "../Utils/UtilsForString.h"
#include "../gui/guicontrol/GuiControl.h"
#include"../AntiCheat/AntiCheat.h"
#include <windows.h>
#include <fstream>
#include <filesystem>
std::unique_ptr<ProcessManager> G_ProcessManager = std::make_unique<ProcessManager>();

// 언리얼 클라이언트 실행 (커맨드라인으로 Hash, NickName, IP, Port 전달)
void ProcessManager::ProcessRunner(const std::string& IP, uint16_t port)
{
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::wstring GameEXE_path = currentPath.wstring() + L"\\MafiaGame\\MafiaGameClient.exe";
	std::wstring GameEXE_MakeWindow = GameEXE_path + L" -game -windowed -ResX=640 -ResY=480";
	std::wstring GameEXE_Full = GameEXE_MakeWindow + L" Hash=" + UtilsForString::UTF8ToWString(G_GuiControl->GetUserHash(), CP_ACP)
		+ L"?NickName=" + UtilsForString::UTF8ToWString(G_GuiControl->TempNickName, CP_ACP)
		+ L"?IP=" + UtilsForString::UTF8ToWString(IP, CP_ACP)
		+ L":" + std::to_wstring(port);

	std::vector<wchar_t> commandLine(GameEXE_Full.begin(), GameEXE_Full.end());
	commandLine.push_back(L'\0');

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (!CreateProcessW(NULL, commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		MessageBoxA(NULL, "실패", "실패", 0);
		return;
	}

	// 안티치트에 보호 PID 등록
	G_AntiCheat->AddProtectedPID(pi.dwProcessId);
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitCode;
	if (GetExitCodeProcess(pi.hProcess, &exitCode))
	{
		switch (exitCode)
		{
		case 0:
			break;
		case 1:
			break;
		case STILL_ACTIVE:
			break;
		case 3:
			break;
		case -1:
			break;
		default:
			break;
		}
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}