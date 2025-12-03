#pragma once

#include "ELinearConstraintMotion.h"
#include "EAngularConstraintMotion.h"
#include "FConstraintSetup.generated.h"

/**
 * FConstraintSetup
 *
 * 두 바디 간의 물리 제약 조건 설정
 * Unreal Engine 스타일의 Motion 기반 설계
 */
USTRUCT(DisplayName="제약 조건 설정", Description="두 바디 간의 물리 제약 조건 설정")
struct FConstraintSetup
{
	GENERATED_REFLECTION_BODY()

public:
	// ────────────────────────────────────────────────
	// 기본 정보
	// ────────────────────────────────────────────────
	UPROPERTY()
	FName JointName;

	// ────────────────────────────────────────────────
	// 연결된 바디
	// ────────────────────────────────────────────────
	UPROPERTY()
	int32 ParentBodyIndex = -1;

	UPROPERTY()
	int32 ChildBodyIndex = -1;

	// ────────────────────────────────────────────────
	// Linear Constraint (위치 제한)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="X축 선형 이동 타입")
	ELinearConstraintMotion LinearXMotion = ELinearConstraintMotion::Locked;

	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="Y축 선형 이동 타입")
	ELinearConstraintMotion LinearYMotion = ELinearConstraintMotion::Locked;

	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="Z축 선형 이동 타입")
	ELinearConstraintMotion LinearZMotion = ELinearConstraintMotion::Locked;

	UPROPERTY(EditAnywhere, Category="Linear", Tooltip="선형 이동 제한 거리 (cm)")
	float LinearLimit = 0.0f;

	// ────────────────────────────────────────────────
	// Angular Constraint - Swing (Cone 제한)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing1 (Y축 회전) 타입")
	EAngularConstraintMotion Swing1Motion = EAngularConstraintMotion::Limited;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing2 (Z축 회전) 타입")
	EAngularConstraintMotion Swing2Motion = EAngularConstraintMotion::Limited;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing1 제한 각도 (degrees)")
	float Swing1LimitDegrees = 45.0f;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Swing2 제한 각도 (degrees)")
	float Swing2LimitDegrees = 45.0f;

	// ────────────────────────────────────────────────
	// Angular Constraint - Twist (X축 회전)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Twist (X축 회전) 타입")
	EAngularConstraintMotion TwistMotion = EAngularConstraintMotion::Limited;

	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="Twist 제한 각도 (±degrees, 대칭)")
	float TwistLimitDegrees = 45.0f;

	// ────────────────────────────────────────────────
	// Drive (스프링/댐퍼)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Drive", Tooltip="스프링 강성")
	float Stiffness = 0.0f;

	UPROPERTY(EditAnywhere, Category="Drive", Tooltip="감쇠 계수")
	float Damping = 0.0f;

	// ────────────────────────────────────────────────
	// Constraint Frame - Child (Body1)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="자식 바디 로컬 연결점 위치")
	FVector ChildPosition = FVector::Zero();

	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="자식 프레임 회전 (Roll, Pitch, Yaw degrees)")
	FVector ChildRotation = FVector::Zero();

	// ────────────────────────────────────────────────
	// Constraint Frame - Parent (Body2)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="부모 바디 로컬 연결점 위치")
	FVector ParentPosition = FVector::Zero();

	UPROPERTY(EditAnywhere, Category="Frame", Tooltip="부모 프레임 회전 (Roll, Pitch, Yaw degrees)")
	FVector ParentRotation = FVector::Zero();

	// ────────────────────────────────────────────────
	// Angular Rotation Offset (비대칭 Limit용)
	// ────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Angular", Tooltip="각도 제한 중심점 오프셋 (Roll, Pitch, Yaw degrees)")
	FVector AngularRotationOffset = FVector::Zero();

	// ────────────────────────────────────────────────
	// 에디터 상태 (직렬화 제외)
	// ────────────────────────────────────────────────
	bool bSelected = false;

	// ────────────────────────────────────────────────
	// 생성자
	// ────────────────────────────────────────────────
	FConstraintSetup() = default;

	FConstraintSetup(const FName& InJointName, int32 InParentIndex, int32 InChildIndex)
		: JointName(InJointName)
		, ParentBodyIndex(InParentIndex)
		, ChildBodyIndex(InChildIndex)
	{}

	// ────────────────────────────────────────────────
	// 헬퍼: Linear motion 중 하나라도 Limited인지
	// ────────────────────────────────────────────────
	bool HasLinearLimit() const
	{
		return LinearXMotion == ELinearConstraintMotion::Limited ||
		       LinearYMotion == ELinearConstraintMotion::Limited ||
		       LinearZMotion == ELinearConstraintMotion::Limited;
	}

	// ────────────────────────────────────────────────
	// 헬퍼: Angular motion 중 하나라도 Limited인지
	// ────────────────────────────────────────────────
	bool HasAngularLimit() const
	{
		return Swing1Motion == EAngularConstraintMotion::Limited ||
		       Swing2Motion == EAngularConstraintMotion::Limited ||
		       TwistMotion == EAngularConstraintMotion::Limited;
	}
};
