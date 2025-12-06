#include "pch.h"
#include "GameJamGameMode.h"
#include "TimerTrigger.h"
#include "LevelTransitionManager.h"
#include "World.h"
#include "GameUI/SGameHUD.h"
#include "GameUI/SButton.h"
#include "GameUI/STextBlock.h"
#include <cmath>

IMPLEMENT_CLASS(AGameJamGameMode)

AGameJamGameMode::AGameJamGameMode()
{
    ObjectName = "GameJamGameMode";
}

// ════════════════════════════════════════════════════════════════════════
// 생명주기

void AGameJamGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!SGameHUD::Get().IsInitialized()) { return; }

    // 씬 타입 감지
    CurrentSceneType = DetectSceneType();

    UE_LOG("[info] GameJamGameMode: BeginPlay - Scene type: %d", static_cast<int>(CurrentSceneType));

    // TimerTrigger 찾기 (있으면)
    FindTimerTrigger();

    // LevelTransitionManager에 NextLevel 자동 설정 (테스트용)
    SetupNextLevel();

    // 씬 타입에 따라 다른 UI 초기화
    switch (CurrentSceneType)
    {
    case ESceneType::Intro:
        InitIntroScene();
        break;
    case ESceneType::Cutscene:
        InitCutscene();
        break;
    case ESceneType::Stage:
        InitStage();
        break;
    case ESceneType::Ending:
        InitEnding();
        break;
    }
}

void AGameJamGameMode::EndPlay()
{
    Super::EndPlay();
    ClearAllWidgets();
}

void AGameJamGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 타이머가 있는 씬에서만 UI 업데이트
    if (CurrentSceneType == ESceneType::Cutscene || CurrentSceneType == ESceneType::Stage)
    {
        UpdateTimerUI();
    }
}

// ════════════════════════════════════════════════════════════════════════
// 씬 타입 감지

AGameJamGameMode::ESceneType AGameJamGameMode::DetectSceneType()
{
    // ObjectName으로 씬 타입 판단 (GameMode의 이름은 씬과 관계없이 고정이므로 다른 방법 필요)
    // 방법 1: LevelTransitionManager를 찾아서 그 이름으로 판단
    // 방법 2: 특정 액터의 존재 여부로 판단

    // 간단하게: TimerTrigger가 있으면 Cutscene 또는 Stage, 없으면 Intro 또는 Ending
    // StartButton/QuitButton이 필요하면 Intro, RestartButton이 필요하면 Ending

    // 임시로 모든 씬을 Stage로 처리 (나중에 씬별로 커스터마이징 가능)
    // 실제로는 각 씬에 특별한 마커 액터를 배치하거나, GameInstance에 씬 정보 저장

    if (!GetWorld())
        return ESceneType::Stage;

    // TimerTrigger 찾기
    bool bHasTimer = false;
    for (AActor* Actor : GetWorld()->GetActors())
    {
        if (dynamic_cast<ATimerTrigger*>(Actor))
        {
            bHasTimer = true;
            break;
        }
    }

    // 간단한 휴리스틱: 타이머가 있으면 Stage, 없으면 Intro (나중에 개선 가능)
    return bHasTimer ? ESceneType::Stage : ESceneType::Intro;
}

// ════════════════════════════════════════════════════════════════════════
// 씬 타입별 초기화

