#pragma once
#include <PxPhysicsAPI.h>

// 표면 타입 (발소리, 파티클 효과 등에 사용)
enum class ESurfaceType : uint8
{
    Default,
    Metal,
    Wood,
    Stone,
    Grass,
    Dirt,
    Water,
    Glass,
    Flesh,
    Custom1,
    Custom2,
    Custom3
};

// 마찰/반발력 합산 방식
enum class EFrictionCombineMode : uint8
{
    Average,    // 평균
    Min,        // 최소값
    Multiply,   // 곱
    Max         // 최대값
};

class UPhysicalMaterial : public UResourceBase
{
public:
    // --- 실제 PhysX 객체 ---
    physx::PxMaterial* MatHandle = nullptr;

    // --- 기본 물리 속성 ---
    float StaticFriction = 0.5f;    // 정지 마찰력
    float DynamicFriction = 0.5f;   // 운동 마찰력
    float Restitution = 0.3f;       // 반발 계수 (0=완전 비탄성, 1=완전 탄성)
    float Density = 1000.0f;        // 밀도 (kg/m^3) - 물 = 1000

    // --- 합산 방식 ---
    EFrictionCombineMode FrictionCombineMode = EFrictionCombineMode::Average;
    EFrictionCombineMode RestitutionCombineMode = EFrictionCombineMode::Average;

    // --- 표면 타입 ---
    ESurfaceType SurfaceType = ESurfaceType::Default;

    // --- 고급 설정 ---
    bool bOverrideFrictionCombineMode = false;
    bool bOverrideRestitutionCombineMode = false;

    // --- 생성자/소멸자 ---
    UPhysicalMaterial() = default;
    ~UPhysicalMaterial() override { Release(); }

    // 생성 함수
    void CreateMaterial();

    // 속성 업데이트 (런타임 중 변경 시)
    void UpdateMaterial();

    // 해제 함수
    void Release();

    // --- 프리셋 생성 함수 ---
    static UPhysicalMaterial* CreateDefaultMaterial();
    static UPhysicalMaterial* CreateMetalMaterial();
    static UPhysicalMaterial* CreateWoodMaterial();
    static UPhysicalMaterial* CreateRubberMaterial();
    static UPhysicalMaterial* CreateIceMaterial();

private:
    physx::PxCombineMode::Enum ToPxCombineMode(EFrictionCombineMode Mode) const;
};
