// 로그인 화면 구현
#include "LoginGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

void LoginGui::Render()
{
    ImGui::Begin("##LoginPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SetCursorPosY(300);

    // 아이디 입력
    ImGui::Text(u8"아이디:");
    ImGui::InputText("##ID", parentGui->TempId, ID_PW_SIZE);

    // 비밀번호 입력 (마스킹 처리)
    ImGui::Text(u8"비밀번호:");
    ImGui::InputText("##Password", parentGui->TempPw, ID_PW_SIZE, ImGuiInputTextFlags_Password);

    // 로그인 버튼
    if (ImGui::Button(u8"로그인", ImVec2(290, 40))) {
        parentGui->SignIn(parentGui->TempId, parentGui->TempPw);
    }

    ImGui::End();
}