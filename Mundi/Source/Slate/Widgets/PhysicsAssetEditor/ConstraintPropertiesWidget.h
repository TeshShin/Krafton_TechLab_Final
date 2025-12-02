#pragma once

#include "Source/Slate/Widgets/Widget.h"

struct PhysicsAssetEditorState;
struct FConstraintSetup;

/**
 * UConstraintPropertiesWidget
 *
 * Physics Asset Editor의 우측 패널 (제약 조건 선택 시)
 * Reflection 기반으로 FConstraintSetup 프로퍼티 편집
 *
 * UWidget 기반 인스턴스로 에디터 상태와 연결
 */
class UConstraintPropertiesWidget : public UWidget
{
public:
	DECLARE_CLASS(UConstraintPropertiesWidget, UWidget)

	UConstraintPropertiesWidget();
	virtual ~UConstraintPropertiesWidget() = default;

	// UWidget 인터페이스
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void RenderWidget() override;

	// 외부 상태 연결
	void SetEditorState(PhysicsAssetEditorState* InState) { EditorState = InState; }
	PhysicsAssetEditorState* GetEditorState() const { return EditorState; }

	// 속성이 변경되었는지 확인
	bool WasModified() const { return bWasModified; }
	void ClearModifiedFlag() { bWasModified = false; }

private:
	PhysicsAssetEditorState* EditorState = nullptr;
	bool bWasModified = false;
};
