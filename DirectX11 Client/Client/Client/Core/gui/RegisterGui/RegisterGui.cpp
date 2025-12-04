// 회원가입 화면 구현
#include "RegisterGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

void RegisterGui::Render()
{
    ImGui::Begin("##RegisterPanel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SetCursorPosY(300);

    // 아이디 입력
    ImGui::Text(u8"아이디:");
    ImGui::InputText("##ID", parentGui->TempId, ID_PW_SIZE);

    // 비밀번호 입력
    ImGui::Text(u8"비밀번호:");
    ImGui::InputText("##Password", parentGui->TempPw, ID_PW_SIZE, ImGuiInputTextFlags_Password);

    // 비밀번호 확인 입력
    ImGui::Text(u8"비밀번호확인:");
    ImGui::InputText("##CheckPassword", parentGui->TempCheckPw, ID_PW_SIZE, ImGuiInputTextFlags_Password);

    // 회원가입 버튼 (비밀번호 일치 여부는 SignUp에서 확인)
    if (ImGui::Button(u8"회원가입", ImVec2(290, 40))) {
        parentGui->SignUp(parentGui->TempId, parentGui->TempPw, parentGui->TempCheckPw);
    }

    ImGui::End();
}