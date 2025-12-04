// 로그인 화면 GUI
#pragma once
#include"../basegui/BaseGui.h"

class LoginGui :public BaseGui
{
public:
	using BaseGui::BaseGui;
	void Render() override;  // 아이디/비밀번호 입력 및 로그인 버튼
};