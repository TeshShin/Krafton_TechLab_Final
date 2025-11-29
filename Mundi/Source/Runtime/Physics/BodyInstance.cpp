#include "pch.h"
#include "BodyInstance.h"
#include "PhysicalMaterial.h"
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
    PxTransform PTransform = Transform;

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

    // UserData 연결 (변환해서 사용하면 편함)
    RigidActor->userData = static_cast<void*>(this);

    // 씬에 추가
    Scene->addActor(*RigidActor);
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
        return RigidActor->getGlobalPose();
    }
    return FTransform();
}

void FBodyInstance::SetWorldTransform(const FTransform& NewTransform, bool bTeleport)
{
    if (!RigidActor) return;

    PxTransform PTransform = NewTransform;

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
        PxVec3 Pos = RigidActor->getGlobalPose().p;
        return FVector(Pos.x, Pos.y, Pos.z);
    }
    return FVector::Zero();
}

FQuat FBodyInstance::GetWorldRotation() const
{
    if (RigidActor)
    {
        PxQuat Q = RigidActor->getGlobalPose().q;
        return FQuat(Q.x, Q.y, Q.z, Q.w);
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
        PxVec3 PForce(Force.X, Force.Y, Force.Z);
        PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;
        DynamicActor->addForce(PForce, Mode);
    }
}

void FBodyInstance::AddForceAtLocation(const FVector& Force, const FVector& Location)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PForce(Force.X, Force.Y, Force.Z);
        PxVec3 PLocation(Location.X, Location.Y, Location.Z);
        PxRigidBodyExt::addForceAtPos(*DynamicActor, PForce, PLocation);
    }
}

void FBodyInstance::AddTorque(const FVector& Torque, bool bAccelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PTorque(Torque.X, Torque.Y, Torque.Z);
        PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;
        DynamicActor->addTorque(PTorque, Mode);
    }
}

void FBodyInstance::AddImpulse(const FVector& Impulse, bool bVelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PImpulse(Impulse.X, Impulse.Y, Impulse.Z);
        PxForceMode::Enum Mode = bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE;
        DynamicActor->addForce(PImpulse, Mode);
    }
}

void FBodyInstance::AddImpulseAtLocation(const FVector& Impulse, const FVector& Location)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PImpulse(Impulse.X, Impulse.Y, Impulse.Z);
        PxVec3 PLocation(Location.X, Location.Y, Location.Z);
        PxRigidBodyExt::addForceAtPos(*DynamicActor, PImpulse, PLocation, PxForceMode::eIMPULSE);
    }
}

void FBodyInstance::AddAngularImpulse(const FVector& AngularImpulse, bool bVelChange)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PImpulse(AngularImpulse.X, AngularImpulse.Y, AngularImpulse.Z);
        PxForceMode::Enum Mode = bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE;
        DynamicActor->addTorque(PImpulse, Mode);
    }
}

FVector FBodyInstance::GetLinearVelocity() const
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 Vel = DynamicActor->getLinearVelocity();
        return FVector(Vel.x, Vel.y, Vel.z);
    }
    return FVector::Zero();
}

void FBodyInstance::SetLinearVelocity(const FVector& Velocity, bool bAddToCurrent)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PVel(Velocity.X, Velocity.Y, Velocity.Z);
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
        PxVec3 Vel = DynamicActor->getAngularVelocity();
        return FVector(Vel.x, Vel.y, Vel.z);
    }
    return FVector::Zero();
}

void FBodyInstance::SetAngularVelocity(const FVector& AngularVelocity, bool bAddToCurrent)
{
    if (PxRigidDynamic* DynamicActor = GetDynamicActor())
    {
        PxVec3 PVel(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z);
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
        PxVec3 Inertia = DynamicActor->getMassSpaceInertiaTensor();
        return FVector(Inertia.x, Inertia.y, Inertia.z);
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

// --- DOF 잠금 ---

void FBodyInstance::ApplyDOFLock()
{
    PxRigidDynamic* DynamicActor = GetDynamicActor();
    if (!DynamicActor) return;

    PxRigidDynamicLockFlags LockFlags;

    // 프리셋 모드
    switch (DOFMode)
    {
    case EDOFMode::YZPlane:
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        break;
    case EDOFMode::XZPlane:
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
        break;
    case EDOFMode::XYPlane:
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        break;
    case EDOFMode::SixDOF:
    case EDOFMode::CustomPlane:
        // 개별 플래그 사용
        if (bLockXLinear) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
        if (bLockYLinear) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
        if (bLockZLinear) LockFlags |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
        if (bLockXAngular) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
        if (bLockYAngular) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
        if (bLockZAngular) LockFlags |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
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
            DynamicActor->setMaxAngularVelocity(MaxAngularVelocity * (PI / 180.0f));
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
