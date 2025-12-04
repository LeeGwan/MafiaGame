// 윈도우 제어 GUI 구현
#include "ControlGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

void ControlGui::Render()
{
    ImGuiIO& io = ImGui::GetIO();

    // 오른쪽 상단에 제어 버튼 배치
    ImVec2 controlButtonPos = ImVec2(io.DisplaySize.x - 80, 10);
    ImGui::SetNextWindowPos(controlButtonPos);
    ImGui::SetNextWindowSize(ImVec2(70, 35));

    ImGui::Begin("##ControlButtons", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    // 버튼 색상 설정
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 0.9f));

    // 최소화 버튼
    if (ImGui::Button("_", ImVec2(25, 25))) {
        if (parentGui->m_mainWindow) {
            ShowWindow(parentGui->m_mainWindow, SW_MINIMIZE);
        }
    }

    ImGui::SameLine();

    // 닫기 버튼
    if (ImGui::Button("X", ImVec2(25, 25))) {
        if (parentGui->m_mainWindow) {
            // 로그인 상태면 로그아웃 처리 (현재 주석 처리)
            if (parentGui->IsLogin()) {
                //   G_core->get_C_eventmanager()->trigger(EventType::LOGOUT_EVENT, false, parentGui->GetUserHash());
            }
            PostMessage(parentGui->m_mainWindow, WM_CLOSE, 0, 0);
        }
    }

    ImGui::PopStyleColor(2);
    ImGui::End();

    // 뒤로가기 버튼 (Init, Lobby 화면 제외)
    if (parentGui->currentUitype.load() != EUIType::Init &&
        parentGui->currentUitype.load() != EUIType::Lobby) {

        ImVec2 backButtonPos = ImVec2(10, io.DisplaySize.y - 50);
        ImGui::SetNextWindowPos(backButtonPos);
        ImGui::SetNextWindowSize(ImVec2(90, 40));

        ImGui::Begin("##BackButton", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

        if (ImGui::Button(u8"뒤로가기", ImVec2(80, 30))) {
            parentGui->SetUitype(EUIType::Init);
        }

        ImGui::End();
    }
}