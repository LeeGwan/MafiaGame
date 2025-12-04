// GUI 베이스 클래스 (모든 GUI 화면의 부모 클래스)
#pragma once
#include "../../../Dependencies/Imgui/imgui.h"

class GuiControl;  // GUI 컨트롤러

// 추상 클래스 (순수 가상 함수 Render 포함)
class BaseGui
{
public:
	// 생성자: parentGui 포인터 저장
	explicit BaseGui(GuiControl* parent) : parentGui(parent) {}
	virtual ~BaseGui() = default;

	// 순수 가상 함수 (자식 클래스에서 반드시 구현)
	virtual void Render() = 0;

protected:
	GuiControl* parentGui;  // GUI 컨트롤러 참조 (SetUitype, SignIn 등 호출용)

	// 기본 패널 크기
	const ImVec2 panelSize = ImVec2(350, 280);      // 일반 패널 크기
	const ImVec2 InitpanelSize = ImVec2(1020, 680); // 초기 화면 패널 크기
};