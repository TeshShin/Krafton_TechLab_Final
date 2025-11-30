#include "pch.h"
#include "BodyInstance.h"
#include "PhysicalMaterial.h"
#include "PhysXConversion.h"
#include "BodySetup.h"
using namespace physx;

// --- 생성자/소멸자 ---

FBodyInstance::FBodyInstance()
    : OwnerComponent(nullptr)
    , bIsTrigger(false)
{
}

FBodyInstance::FBodyInstance(UPrimitiveComponent* Owner)
    : OwnerComponent(Owner)
    , bIsTrigger(false)
{
}

FBodyInstance::~FBodyInstance()
{
    TermBody();
}

// --- 초기화/해제 ---

void FBodyInstance::InitBody(const FTransform& Transform, const PxGeometry& Geometry, UPhysicalMaterial* PhysMat)
{
    // 이미 있으면 삭제하고 다시 생성
    TermBody();

    FPhysicsSystem& System = FPhysicsSystem::Get();
    PxPhysics* Physics = System.GetPhysics();
    PxScene* Scene = System.GetScene();

    // 좌표계 변환 적용
    PxTransform PTransform = PhysXConvert::ToPx(Transform);

    // Actor 생성 (Static vs Dynamic)
    if (bSimulatePhysics)
    {
        PxRigidDynamic* DynamicActor = Physics->createRigidDynamic(PTransform);
        DynamicActor->setLinearDamping(LinearDamping);
        DynamicActor->setAngularDamping(AngularDamping);
        DynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bEnableGravity);

        // CCD 설정
        if (bUseCCD)
        {
            DynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
        }

        // 시작 상태
        if (!bStartAwake)
        {
            DynamicActor->putToSleep();
        }

        RigidActor = DynamicActor;
    }
    else
    {
        RigidActor = Physics->createRigidStatic(PTransform);
    }

    if (!RigidActor) return;

    // 재질 가져오기 (없으면 기본 재질)
    PxMaterial* Material = System.GetDefaultMaterial();
    UPhysicalMaterial* UsedPhysMat = PhysMaterialOverride ? PhysMaterialOverride : PhysMat;
    if (UsedPhysMat && UsedPhysMat->MatHandle)
    {
        Material = UsedPhysMat->MatHandle;
    }

    // Shape 생성 및 부착
    PxShape* Shape = Physics->createShape(Geometry, *Material);
    if (Shape)
    {
        if (bIsTrigger)
        {
            Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
            Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
        }
        else
        {
            Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
            Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
        }

        // Scene Query 활성화
        Shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        RigidActor->attachShape(*Shape);
        Shapes.Add(Shape);
        Shape->release(); // Actor가 소유권을 가짐
    }

    // 질량 계산
    UpdateMassProperties();

    // DOF 잠금 적용
    ApplyDOFLock();

    // UserData 연결
    RigidActor->userData = static_cast<void*>(this);

    // 씬에 추가
    Scene->addActor(*RigidActor);
}

void FBodyInstance::InitBodyFromSetup(const FTransform& Transform, UBodySetup* InBodySetup,
                                       UPhysicalMaterial* PhysMat, const FVector& Scale3D)
{
    if (!InBodySetup || !InBodySetup->HasValidShapes())
    {
        UE_LOG("[FBodyInstance] InitBodyFromSetup failed: Invalid BodySetup or no shapes");
        return;
    }

    // 이미 있으면 삭제하고 다시 생성
    TermBody();

    FPhysicsSystem& System = FPhysicsSystem::Get();
    PxPhysics* Physics = System.GetPhysics();
    PxScene* Scene = System.GetScene();

    // 좌표계 변환 적용
    PxTransform PTransform = PhysXConvert::ToPx(Transform);

    // Actor 생성 (Static vs Dynamic)
    if (bSimulatePhysics)
    {
        PxRigidDynamic* DynamicActor = Physics->createRigidDynamic(PTransform);
        DynamicActor->setLinearDamping(LinearDamping);
        DynamicActor->setAngularDamping(AngularDamping);
        DynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bEnableGravity);

        // CCD 설정
        if (bUseCCD)
        {
            DynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
        }

        // 시작 상태
        if (!bStartAwake)
        {
            DynamicActor->putToSleep();
        }

        RigidActor = DynamicActor;
    }
    else
    {
        RigidActor = Physics->createRigidStatic(PTransform);
    }

    if (!RigidActor)
    {
        UE_LOG("[FBodyInstance] InitBodyFromSetup failed: Could not create RigidActor");
        return;
    }

    // BodySetup에서 모든 Shape 생성 및 부착
    InBodySetup->CreatePhysicsShapes(this, Scale3D, PhysMat);

    // UBodySetupCore 설정 적용 (PhysicsType, CollisionResponse, CollisionTraceFlag)
    ApplyBodySetupSettings(InBodySetup);

    // 질량 계산
    UpdateMassProperties();

    // DOF 잠금 적용
    ApplyDOFLock();

    // UserData 연결
    RigidActor->userData = static_cast<void*>(this);

    // 씬에 추가
    Scene->addActor(*RigidActor);

    UE_LOG("[FBodyInstance] InitBodyFromSetup success: %d shapes created",
           Shapes.Num());
}

