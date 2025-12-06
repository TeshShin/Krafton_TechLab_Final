#pragma once
#include "GameModeBase.h"
#include "Source/Runtime/Core/Memory/PointerTypes.h"
#include "AGameJamGameMode.generated.h"

class SButton;
class STextBlock;
class ATimerTrigger;

/**
 * 게임잼용 범용 GameMode
 *
 * 씬 타입에 따라 다른 UI와 로직을 자동으로 설정:
 * - Intro: 시작/종료 버튼
 * - Cutscene: 타이머 + 자동 전환
 * - Stage: 타이머 + 게임 UI
 * - Ending: 재시작 버튼
 *
 * 사용법:
 * 1. 모든 .scene 파일에 "GameModeClass": "AGameJamGameMode" 설정
 * 2. 각 씬마다 LevelTransitionManager 배치 필수
 * 3. 시간 제한이 필요한 씬에만 TimerTrigger 배치
 */
UCLASS(DisplayName="게임잼 게임 모드", Description="6개 씬을 지원하는 범용 게임 모드")
class AGameJamGameMode : public AGameModeBase
{
public:
    GENERATED_REFLECTION_BODY()

    AGameJamGameMode();
    virtual ~AGameJamGameMode() override = default;

    // ════════════════════════════════════════════════════════════════════════
    // 생명주기

    virtual void BeginPlay() override;
    virtual void EndPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ════════════════════════════════════════════════════════════════════════
    // 씬 타입 감지

    enum class ESceneType
    {
        Intro,      // 게임 인트로 (버튼)
        Cutscene,   // 컷씬 (타이머 자동 전환)
        Stage,      // 스테이지 (타이머 + 게임플레이)
        Ending      // 엔딩 (재시작 버튼)
    };

    /**
     * 현재 씬 타입 자동 감지
     * 레벨 이름으로 판단: Intro.scene, Cutscene1.scene, Stage1.scene, Ending.scene 등
     */
    ESceneType DetectSceneType();

private:
    // ════════════════════════════════════════════════════════════════════════
    // UI 위젯 (씬 타입마다 다름)

    // Intro
    TSharedPtr<SButton> StartButton;        
    TSharedPtr<SButton> QuitButton;     

    TSharedPtr<STextBlock> TimerText;       // Cutscene/Stage용
    TSharedPtr<STextBlock> InfoText;        // 공통
    TSharedPtr<STextBlock> ScoreText;       // Stage용

    // Ending
    TSharedPtr<SButton> RestartButton;      // Ending용

    // ════════════════════════════════════════════════════════════════════════
    // 씬 타입별 초기화

    void InitIntroScene();      // 인트로 UI 생성
    void InitCutscene();        // 컷씬 UI 생성
    void InitStage();           // 스테이지 UI 생성
    void InitEnding();          // 엔딩 UI 생성

    // ════════════════════════════════════════════════════════════════════════
    // 공통 기능

    ATimerTrigger* TimerTrigger = nullptr;
    ESceneType CurrentSceneType = ESceneType::Intro;

    void FindTimerTrigger();
    void UpdateTimerUI();
    void TransitionToNextScene();
    void SetupNextLevel();  // 씬별로 NextLevel 자동 설정
    void ClearAllWidgets();
};
