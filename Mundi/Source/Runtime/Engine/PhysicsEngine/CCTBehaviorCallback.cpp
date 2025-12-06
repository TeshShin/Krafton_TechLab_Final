#include "pch.h"
#include "CCTBehaviorCallback.h"

FCCTBehaviorCallback::FCCTBehaviorCallback()
    : bCanRideOnOtherCCT(false)
    , bCanRideOnDynamic(false)
{
}

FCCTBehaviorCallback::~FCCTBehaviorCallback()
{
}

PxControllerBehaviorFlags FCCTBehaviorCallback::getBehaviorFlags(
    const PxShape& shape,
    const PxActor& actor)
{
    PxControllerBehaviorFlags flags(0);

    // Dynamic 물체 위에 올라탈 수 있는지 확인
    if (bCanRideOnDynamic)
    {
        const PxRigidDynamic* dynamic = actor.is<PxRigidDynamic>();
        if (dynamic && !(dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
        {
            flags |= PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
        }
    }

    // Kinematic 물체 위에는 기본적으로 올라탈 수 있음
    const PxRigidDynamic* dynamic = actor.is<PxRigidDynamic>();
    if (dynamic && (dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
    {
        flags |= PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    // Static 물체 위에도 올라탈 수 있음
    if (actor.is<PxRigidStatic>())
    {
        flags |= PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    return flags;
}

PxControllerBehaviorFlags FCCTBehaviorCallback::getBehaviorFlags(
    const PxController& controller)
{
    PxControllerBehaviorFlags flags(0);

    // 다른 CCT 위에 올라탈 수 있는지
    if (bCanRideOnOtherCCT)
    {
        flags |= PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    return flags;
}

PxControllerBehaviorFlags FCCTBehaviorCallback::getBehaviorFlags(
    const PxObstacle& obstacle)
{
    // Obstacle은 현재 미사용
    // 기본적으로 올라탈 수 있도록 설정
    return PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
}