void FBodyInstance::TermBody()
{
    if (RigidActor)
    {
        FPhysicsSystem::Get().GetScene()->removeActor(*RigidActor);
        RigidActor->release();
        RigidActor = nullptr;
    }
    Shapes.Empty();
}

// --- Transform ---

FTransform FBodyInstance::GetWorldTransform() const
{
    if (RigidActor)
    {
        // PhysX → 프로젝트 좌표계 변환
        return PhysXConvert::FromPx(RigidActor->getGlobalPose());
    }
    return FTransform();
}

void FBodyInstance::SetWorldTransform(const FTransform& NewTransform, bool bTeleport)
{
    if (!RigidActor) return;

    // 프로젝트 → PhysX 좌표계 변환
    PxTransform PTransform = PhysXConvert::ToPx(NewTransform);

    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        if (bTeleport)
        {
            DynamicActor->setGlobalPose(PTransform);
        }
        else
        {
            DynamicActor->setKinematicTarget(PTransform);
        }
    }
    else
    {
        RigidActor->setGlobalPose(PTransform);
    }
}

FVector FBodyInstance::GetWorldLocation() const
{
    if (RigidActor)
    {
        return PhysXConvert::FromPx(RigidActor->getGlobalPose().p);
    }
    return FVector::Zero();
}

FQuat FBodyInstance::GetWorldRotation() const
{
    if (RigidActor)
    {
        return PhysXConvert::FromPx(RigidActor->getGlobalPose().q);
    }
    return FQuat::Identity();
}

// --- 물리 상태 ---

bool FBodyInstance::IsDynamic() const
{
    return RigidActor && RigidActor->is<PxRigidDynamic>() != nullptr;
}

bool FBodyInstance::IsAwake() const
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        return !DynamicActor->isSleeping();
    }
    return false;
}

void FBodyInstance::WakeUp()
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        DynamicActor->wakeUp();
    }
}

void FBodyInstance::PutToSleep()
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        DynamicActor->putToSleep();
    }
}

// --- 힘/토크/속도 ---

void FBodyInstance::AddForce(const FVector& Force, bool bAccelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PForce = PhysXConvert::ToPx(Force);
        PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;
        DynamicActor->addForce(PForce, Mode);
    }
}

void FBodyInstance::AddForceAtLocation(const FVector& Force, const FVector& Location)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PForce = PhysXConvert::ToPx(Force);
        PxVec3 PLocation = PhysXConvert::ToPx(Location);
        PxRigidBodyExt::addForceAtPos(*DynamicActor, PForce, PLocation);
    }
}

void FBodyInstance::AddTorque(const FVector& Torque, bool bAccelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PTorque = PhysXConvert::AngularToPx(Torque);
        PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;
        DynamicActor->addTorque(PTorque, Mode);
    }
}

void FBodyInstance::AddImpulse(const FVector& Impulse, bool bVelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PImpulse = PhysXConvert::ToPx(Impulse);
        PxForceMode::Enum Mode = bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE;
        DynamicActor->addForce(PImpulse, Mode);
    }
}

void FBodyInstance::AddImpulseAtLocation(const FVector& Impulse, const FVector& Location)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PImpulse = PhysXConvert::ToPx(Impulse);
        PxVec3 PLocation = PhysXConvert::ToPx(Location);
        PxRigidBodyExt::addForceAtPos(*DynamicActor, PImpulse, PLocation, PxForceMode::eIMPULSE);
    }
}

