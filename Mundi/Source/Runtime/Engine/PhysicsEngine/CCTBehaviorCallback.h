#pragma once

#include "PhysXSupport.h"

/**
 * FCCTBehaviorCallback
 *
 * CCT의 충돌 동작을 제어하는 콜백 클래스
 * Dynamic 물체와의 상호작용 정책을 정의
 */
class FCCTBehaviorCallback : public PxControllerBehaviorCallback
{
public:
    FCCTBehaviorCallback();
    virtual ~FCCTBehaviorCallback();

    // ==================================================================================
    // PxControllerBehaviorCallback Interface
    // ==================================================================================

    /**
     * Shape와 충돌 시 동작 결정
     * Dynamic 물체에 대해 밀기 또는 슬라이딩 결정
     */
    virtual PxControllerBehaviorFlags getBehaviorFlags(
        const PxShape& shape,
        const PxActor& actor) override;

    /**
     * 다른 Controller와 충돌 시 동작 결정
     * CCT끼리 밀어내기 또는 통과 결정
     */
    virtual PxControllerBehaviorFlags getBehaviorFlags(
        const PxController& controller) override;

    /**
     * Obstacle과 충돌 시 동작 결정
     * (현재 미사용)
     */
    virtual PxControllerBehaviorFlags getBehaviorFlags(
        const PxObstacle& obstacle) override;

    // ==================================================================================
    // 설정
    // ==================================================================================

    /** CCT 위에 다른 CCT가 올라탈 수 있는지 */
    bool bCanRideOnOtherCCT = false;

    /** CCT 위에 Dynamic 물체가 올라탈 수 있는지 */
    bool bCanRideOnDynamic = false;
};
