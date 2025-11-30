#pragma once

UENUM()
enum class EAggCollisionShape : uint8
{
    Sphere,
    Box,
    Sphyl,
    Convex,
    TaperedCapsule,
    LevelSet,
    Unknown
};

UENUM()
enum class ECollisionEnabled : uint8
{
    /** 물리 엔진에 어떠한 표현도 생성하지 않는다.
    레이캐스트, 스윕, 오버랩 같은 공간 쿼리나 강체·제약 같은 시뮬레이션에 사용할 수 없다.
    특히 이동하는 오브젝트에 대해 최고의 성능을 제공한다. */
    NoCollision,
    /** 공간 쿼리(레이캐스트, 스윕, 오버랩)에만 사용된다.
    시뮬레이션(강체, 제약)에는 사용할 수 없다.
    캐릭터 이동이나 물리 시뮬레이션이 필요 없는 요소에 유용하며,
    시뮬레이션 트리에서 데이터를 제외해 성능 이득이 있다. */
    QueryOnly,
    /** 물리 시뮬레이션(강체, 제약)에만 사용된다.
    공간 쿼리(레이캐스트, 스윕, 오버랩)에는 사용할 수 없다.
    본 단위 충돌 검사가 필요 없는 캐릭터의 출렁이는 부분(jiggly bits)에 유용하며,
    쿼리 트리에서 데이터를 제외해 성능 향상이 있다. */
    PhysicsOnly,
    /** 공간 쿼리(레이캐스트, 스윕, 오버랩)와 시뮬레이션(강체, 제약) 둘 다에 사용할 수 있다. */
    QueryAndPhysics
};

USTRUCT()
struct FKShapeElem
{
    FKShapeElem()
        : ShapeType(EAggCollisionShape::Unknown)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
        // , RestOffset(0.0f)
    {
    }

    FKShapeElem(EAggCollisionShape InShapeType)
        : ShapeType(InShapeType)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
        // , RestOffset(0.0f)
    {
    }

    FKShapeElem(const FKShapeElem& Other)
        : ShapeType(Other.ShapeType)
        , Name(Other.Name)
        , bContributeToMass(Other.bContributeToMass)
        , CollisionEnabled(Other.CollisionEnabled)
        // , RestOffset(Other.RestOffset)
    {
    }

    virtual ~FKShapeElem() = default; 

    static inline EAggCollisionShape StaticShapeType = EAggCollisionShape::Unknown;

    const FName& GetName() const { return Name; }

    void SetName(const FName& InName) { Name = InName; }

    EAggCollisionShape GetShapeType() const { return ShapeType; }

    bool GetContributeToMass() const { return bContributeToMass; }

    void SetContributeToMass(bool bInContributeToMass) { bContributeToMass = bInContributeToMass; }

    ECollisionEnabled GetCollisionEnabled() const { return CollisionEnabled; }

    void SetCollisionEnabled(ECollisionEnabled InCollisionEnabled) { CollisionEnabled = InCollisionEnabled; }

    virtual FTransform GetTransform() const { return FTransform();}

private:
    /** 충돌체 타입 */
    EAggCollisionShape ShapeType;

    /** 식별용 이름 */
    FName Name;

    /** 쉐이프가 강체의 질량 계산에 기여할지 여부 */
    bool bContributeToMass;

    /** 충돌 활성화 상태 */
    ECollisionEnabled CollisionEnabled;

    /** PhysX Contact Generation Offset (충돌 마진) */
    // float RestOffset;
};
