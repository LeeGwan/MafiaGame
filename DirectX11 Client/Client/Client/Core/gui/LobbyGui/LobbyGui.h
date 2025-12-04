// 로비 화면 GUI
#pragma once
#include"../basegui/BaseGui.h"

class LobbyGui :public BaseGui
{
public:
	using BaseGui::BaseGui;
	void Render() override;  // 닉네임 입력, 게임 찾기, 로그아웃 버튼
};