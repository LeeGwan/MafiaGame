// 초기 화면 구현
#include "InitGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

void InitGui::Render()
{
    // 초기 화면 패널 생성
    ImGui::Begin("##InitPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // 버튼 크기 및 간격 설정
    ImVec2 buttonSize = ImVec2(290, 130);
    float spacing = 40.0f;
    float totalWidth = buttonSize.x * 2 + spacing;

    // 버튼 위치 설정 (화면 중앙)
    ImGui::SetCursorPosX((1020 - totalWidth) * 0.5f);
    ImGui::SetCursorPosY(340);

    // 로그인 버튼
    if (ImGui::Button(u8"로그인", buttonSize)) {
        parentGui->SetUitype(EUIType::Login);
    }

    ImGui::SameLine(0, spacing);

    // 회원가입 버튼
    if (ImGui::Button(u8"회원가입", buttonSize)) {
        parentGui->SetUitype(EUIType::Register);
    }

    ImGui::End();
}