void AGameJamGameMode::InitIntroScene()
{
    UE_LOG("[info] GameJamGameMode: Initializing Intro scene");

    // 타이틀 텍스트
    InfoText = MakeShared<STextBlock>();
    InfoText->SetText(L"게임 타이틀")
        .SetFontSize(48.f)
        .SetColor(FSlateColor::White())
        .SetShadow(true, FVector2D(3.f, 3.f), FSlateColor::Black());

    SGameHUD::Get().AddWidget(InfoText)
        .SetAnchor(0.5f, 0.3f)
        .SetPivot(0.5f, 0.5f)
        .SetSize(600.f, 80.f);

    // 시작 버튼
    StartButton = MakeShared<SButton>();
    StartButton->SetText(L"게임 시작")
        .SetBackgroundColor(FSlateColor(0.2f, 0.6f, 0.2f, 1.f))
        .SetFontSize(28.f)
        .SetCornerRadius(8.f)
        .OnClicked([]() {
            UE_LOG("[info] GameJamGameMode: Start button clicked");

            // World를 통해 안전하게 LevelTransitionManager 접근
            if (!GEngine.IsPIEActive())
                return;

            for (const auto& Context : GEngine.GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::Game && Context.World)
                {
                    for (AActor* Actor : Context.World->GetActors())
                    {
                        ALevelTransitionManager* Manager = dynamic_cast<ALevelTransitionManager*>(Actor);
                        if (Manager)
                        {
                            if (Manager->IsTransitioning())
                            {
                                UE_LOG("[warning] GameJamGameMode: Already transitioning, ignoring button click");
                                return;
                            }

                            Manager->TransitionToNextLevel();
                            return;
                        }
                    }
                    break;
                }
            }

            UE_LOG("[error] GameJamGameMode: LevelTransitionManager not found!");
        });

    SGameHUD::Get().AddWidget(StartButton)
        .SetAnchor(0.5f, 0.5f)
        .SetPivot(0.5f, 0.5f)
        .SetOffset(0.f, 0.f)
        .SetSize(250.f, 70.f);

    // 종료 버튼
    QuitButton = MakeShared<SButton>();
    QuitButton->SetText(L"게임 종료")
        .SetBackgroundColor(FSlateColor(0.6f, 0.2f, 0.2f, 1.f))
        .SetFontSize(28.f)
        .SetCornerRadius(8.f)
        .OnClicked([]() {
            UE_LOG("[info] GameJamGameMode: Quit button clicked");
            PostQuitMessage(0);
        });

    SGameHUD::Get().AddWidget(QuitButton)
        .SetAnchor(0.5f, 0.5f)
        .SetPivot(0.5f, 0.5f)
        .SetOffset(0.f, 90.f)
        .SetSize(250.f, 70.f);
}

void AGameJamGameMode::InitCutscene()
{
    UE_LOG("[info] GameJamGameMode: Initializing Cutscene");

    // 컷씬 안내 텍스트
    InfoText = MakeShared<STextBlock>();
    InfoText->SetText(L"스토리 장면")
        .SetFontSize(24.f)
        .SetColor(FSlateColor(0.8f, 0.8f, 0.8f, 1.f))
        .SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

    SGameHUD::Get().AddWidget(InfoText)
        .SetAnchor(0.5f, 0.1f)
        .SetPivot(0.5f, 0.5f)
        .SetSize(400.f, 50.f);

    // 타이머 텍스트 (있으면)
    if (TimerTrigger)
    {
        TimerText = MakeShared<STextBlock>();
        TimerText->SetText(L"")
            .SetFontSize(20.f)
            .SetColor(FSlateColor(1.f, 1.f, 1.f, 0.7f))
            .SetShadow(true, FVector2D(1.f, 1.f), FSlateColor::Black());

        SGameHUD::Get().AddWidget(TimerText)
            .SetAnchor(1.f, 0.f)
            .SetPivot(1.f, 0.f)
            .SetOffset(-20.f, 20.f)
            .SetSize(200.f, 40.f);
    }
}

