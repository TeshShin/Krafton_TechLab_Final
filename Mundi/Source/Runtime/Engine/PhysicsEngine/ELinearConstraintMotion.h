#pragma once

/**
 * ELinearConstraintMotion
 *
 * Linear constraint의 축별 motion 타입
 * Unreal Engine 스타일과 동일
 */
UENUM()
enum class ELinearConstraintMotion : uint8
{
	Free    = 0,   // 자유 이동 (제한 없음)
	Limited = 1,   // 제한된 이동 (LinearLimit 적용)
	Locked  = 2,   // 잠금 (이동 불가)
};

/** Motion 타입 이름 반환 */
inline const char* GetLinearConstraintMotionName(ELinearConstraintMotion Motion)
{
	switch (Motion)
	{
	case ELinearConstraintMotion::Free:    return "Free";
	case ELinearConstraintMotion::Limited: return "Limited";
	case ELinearConstraintMotion::Locked:  return "Locked";
	default: return "Unknown";
	}
}
