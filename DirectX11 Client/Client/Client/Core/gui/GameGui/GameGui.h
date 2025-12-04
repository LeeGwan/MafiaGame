// 게임 진행 화면 GUI
#pragma once
#include"../basegui/BaseGui.h"

class GameGui :public BaseGui
{
public:
	using BaseGui::BaseGui;
	void Render() override;  // 게임 중 표시 및 경과 시간
};