void AGameJamGameMode::InitStage()
{
    UE_LOG("[info] GameJamGameMode: Initializing Stage");

    // 스테이지 정보 텍스트
    InfoText = MakeShared<STextBlock>();
    InfoText->SetText(L"목표를 달성하세요!")
        .SetFontSize(24.f)
        .SetColor(FSlateColor::White())
        .SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

    SGameHUD::Get().AddWidget(InfoText)
        .SetAnchor(0.5f, 0.05f)
        .SetPivot(0.5f, 0.5f)
        .SetSize(400.f, 50.f);

    // 점수 텍스트
    ScoreText = MakeShared<STextBlock>();
    ScoreText->SetText(L"수집: 0 / 10")
        .SetFontSize(28.f)
        .SetColor(FSlateColor(1.f, 0.9f, 0.3f, 1.f))
        .SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

    SGameHUD::Get().AddWidget(ScoreText)
        .SetAnchor(0.f, 0.f)
        .SetPivot(0.f, 0.f)
        .SetOffset(20.f, 20.f)
        .SetSize(300.f, 50.f);

    // 타이머 텍스트 (큰 폰트)
    if (TimerTrigger)
    {
        TimerText = MakeShared<STextBlock>();
        TimerText->SetText(L"60")
            .SetFontSize(48.f)
            .SetColor(FSlateColor(1.f, 0.3f, 0.3f, 1.f))
            .SetShadow(true, FVector2D(3.f, 3.f), FSlateColor::Black());

        SGameHUD::Get().AddWidget(TimerText)
            .SetAnchor(1.f, 0.f)
            .SetPivot(1.f, 0.f)
            .SetOffset(-30.f, 30.f)
            .SetSize(150.f, 70.f);
    }
}

void AGameJamGameMode::InitEnding()
{
    UE_LOG("[info] GameJamGameMode: Initializing Ending scene");

    // 엔딩 텍스트
    InfoText = MakeShared<STextBlock>();
    InfoText->SetText(L"축하합니다!\n게임을 클리어하셨습니다!")
        .SetFontSize(36.f)
        .SetColor(FSlateColor::White())
        .SetShadow(true, FVector2D(3.f, 3.f), FSlateColor::Black());

    SGameHUD::Get().AddWidget(InfoText)
        .SetAnchor(0.5f, 0.3f)
        .SetPivot(0.5f, 0.5f)
        .SetSize(600.f, 100.f);

    // 재시작 버튼
    RestartButton = MakeShared<SButton>();
    RestartButton->SetText(L"처음부터 다시")
        .SetBackgroundColor(FSlateColor(0.2f, 0.5f, 0.8f, 1.f))
        .SetFontSize(28.f)
        .SetCornerRadius(8.f)
        .OnClicked([]() {
            UE_LOG("[info] GameJamGameMode: Restart button clicked");

            // World를 통해 안전하게 LevelTransitionManager 접근
            if (!GEngine.IsPIEActive())
                return;

            for (const auto& Context : GEngine.GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::Game && Context.World)
                {
                    for (AActor* Actor : Context.World->GetActors())
                    {
                        ALevelTransitionManager* Manager = dynamic_cast<ALevelTransitionManager*>(Actor);
                        if (Manager)
                        {
                            if (Manager->IsTransitioning())
                            {
                                UE_LOG("[warning] GameJamGameMode: Already transitioning, ignoring restart button");
                                return;
                            }

                            Manager->TransitionToLevel(L"Data/Scenes/TestButton.scene");
                            return;
                        }
                    }
                    break;
                }
            }

            UE_LOG("[error] GameJamGameMode: LevelTransitionManager not found!");
        });

    SGameHUD::Get().AddWidget(RestartButton)
        .SetAnchor(0.5f, 0.6f)
        .SetPivot(0.5f, 0.5f)
        .SetSize(250.f, 70.f);
}

// ════════════════════════════════════════════════════════════════════════
// 공통 기능

void AGameJamGameMode::FindTimerTrigger()
{
    TimerTrigger = nullptr;

    if (!GetWorld())
        return;

    for (AActor* Actor : GetWorld()->GetActors())
    {
        ATimerTrigger* Timer = dynamic_cast<ATimerTrigger*>(Actor);
        if (Timer)
        {
            TimerTrigger = Timer;
            UE_LOG("[info] GameJamGameMode: Found TimerTrigger - %.1fs", Timer->TriggerTime);
            break;
        }
    }
}

