#include "pch.h"
#include "BodyInstance.h"
#include "BodySetup.h"
#include "PhysScene.h"

FBodyInstance::FBodyInstance()
    : OwnerComponent(nullptr)
    , BodySetup(nullptr)
    , PhysScene(nullptr)
    , Scale3D(FVector(1.f, 1.f, 1.f))
    , bSimulatePhysics(false)
    , LinearDamping(0.01f)
    , AngularDamping(0.f)
    , MassInKgOverride(100.0f)
    , bOverrideMass(false)
{
}

FBodyInstance::~FBodyInstance()
{
}

void FBodyInstance::InitBody(UBodySetup* Setup, const FTransform& Transform, UPrimitiveComponent* Component, FPhysScene* InRBScene)
{
    if (!Setup || !InRBScene || !GPhysXSDK)
    {
        UE_LOG("[PhysX Error] InitBody에 실패했습니다.");
        return;
    }

    if (IsValidBodyInstance())
    {
        TermBody();
    }

    BodySetup       = Setup;
    OwnerComponent  = Component;
    PhysScene       = InRBScene;
    Scale3D         = Transform.Scale3D;

    PxTransform PhysicsTransform = U2PTransform(Transform);

    if (bSimulatePhysics)
    {
        RigidActor = GPhysXSDK->createRigidDynamic(PhysicsTransform);
    }
    else
    {
        RigidActor = GPhysXSDK->createRigidStatic(PhysicsTransform);    
    }

    if (!RigidActor)
    {
        UE_LOG("[PhysX Error] RigidActor 생성에 실패했습니다.");
        return;
    }

    RigidActor->userData = this;

    Setup->AddShapesToRigidActor_AssumesLocked(this, Scale3D, RigidActor);

    if (IsDynamic())
    {
        PxRigidDynamic* DynamicActor = RigidActor->is<PxRigidDynamic>();

        if (bOverrideMass)
        {
            // 질량에 맞추어 관성 텐서 계산
            PxRigidBodyExt::setMassAndUpdateInertia(*DynamicActor, MassInKgOverride);
        }
        else
        {
            // 제공된 밀도를 통해 크기에 맞춰서 무게를 계산 (1x1x1 박스 기준 10kg)
            PxRigidBodyExt::updateMassAndInertia(*DynamicActor, 10.0f);
        }

        DynamicActor->setLinearDamping(LinearDamping);
        DynamicActor->setAngularDamping(AngularDamping);
    }

    if (PhysScene->GetPxScene())
    {
        PhysScene->GetPxScene()->addActor(*RigidActor); 
    }
}

void FBodyInstance::TermBody()
{
    if (RigidActor)
    {
        if (PhysScene && PhysScene->GetPxScene())
        {
            PhysScene->GetPxScene()->removeActor(*RigidActor);
        }

        RigidActor->release();
        RigidActor = nullptr;
    }

    BodySetup = nullptr;
    PhysScene = nullptr;
    OwnerComponent = nullptr; 
}

FTransform FBodyInstance::GetUnrealWorldTransform() const
{
    if (IsValidBodyInstance())
    {
        PxTransform PhysicsTransform = RigidActor->getGlobalPose();
        return P2UTransform(PhysicsTransform);
    }
    return FTransform();
}

void FBodyInstance::SetLinearVelocity(const FVector& NewVel, bool bAddToCurrent)
{
    if (IsValidBodyInstance() && IsDynamic())
    {
        PxRigidDynamic* DynamicActor = RigidActor->is<PxRigidDynamic>();

        if (bAddToCurrent)
        {
            PxVec3 CurrentVel = DynamicActor->getLinearVelocity();
            DynamicActor->setLinearVelocity(CurrentVel + U2PVector(NewVel));
        }
        else
        {
            DynamicActor->setLinearVelocity(U2PVector(NewVel));
        }
    }
}

void FBodyInstance::AddForce(const FVector& Force, bool bAccelChange)
{
    if (IsValidBodyInstance() && IsDynamic())
    {
        PxRigidDynamic* DynamicActor = RigidActor->is<PxRigidDynamic>();
        PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;

        DynamicActor->addForce(U2PVector(Force), Mode);
    }
}

void FBodyInstance::AddTorque(const FVector& Torque, bool bAccelChange)
{
    if (IsValidBodyInstance() && IsDynamic())
    {
        PxRigidDynamic* DynamicActor = RigidActor->is<PxRigidDynamic>();
        PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;

        DynamicActor->addTorque(U2PVector(Torque), Mode);
    }
}

bool FBodyInstance::IsDynamic() const
{
    return RigidActor->is<PxRigidDynamic>();
}
