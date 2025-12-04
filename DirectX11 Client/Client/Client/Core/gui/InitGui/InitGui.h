// 초기 화면 GUI (로그인/회원가입 선택)
#pragma once
#include"../basegui/BaseGui.h"

class InitGui :public BaseGui
{
public:
	using BaseGui::BaseGui;  // 부모 생성자 상속
	void Render() override;  // 로그인/회원가입 버튼 렌더링
};