// 매칭 대기 화면 GUI
#pragma once
#include"../basegui/BaseGui.h"

class MatchingGui :public BaseGui
{
public:
	using BaseGui::BaseGui;
	void Render() override;  // 매칭 중 표시 및 경과 시간, 취소 버튼
};