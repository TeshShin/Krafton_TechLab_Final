-- LevelTransitionTrigger.lua
-- ACharacter가 오버랩 시 지정된 장면으로 이동하는 스크립트

-- 설정
local NextScenePath = "Data/Scenes/Rescue.scene"  -- 전환할 장면 경로
local bTransitioned = false  -- 중복 전환 방지

function OnBeginPlay()
    print("[LevelTransitionTrigger] Initialized")
    print("[LevelTransitionTrigger] Target scene: " .. NextScenePath)
end

function OnEndPlay()
    bTransitioned = false
end

function OnBeginOverlap(OtherActor)
    -- 이미 전환 중이면 무시
    if bTransitioned then
        return
    end

    -- OtherActor가 유효한지 확인
    if not OtherActor then
        return
    end

    -- OtherActor가 파괴되었는지 확인
    if OtherActor:IsDestroyed() then
        return
    end

    -- ACharacter인지 확인 (GetOwnerAs로 캐스팅 시도)
    local character = GetOwnerAs(OtherActor, "ACharacter")
    if not character then
        -- AFirefighterCharacter도 시도
        character = GetOwnerAs(OtherActor, "AFirefighterCharacter")
    end

    if character then
        print("[LevelTransitionTrigger] Character detected! Transitioning to: " .. NextScenePath)
        bTransitioned = true
        TransitionToLevel(NextScenePath)
    end
end

function OnEndOverlap(OtherActor)
end

function Update(dt)
end