void AGameJamGameMode::UpdateTimerUI()
{
    if (!TimerText || !TimerTrigger)
        return;

    if (TimerTrigger->IsRunning())
    {
        float Remaining = TimerTrigger->GetRemainingTime();
        int Seconds = static_cast<int>(std::ceil(Remaining));

        if (CurrentSceneType == ESceneType::Cutscene)
        {
            TimerText->SetText(L"다음 장면까지: " + std::to_wstring(Seconds) + L"초");
        }
        else // Stage
        {
            TimerText->SetText(std::to_wstring(Seconds));

            // 시간이 얼마 안 남으면 빨간색으로 깜빡임
            if (Seconds <= 10)
            {
                float BlinkAlpha = (std::sin(TimerTrigger->GetElapsedTime() * 5.0f) + 1.0f) * 0.5f;
                TimerText->SetColor(FSlateColor(1.f, 0.2f, 0.2f, 0.5f + BlinkAlpha * 0.5f));
            }
        }
    }
    else if (TimerTrigger->HasExpired())
    {
        TimerText->SetText(L"시간 종료!");
    }
}

void AGameJamGameMode::TransitionToNextScene()
{
    if (!GetWorld())
        return;

    for (AActor* Actor : GetWorld()->GetActors())
    {
        ALevelTransitionManager* Manager = dynamic_cast<ALevelTransitionManager*>(Actor);
        if (Manager)
        {
            // 이미 전환 중이면 무시
            if (Manager->IsTransitioning())
            {
                UE_LOG("[warning] GameJamGameMode: Already transitioning, ignoring button click");
                return;
            }

            Manager->TransitionToNextLevel();
            return;
        }
    }

    UE_LOG("[error] GameJamGameMode: LevelTransitionManager not found!");
}

void AGameJamGameMode::SetupNextLevel()
{
    if (!GetWorld())
        return;

    // LevelTransitionManager 찾기
    ALevelTransitionManager* Manager = nullptr;
    for (AActor* Actor : GetWorld()->GetActors())
    {
        Manager = dynamic_cast<ALevelTransitionManager*>(Actor);
        if (Manager)
            break;
    }

    if (!Manager)
    {
        UE_LOG("[error] GameJamGameMode: LevelTransitionManager not found!");
        return;
    }

    // 씬 타입으로 NextLevel 설정 (간단하게)
    if (CurrentSceneType == ESceneType::Intro)
    {
        // TestButton.scene (Intro) -> TestTimer.scene
        Manager->SetNextLevel(L"Data/Scenes/TestTimer.scene");
        UE_LOG("[info] GameJamGameMode: Set NextLevel -> TestTimer.scene");
    }
    else if (TimerTrigger != nullptr)
    {
        // TestTimer.scene (타이머 있음) -> TestVolume.scene
        Manager->SetNextLevel(L"Data/Scenes/TestVolume.scene");
        UE_LOG("[info] GameJamGameMode: Set NextLevel -> TestVolume.scene");
    }
    else
    {
        // TestVolume.scene (타이머 없음) -> TestButton.scene (loop)
        Manager->SetNextLevel(L"Data/Scenes/TestButton.scene");
        UE_LOG("[info] GameJamGameMode: Set NextLevel -> TestButton.scene (loop)");
    }
}

void AGameJamGameMode::ClearAllWidgets()
{
    if (!SGameHUD::Get().IsInitialized())
        return;

    if (StartButton) { SGameHUD::Get().RemoveWidget(StartButton); StartButton.Reset(); }
    if (QuitButton) { SGameHUD::Get().RemoveWidget(QuitButton); QuitButton.Reset(); }
    if (RestartButton) { SGameHUD::Get().RemoveWidget(RestartButton); RestartButton.Reset(); }
    if (TimerText) { SGameHUD::Get().RemoveWidget(TimerText); TimerText.Reset(); }
    if (InfoText) { SGameHUD::Get().RemoveWidget(InfoText); InfoText.Reset(); }
    if (ScoreText) { SGameHUD::Get().RemoveWidget(ScoreText); ScoreText.Reset(); }
}
