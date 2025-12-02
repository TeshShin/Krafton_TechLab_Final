#pragma once

/**
 * EAngularConstraintMotion
 *
 * Angular constraint의 축별 motion 타입 (Swing1, Swing2, Twist)
 * Unreal Engine 스타일과 동일
 */
UENUM()
enum class EAngularConstraintMotion : uint8
{
	Free    = 0,   // 자유 회전 (제한 없음)
	Limited = 1,   // 제한된 회전 (Limit 적용)
	Locked  = 2,   // 잠금 (회전 불가)
};

/** Motion 타입 이름 반환 */
inline const char* GetAngularConstraintMotionName(EAngularConstraintMotion Motion)
{
	switch (Motion)
	{
	case EAngularConstraintMotion::Free:    return "Free";
	case EAngularConstraintMotion::Limited: return "Limited";
	case EAngularConstraintMotion::Locked:  return "Locked";
	default: return "Unknown";
	}
}
