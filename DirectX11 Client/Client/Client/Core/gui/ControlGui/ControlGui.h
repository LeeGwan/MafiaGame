// 윈도우 제어 GUI (최소화, 닫기, 뒤로가기 버튼)
#pragma once
#include"../basegui/BaseGui.h"

class ControlGui :public BaseGui
{
public:
	using BaseGui::BaseGui;
	void Render() override;  // 최소화, 닫기, 뒤로가기 버튼
};