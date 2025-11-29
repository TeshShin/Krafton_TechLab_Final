#pragma once

#include "AggregateGeom.h"
#include "UBodySetup.generated.h"

struct FBodyInstance;

UCLASS()
class UBodySetup : public UObject
{
    GENERATED_REFLECTION_BODY()
public:
    UBodySetup();

    virtual ~UBodySetup();

    /** 단순화된 충돌 표현 */
    FKAggregateGeom AggGeom;

    /** 밀도, 마찰 등과 관련된 정보를 포함하는 물리 재질 */
    PxMaterial* PhysMaterial;

    void AddShapesToRigidActor_AssumesLocked(
        FBodyInstance* OwningInstance,
        const FVector& Scale3D,
        PxRigidActor* PDestActor);

    /** 테스트용 함수 (박스 추가) */
    void AddBox(const FVector& Extent);
};