void FBodyInstance::AddAngularImpulse(const FVector& AngularImpulse, bool bVelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PImpulse = PhysXConvert::AngularToPx(AngularImpulse);
        PxForceMode::Enum Mode = bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE;
        DynamicActor->addTorque(PImpulse, Mode);
    }
}

FVector FBodyInstance::GetLinearVelocity() const
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        return PhysXConvert::FromPx(DynamicActor->getLinearVelocity());
    }
    return FVector::Zero();
}

void FBodyInstance::SetLinearVelocity(const FVector& Velocity, bool bAddToCurrent)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PVel = PhysXConvert::ToPx(Velocity);
        if (bAddToCurrent)
        {
            PVel += DynamicActor->getLinearVelocity();
        }
        DynamicActor->setLinearVelocity(PVel);
    }
}

FVector FBodyInstance::GetAngularVelocity() const
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        return PhysXConvert::AngularFromPx(DynamicActor->getAngularVelocity());
    }
    return FVector::Zero();
}

void FBodyInstance::SetAngularVelocity(const FVector& AngularVelocity, bool bAddToCurrent)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PVel = PhysXConvert::AngularToPx(AngularVelocity);
        if (bAddToCurrent)
        {
            PVel += DynamicActor->getAngularVelocity();
        }
        DynamicActor->setAngularVelocity(PVel);
    }
}

// --- 질량 ---

float FBodyInstance::GetBodyMass() const
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        return DynamicActor->getMass();
    }
    return 0.0f;
}

void FBodyInstance::SetMassOverride(float InMassInKg, bool bNewOverrideMass)
{
    MassInKg = InMassInKg;
    bOverrideMass = bNewOverrideMass;
    UpdateMassProperties();
}

FVector FBodyInstance::GetBodyInertiaTensor() const
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        // 관성 텐서도 좌표계 변환 필요
        return PhysXConvert::FromPx(DynamicActor->getMassSpaceInertiaTensor());
    }
    return FVector::Zero();
}

// --- 충돌 설정 ---

void FBodyInstance::SetCollisionEnabled(bool bEnabled)
{
    if (!RigidActor) return;

    for (int32 i = 0; i < Shapes.Num(); ++i)
    {
        if (Shapes[i])
        {
            Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, bEnabled && !bIsTrigger);
            Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, bEnabled);
        }
    }
}

void FBodyInstance::SetSimulatePhysics(bool bNewSimulate)
{
    if (bSimulatePhysics == bNewSimulate) return;

    bSimulatePhysics = bNewSimulate;

    // 런타임 중 변경은 Body 재생성이 필요
    // 간단한 구현: 현재는 플래그만 변경
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        DynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, !bNewSimulate);
    }
}

void FBodyInstance::SetEnableGravity(bool bNewEnableGravity)
{
    bEnableGravity = bNewEnableGravity;

    if (RigidActor)
    {
        RigidActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bNewEnableGravity);
    }
}

// --- UBodySetupCore 설정 적용 ---

void FBodyInstance::ApplyBodySetupSettings(const UBodySetupCore* BodySetupCore)
{
    if (!BodySetupCore) return;

    // 1. PhysicsType 적용 (Simulated/Kinematic/Default)
    ApplyPhysicsType(BodySetupCore->PhysicsType);

    // 2. CollisionResponse 적용 (충돌 활성/비활성)
    ApplyCollisionResponse(BodySetupCore->CollisionResponse);

    // 3. CollisionTraceFlag 저장 (Shape 플래그에 적용)
    CollisionTraceFlag = BodySetupCore->CollisionTraceFlag;

    // Shape 플래그 적용
    for (int32 i = 0; i < Shapes.Num(); ++i)
    {
        if (!Shapes[i]) continue;

        switch (CollisionTraceFlag)
        {
        case ECollisionTraceFlag::UseSimpleCollision:
            // 단순 충돌만: 시뮬레이션 O, 쿼리 X
            Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !bIsTrigger);
            Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
            break;

        case ECollisionTraceFlag::UseComplexCollision:
            // 복잡 충돌만: 시뮬레이션 X, 쿼리 O
            Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
            Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
            break;

        case ECollisionTraceFlag::UseSimpleAsComplex:
        case ECollisionTraceFlag::UseComplexAsSimple:
        case ECollisionTraceFlag::UseDefault:
        default:
            // 기본: 둘 다 활성
            Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !bIsTrigger);
            Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
            break;
        }
    }
}

