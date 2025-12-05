#pragma once
#include "Info.h"
#include "ALevelTransitionManager.generated.h"

/**
 * 레벨 전환 상태
 */
enum class ELevelTransitionState : uint8
{
    Idle,           // 전환 중이 아님
    Transitioning   // 전환 중
};

/**
 * 레벨 전환 관리자
 * - PIE 모드에서만 활성화
 * - Persistent Actor로 설정되어 씬 전환 시에도 유지됨
 * - Lua에서 호출 가능한 TransitionToLevel() 제공
 */
UCLASS(DisplayName="ALevelTransitionManager")
class ALevelTransitionManager : public AInfo
{
public:
    GENERATED_REFLECTION_BODY()

    ALevelTransitionManager();
    virtual ~ALevelTransitionManager() override;

    // ════════════════════════════════════════════════════════════════════════
    // 레벨 전환 API

    /**
     * 지정된 레벨로 전환
     * @param LevelPath 레벨 파일 경로 (예: L"Data/Scenes/Level2.scene")
     */
    void TransitionToLevel(const FWideString& LevelPath);

    /**
     * 현재 전환 중인지 여부
     */
    bool IsTransitioning() const { return TransitionState == ELevelTransitionState::Transitioning; }

    /**
     * 현재 전환 상태 조회
     */
    ELevelTransitionState GetTransitionState() const { return TransitionState; }

    // ════════════════════════════════════════════════════════════════════════
    // Actor 오버라이드

    virtual void BeginPlay() override;

private:
    // 전환 상태
    ELevelTransitionState TransitionState = ELevelTransitionState::Idle;

    // 레벨 파일 존재 여부 확인 (유틸리티)
    bool DoesLevelFileExist(const FWideString& LevelPath) const;
};
