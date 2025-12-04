// 회원가입 화면 GUI
#pragma once
#include"../basegui/BaseGui.h"

class RegisterGui :public BaseGui
{
public:
	using BaseGui::BaseGui;
	void Render() override;  // 아이디/비밀번호/비밀번호 확인 입력 및 회원가입 버튼
};