void FBodyInstance::ApplyPhysicsType(EPhysicsType PhysicsType)
{
    PxRigidDynamic* DynamicActor = GetDynamicActor();
    if (!DynamicActor) return;

    switch (PhysicsType)
    {
    case EPhysicsType::Simulated:
        // 물리 시뮬레이션 활성화 (Kinematic 플래그 해제)
        DynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
        bSimulatePhysics = true;
        break;

    case EPhysicsType::Kinematic:
        // 키네마틱 모드 (물리 시뮬레이션 비활성화, 직접 제어)
        DynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        bSimulatePhysics = false;
        break;

    case EPhysicsType::Default:
    default:
        // 기존 bSimulatePhysics 값 유지
        DynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, !bSimulatePhysics);
        break;
    }
}

void FBodyInstance::ApplyCollisionResponse(EBodyCollisionResponse::Type Response)
{
    if (!RigidActor) return;

    bool bEnableCollision = (Response == EBodyCollisionResponse::BodyCollision_Enabled);

    for (int32 i = 0; i < Shapes.Num(); ++i)
    {
        if (!Shapes[i]) continue;

        if (bEnableCollision)
        {
            // 충돌 활성화
            Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !bIsTrigger);
            Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        }
        else
        {
            // 충돌 비활성화 (오버랩만 감지)
            Shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
            Shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
        }
    }
}

// --- DOF 잠금 ---
// 주의: DOF 잠금은 PhysX 좌표계 기준으로 적용됨
// 프로젝트 좌표계와 다르므로 축 매핑 필요

void FBodyInstance::ApplyDOFLock()
{
    PxRigidDynamic* DynamicActor = GetDynamicActor();
    if (!DynamicActor) return;

    PxRigidDynamicLockFlags LockFlags;

    // 프로젝트 좌표계 → PhysX 좌표계 축 매핑
    // 프로젝트 X → PhysX Z
    // 프로젝트 Y → PhysX X
    // 프로젝트 Z → PhysX Y

    switch (DOFMode)
    {
    case EDOFMode::YZPlane:
        // 프로젝트 X축 고정 → PhysX Z축 고정
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        break;
    case EDOFMode::XZPlane:
        // 프로젝트 Y축 고정 → PhysX X축 고정
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        break;
    case EDOFMode::XYPlane:
        // 프로젝트 Z축 고정 → PhysX Y축 고정
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        break;
    case EDOFMode::SixDOF:
    case EDOFMode::CustomPlane:
        // 개별 플래그 사용 (축 매핑 적용)
        // 프로젝트 X → PhysX Z
        if (bLockXLinear) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
        if (bLockXAngular) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        // 프로젝트 Y → PhysX X
        if (bLockYLinear) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
        if (bLockYAngular) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        // 프로젝트 Z → PhysX Y
        if (bLockZLinear) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
        if (bLockZAngular) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        break;
    default:
        break;
    }

    DynamicActor->setRigidDynamicLockFlags(LockFlags);
}

// --- Private 헬퍼 함수 ---

void FBodyInstance::UpdateMassProperties()
{
    PxRigidDynamic* DynamicActor = GetDynamicActor();
    if (!DynamicActor) return;

    if (bOverrideMass)
    {
        PxRigidBodyExt::setMassAndUpdateInertia(*DynamicActor, MassInKg);
    }
    else
    {
        float Density = 1000.0f; // 기본 밀도 (kg/m^3)
        UPhysicalMaterial* UsedMat = PhysMaterialOverride;
        if (UsedMat)
        {
            Density = UsedMat->Density;
        }
        PxRigidBodyExt::updateMassAndInertia(*DynamicActor, Density);
    }
}

void FBodyInstance::ApplyPhysicsSettings()
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        DynamicActor->setLinearDamping(LinearDamping);
        DynamicActor->setAngularDamping(AngularDamping);

        if (MaxLinearVelocity > 0.0f)
        {
            DynamicActor->setMaxLinearVelocity(MaxLinearVelocity);
        }
        if (MaxAngularVelocity > 0.0f)
        {
            // PhysX는 rad/s 사용
            DynamicActor->setMaxAngularVelocity(PhysXConvert::DegreesToRadians(MaxAngularVelocity));
        }
    }
}

PxRigidDynamic* FBodyInstance::GetDynamicActor() const
{
    if (RigidActor)
    {
        return RigidActor->is<PxRigidDynamic>();
    }
    return nullptr;
}
