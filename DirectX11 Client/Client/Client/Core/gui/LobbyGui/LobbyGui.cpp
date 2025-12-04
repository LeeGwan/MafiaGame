// 로비 화면 구현
#include "LobbyGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

void LobbyGui::Render()
{
    ImGui::Begin("##LobbyPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // 닉네임 입력, 언제든지 바꿀수 있다
    ImGui::InputText("##NICKNAME", parentGui->TempNickName, ID_PW_SIZE);

    // 게임 찾기 버튼 (매칭 시작)
    if (ImGui::Button(u8"게임 찾기", ImVec2(200, 60))) {
        parentGui->JoinRoom(parentGui->GetUserHash(), parentGui->TempNickName);
    }

    // 로그아웃 버튼
    if (ImGui::Button(u8"로그아웃", ImVec2(200, 60))) {
        if (parentGui->IsLogin()) {
            parentGui->LogOut(parentGui->GetUserHash());
        }
    }

    ImGui::End();
}