-- ============================================================================
-- RescuedPeople.lua
-- 구조된 사람들 애니메이션 컨트롤러
-- BeginPlay에서 랜덤 애니메이션 재생
-- ============================================================================

-- 애니메이션 목록 (FBX 내부 애니메이션 이름 - mixamo.com 형식)
local Animations = {
    "Data/People/Dance1_mixamo.com",
    "Data/People/Dance2_mixamo.com",
    "Data/People/Dance3_mixamo.com",
    "Data/People/Fallen_Idle_mixamo.com",
    "Data/People/Knee_Praying_mixamo.com",
    "Data/People/Stand_Praying_mixamo.com"
}

-- 상태 변수
local SkeletalMeshComp = nil
local StateMachine = nil

function OnBeginPlay()
    print("[RescuedPeople] BeginPlay called")

    -- SkeletalMeshComponent 가져오기
    SkeletalMeshComp = GetComponent(Obj, "USkeletalMeshComponent")
    if not SkeletalMeshComp then
        print("[RescuedPeople] ERROR: No USkeletalMeshComponent found")
        return
    end

    -- 랜덤 시드 설정 (매번 다른 애니메이션 선택)
    math.randomseed(math.floor(os.time() + (Obj.Location.X * 1000) + (Obj.Location.Y * 100)))

    -- 랜덤 애니메이션 선택
    local randomIndex = math.random(1, #Animations)
    local selectedAnim = Animations[randomIndex]

    print("[RescuedPeople] Selected animation: " .. selectedAnim)

    -- 상태 머신 설정
    SkeletalMeshComp:UseStateMachine()
    StateMachine = SkeletalMeshComp:GetOrCreateStateMachine()

    if StateMachine then
        StateMachine:Clear()

        -- 상태 추가 (이름, 에셋 경로, 재생 속도, 루핑 여부)
        local stateIdx = StateMachine:AddState("Anim", selectedAnim, 1.0, true)
        print("[RescuedPeople] AddState returned: " .. tostring(stateIdx))

        if stateIdx >= 0 then
            StateMachine:SetState("Anim", 0.0)
            print("[RescuedPeople] Animation started successfully")
        else
            print("[RescuedPeople] ERROR: Failed to add state (animation not found?)")
        end
    else
        print("[RescuedPeople] ERROR: Failed to create state machine")
    end
end

function Update(dt)
    -- 현재는 특별한 업데이트 로직 없음
end

function OnEndPlay()
    print("[RescuedPeople] EndPlay called")
    SkeletalMeshComp = nil
    StateMachine = nil
end
