// 게임 진행 화면 구현
#include "GameGui.h"
#include "../guicontrol/GuiControl.h"
#include "../EUIType/EUIType.h"

void GameGui::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImVec2 matchingWindowSize = ImVec2(400, 200);

    // 화면 중앙에 게임 창 배치
    ImGui::SetNextWindowPos(ImVec2(center.x - matchingWindowSize.x * 0.5f, center.y - matchingWindowSize.y * 0.5f));
    ImGui::SetNextWindowSize(matchingWindowSize);
    ImGui::SetNextWindowFocus();

    // 스타일 설정
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

    ImGui::Begin("##GameWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_Modal);

    // "게임 중..." 텍스트
    ImGui::SetCursorPosX((matchingWindowSize.x - ImGui::CalcTextSize(u8"게임 중...").x) * 0.5f);
    ImGui::SetCursorPosY(40);
    ImGui::Text(u8"게임 중...");

    // 경과 시간 표시
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - parentGui->m_GameStartTime);
    std::string timeStr = std::to_string(elapsedTime.count()) + u8"초 경과";
    ImGui::SetCursorPosX((matchingWindowSize.x - ImGui::CalcTextSize(timeStr.c_str()).x) * 0.5f);
    ImGui::SetCursorPosY(80);
    ImGui::Text(timeStr.c_str());

    ImVec2 cancelButtonSize = ImVec2(120, 40);
    ImGui::SetCursorPosX((matchingWindowSize.x - cancelButtonSize.x) * 0.5f);
    ImGui::SetCursorPosY(130);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(3);
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}