#pragma once
#include "Object.h"
#include "UBodySetupCore.generated.h"

/**
 * EPhysicsType
 *
 * 물리 시뮬레이션 타입
 */
UENUM()
enum class EPhysicsType : uint8
{
    // 기본값 - OwnerComponent의 설정을 상속
    Default,
    // 물리 시뮬레이션 적용
    Simulated,
    // 키네마틱 - 물리 영향 안받지만 다른 물체와 상호작용
    Kinematic
};

/**
 * ECollisionTraceFlag
 *
 * 충돌 트레이스 복잡도 설정
 */
UENUM()
enum class ECollisionTraceFlag : uint8
{
    // 기본값 - Simple/Complex 분리 사용
    UseDefault,
    // Simple 콜리전만 사용 (Convex)
    UseSimpleCollision,
    // Complex 콜리전만 사용 (Per-Poly)
    UseComplexCollision,
    // Simple을 Complex로도 사용
    UseSimpleAsComplex,
    // Complex를 Simple로도 사용
    UseComplexAsSimple
};

/**
 * EBodyCollisionResponse
 *
 * 바디의 충돌 응답 타입
 */
UENUM()
namespace EBodyCollisionResponse
{
    enum Type : uint8
    {
        // 충돌 활성화
        BodyCollision_Enabled,
        // 충돌 비활성화
        BodyCollision_Disabled
    };
}

/**
 * UBodySetupCore
 *
 * UBodySetup의 기본 클래스.
 * 본 이름과 기본 물리/충돌 설정을 정의.
 */
UCLASS(DisplayName="바디 셋업 코어", Description="물리 바디의 기본 설정")
class UBodySetupCore : public UObject
{
    GENERATED_REFLECTION_BODY()

public:
    // --- 본 정보 ---

    // PhysicsAsset에서 사용. 스켈레탈 메시의 본과 연결.
    UPROPERTY(EditAnywhere, Category="BodySetup")
    FName BoneName = "None";

    // --- 물리 타입 ---

    // 시뮬레이션 타입 (Simulated, Kinematic, Default)
    UPROPERTY(EditAnywhere, Category="Physics")
    EPhysicsType PhysicsType = EPhysicsType::Default;

    // --- 충돌 설정 ---

    // 충돌 트레이스 복잡도 (Simple/Complex)
    UPROPERTY(EditAnywhere, Category="Collision")
    ECollisionTraceFlag CollisionTraceFlag = ECollisionTraceFlag::UseDefault;

    // 충돌 응답 타입
    UPROPERTY(EditAnywhere, Category="Collision")
    EBodyCollisionResponse::Type CollisionResponse = EBodyCollisionResponse::BodyCollision_Enabled;

    // --- 생성자/소멸자 ---
    UBodySetupCore();
    virtual ~UBodySetupCore();

    // --- 유틸리티 ---

    ECollisionTraceFlag GetCollisionTraceFlag() const { return CollisionTraceFlag; }

    // --- 직렬화 ---
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
