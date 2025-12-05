#include "pch.h"
#include "LevelTransitionManager.h"
#include "World.h"
#include <filesystem>

IMPLEMENT_CLASS(ALevelTransitionManager)

ALevelTransitionManager::ALevelTransitionManager()
{
    ObjectName = "LevelTransitionManager";
    bCanEverTick = false;

    SetPersistAcrossLevelTransition(true);

    UE_LOG("[info] LevelTransitionManager: Created (Persistent Actor)");
}

ALevelTransitionManager::~ALevelTransitionManager()
{
    UE_LOG("[info] LevelTransitionManager: Destroyed");
}

void ALevelTransitionManager::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG("[info] LevelTransitionManager: BeginPlay (Ready for level transitions)");
}

// ════════════════════════════════════════════════════════════════════════
// 레벨 전환 API

void ALevelTransitionManager::TransitionToLevel(const FWideString& LevelPath)
{
    // 1. 에러 체크

    // 이미 전환 중이면 무시
    if (IsTransitioning())
    {
        UE_LOG("[warning] LevelTransitionManager: Already transitioning to another level");
        return;
    }

    // 경로가 비어있으면 에러
    if (LevelPath.empty())
    {
        UE_LOG("[error] LevelTransitionManager: Empty level path");
        return;
    }

    // 파일이 존재하지 않으면 에러
    if (!DoesLevelFileExist(LevelPath))
    {
        UE_LOG("[error] LevelTransitionManager: Level file does not exist: %s",
               WideToUTF8(LevelPath).c_str());
        return;
    }

    // 2. 전환 실행

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG("[error] LevelTransitionManager: World is null");
        return;
    }

    UE_LOG("[info] LevelTransitionManager: Transitioning to level: %s",
           WideToUTF8(LevelPath).c_str());

    TransitionState = ELevelTransitionState::Transitioning;

    // World의 LoadLevelFromFile 호출
    // ※ 내부에서 SetLevel()이 호출되며, Persistent Actor는 자동으로 보존됨
    bool bSuccess = World->LoadLevelFromFile(LevelPath);

    if (bSuccess)
    {
        UE_LOG("[info] LevelTransitionManager: Level transition successful");
    }
    else
    {
        UE_LOG("[error] LevelTransitionManager: Level transition failed");
    }

    TransitionState = ELevelTransitionState::Idle;
}

// ════════════════════════════════════════════════════════════════════════
// 유틸리티

bool ALevelTransitionManager::DoesLevelFileExist(const FWideString& LevelPath) const
{
    try
    {
        std::filesystem::path FilePath(LevelPath);
        return std::filesystem::exists(FilePath);
    }
    catch (const std::exception& e)
    {
        UE_LOG("[error] LevelTransitionManager: Exception while checking file existence: %s", e.what());
        return false;
    }
}
