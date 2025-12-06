#include "pch.h"
#include "CCTQueryFilterCallback.h"

#include "BodyInstance.h"

// ==================================================================================
// FCCTQueryFilterCallback
// ==================================================================================

FCCTQueryFilterCallback::FCCTQueryFilterCallback()
    : MyChannel(ECollisionChannel::Pawn)
    , MyCollisionMask(CollisionMasks::DefaultPawn)
    , IgnoreActor(nullptr)
{
}

FCCTQueryFilterCallback::~FCCTQueryFilterCallback()
{
}

PxQueryHitType::Enum FCCTQueryFilterCallback::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags& queryFlags)
{
    if (!shape || !actor)
    {
        return PxQueryHitType::eNONE;
    }

    // 자기 자신 무시
    if (IgnoreActor && actor == IgnoreActor)
    {
        return PxQueryHitType::eNONE;
    }

    // Shape의 FilterData 가져오기
    PxFilterData ShapeFilter = shape->getQueryFilterData();

    // 충돌 채널/마스크 기반 필터링
    // ShapeFilter.word0 = Shape의 채널 비트
    // ShapeFilter.word1 = Shape의 충돌 마스크
    PxU32 OtherChannel = ShapeFilter.word0;
    PxU32 OtherMask = ShapeFilter.word1;

    // CCT의 채널 비트 (인덱스가 아닌 비트로 변환)
    PxU32 MyChannelBit = ChannelToBit(MyChannel);

    // 양방향 체크:
    // 1. 내 마스크가 상대 채널을 포함하는가?
    // 2. 상대 마스크가 내 채널을 포함하는가?
    bool bMyMaskIncludesOther = (MyCollisionMask & OtherChannel) != 0;
    bool bOtherMaskIncludesMe = (OtherMask & MyChannelBit) != 0;

    if (!bMyMaskIncludesOther || !bOtherMaskIncludesMe)
    {
        return PxQueryHitType::eNONE;  // 충돌 안 함
    }

    // 기본적으로 Block 처리
    return PxQueryHitType::eBLOCK;
}

PxQueryHitType::Enum FCCTQueryFilterCallback::postFilter(
    const PxFilterData& filterData,
    const PxQueryHit& hit)
{
    // 기본 구현: preFilter 결과 그대로 사용
    return PxQueryHitType::eBLOCK;
}

// ==================================================================================
// FCCTControllerFilterCallback
// ==================================================================================

FCCTControllerFilterCallback::FCCTControllerFilterCallback()
{
}

FCCTControllerFilterCallback::~FCCTControllerFilterCallback()
{
}

bool FCCTControllerFilterCallback::filter(const PxController& a, const PxController& b)
{
    // 기본적으로 모든 CCT끼리 충돌 허용
    // 필요시 UserData를 통해 추가 필터링 가능

    void* DataA = a.getUserData();
    void* DataB = b.getUserData();

    if (!DataA || !DataB)
    {
        return true;  // 데이터 없으면 기본 충돌
    }

    // 추후 팀/그룹 기반 필터링 추가 가능
    // 예: 같은 팀이면 통과, 다른 팀이면 충돌

    return true;  // 충돌 허용
}
