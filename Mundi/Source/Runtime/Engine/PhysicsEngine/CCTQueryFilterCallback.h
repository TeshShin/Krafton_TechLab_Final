#pragma once

#include "PhysXSupport.h"
#include "ECollisionChannel.h"

/**
 * FCCTQueryFilterCallback
 *
 * CCT의 Scene Query 시 충돌 필터링을 담당
 * 기존 충돌 채널 시스템(ECollisionChannel, CollisionMask)과 통합
 */
class FCCTQueryFilterCallback : public PxQueryFilterCallback
{
public:
    FCCTQueryFilterCallback();
    virtual ~FCCTQueryFilterCallback();

    // ==================================================================================
    // PxQueryFilterCallback Interface
    // ==================================================================================

    /**
     * Scene Query 전 필터링 (Shape 단위)
     * 충돌 채널/마스크를 확인하여 충돌 여부 결정
     *
     * @param filterData Shape에 설정된 FilterData (word0=채널, word1=마스크)
     * @param shape 검사할 Shape
     * @param actor Shape 소유 Actor
     * @param queryFlags 쿼리 플래그 (수정 가능)
     * @return eBLOCK(충돌), eTOUCH(터치), eNONE(무시)
     */
    virtual PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags& queryFlags) override;

    /**
     * Scene Query 후 필터링 (결과 단위)
     * (기본 구현 사용)
     */
    virtual PxQueryHitType::Enum postFilter(
        const PxFilterData& filterData,
        const PxQueryHit& hit) override;

    // ==================================================================================
    // 필터 설정
    // ==================================================================================

    /** 이 CCT의 충돌 채널 */
    ECollisionChannel MyChannel = ECollisionChannel::Pawn;

    /** 이 CCT의 충돌 마스크 */
    uint32 MyCollisionMask = CollisionMasks::DefaultPawn;

    /** 무시할 Actor (자기 자신 등) */
    PxRigidActor* IgnoreActor = nullptr;
};

/**
 * FCCTControllerFilterCallback
 *
 * CCT끼리의 충돌 필터링을 담당
 */
class FCCTControllerFilterCallback : public PxControllerFilterCallback
{
public:
    FCCTControllerFilterCallback();
    virtual ~FCCTControllerFilterCallback();

    /**
     * 두 CCT가 충돌할지 결정
     * @param a 첫 번째 Controller
     * @param b 두 번째 Controller
     * @return true면 충돌, false면 통과
     */
    virtual bool filter(const PxController& a, const PxController& b) override;
};
