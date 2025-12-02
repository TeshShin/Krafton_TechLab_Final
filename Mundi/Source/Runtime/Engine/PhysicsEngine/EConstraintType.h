#pragma once

/**
 * EConstraintType (DEPRECATED)
 *
 * 이 enum은 더 이상 사용되지 않습니다.
 * 대신 ELinearConstraintMotion과 EAngularConstraintMotion을 사용하세요.
 *
 * @deprecated Use ELinearConstraintMotion and EAngularConstraintMotion instead
 */

// DEPRECATED - 하위 호환성을 위해 유지
UENUM()
enum class EConstraintType : uint8
{
	BallAndSocket,  // 구형 조인트 (3축 회전)
	Hinge,          // 힌지 조인트 (1축 회전)
};

[[deprecated("Use ELinearConstraintMotion and EAngularConstraintMotion instead")]]
inline const char* GetConstraintTypeName(EConstraintType Type)
{
	switch (Type)
	{
	case EConstraintType::BallAndSocket: return "Ball & Socket";
	case EConstraintType::Hinge: return "Hinge";
	default: return "Unknown";
	}
}
