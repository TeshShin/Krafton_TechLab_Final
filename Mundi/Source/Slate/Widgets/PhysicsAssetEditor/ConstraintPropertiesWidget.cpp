#include "pch.h"
#include "ConstraintPropertiesWidget.h"
#include "ImGui/imgui.h"
#include "Source/Runtime/Engine/Viewer/PhysicsAssetEditorState.h"
#include "Source/Runtime/Engine/PhysicsEngine/PhysicsAsset.h"
#include "Source/Runtime/Engine/PhysicsEngine/FConstraintSetup.h"
#include "Source/Slate/Widgets/PropertyRenderer.h"

IMPLEMENT_CLASS(UConstraintPropertiesWidget);

UConstraintPropertiesWidget::UConstraintPropertiesWidget()
	: UWidget("ConstraintPropertiesWidget")
{
}

void UConstraintPropertiesWidget::Initialize()
{
	bWasModified = false;
}

void UConstraintPropertiesWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

void UConstraintPropertiesWidget::RenderWidget()
{
	if (!EditorState || !EditorState->EditingAsset) return;

	// 선택된 제약조건이 없으면 렌더링하지 않음
	if (EditorState->bBodySelectionMode || EditorState->SelectedConstraintIndex < 0) return;
	if (EditorState->SelectedConstraintIndex >= static_cast<int32>(EditorState->EditingAsset->ConstraintSetups.size())) return;

	FConstraintSetup& Constraint = EditorState->EditingAsset->ConstraintSetups[EditorState->SelectedConstraintIndex];
	bWasModified = false;

	ImGui::Text("Constraint: %s", Constraint.JointName.ToString().c_str());
	ImGui::Separator();

	// FConstraintSetup struct의 reflection 정보 가져오기
	UStruct* StructType = UStruct::FindStruct("FConstraintSetup");
	if (!StructType)
	{
		ImGui::Text("[Error: FConstraintSetup not registered]");
		return;
	}

	// TODO: PropertyRenderer에 RenderStructPropertiesWithCategories() 추가 후 공통 API로 교체
	// 현재는 임시로 Category 그룹핑 로직을 직접 구현

	// 카테고리별 그룹핑
	TArray<TPair<FString, TArray<const FProperty*>>> CategorizedProps;
	TMap<FString, int32> CategoryIndexMap;

	for (const FProperty& Prop : StructType->GetAllProperties())
	{
		// 에디터 전용 필드 스킵
		if (strcmp(Prop.Name, "bSelected") == 0)
			continue;

		if (Prop.bIsEditAnywhere)
		{
			FString CategoryName = Prop.Category ? Prop.Category : "Default";
			int32* IndexPtr = CategoryIndexMap.Find(CategoryName);

			if (IndexPtr)
			{
				CategorizedProps[*IndexPtr].second.Add(&Prop);
			}
			else
			{
				TArray<const FProperty*> NewPropArray;
				NewPropArray.Add(&Prop);
				int32 NewIndex = CategorizedProps.Add(
					TPair<FString, TArray<const FProperty*>>(CategoryName, NewPropArray));
				CategoryIndexMap.Add(CategoryName, NewIndex);
			}
		}
	}

	// 카테고리별 렌더링
	for (auto& Pair : CategorizedProps)
	{
		const FString& Category = Pair.first;
		const TArray<const FProperty*>& Props = Pair.second;

		if (ImGui::CollapsingHeader(Category.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (const FProperty* Prop : Props)
			{
				ImGui::PushID(Prop);

				if (UPropertyRenderer::RenderProperty(*Prop, &Constraint))
				{
					EditorState->bIsDirty = true;
					EditorState->RequestLinesRebuild();
					bWasModified = true;
				}

				ImGui::PopID();
			}
		}
	}
}
