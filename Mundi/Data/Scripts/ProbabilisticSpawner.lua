-- ProbabilisticSpawner.lua
-- 특정 위치에 정해진 경로의 프리팹을 확률적으로 1회 소환하는 스크립트

-- 스폰 설정
local SpawnChance = 0.5         -- 스폰 확률 (0.0 ~ 1.0, 0.5 = 50%)

-- 프리팹 목록과 각각의 가중치 (가중치가 높을수록 선택될 확률이 높음)
local PrefabList = {
    { path = "Data/Prefabs/OxygenTank.prefab", weight = 50 },
    { path = "Data/Prefabs/FireEX.prefab", weight = 50 }
    -- 필요한 프리팹 경로와 가중치를 추가하세요
    -- { path = "Data/Prefabs/Example.prefab", weight = 20 },
}

-- 스폰된 오브젝트 저장 (관리용)
local SpawnedObject = nil

-- 가중치 기반 랜덤 선택 함수
local function SelectWeightedRandom(list)
    local totalWeight = 0
    for _, item in ipairs(list) do
        totalWeight = totalWeight + item.weight
    end

    local randomValue = math.random() * totalWeight
    local accumulated = 0

    for _, item in ipairs(list) do
        accumulated = accumulated + item.weight
        if randomValue <= accumulated then
            return item.path
        end
    end

    -- 폴백: 첫 번째 항목 반환
    return list[1].path
end

-- 랜덤 오프셋 생성 함수 (옵션)
local function RandomInRange(minVal, maxVal)
    return minVal + (maxVal - minVal) * math.random()
end

-- 프리팹 스폰 함수 (1회 한정)
local function SpawnRandomPrefab()
    -- 확률 체크
    if math.random() > SpawnChance then
        print("[ProbabilisticSpawner] Spawn skipped (probability check failed)")
        return false
    end

    -- 가중치 기반으로 프리팹 선택
    local selectedPath = SelectWeightedRandom(PrefabList)

    -- 스폰 위치 설정 (현재 오브젝트 위치 기준)
    local spawnPos = Obj.Location

    -- 프리팹 스폰
    local spawnedObj = SpawnPrefab(selectedPath)
    if spawnedObj then
        spawnedObj.Location = spawnPos
        SpawnedObject = spawnedObj
        print("[ProbabilisticSpawner] Spawned: " .. selectedPath)
        return true
    end

    return false
end

function OnBeginPlay()
    -- 랜덤 시드 초기화 (위치 기반으로 각 스포너마다 다른 시드 사용)
    local pos = Obj.Location
    local seed = os.time() + math.floor((pos.X + pos.Y + pos.Z) * 1000)
    math.randomseed(seed)

    -- Lua의 math.random()은 시드 직후 편향된 값을 반환하므로 몇 번 버림
    math.random(); math.random(); math.random()

    print("[ProbabilisticSpawner] Initialized at position: " ..
          tostring(pos.X) .. ", " ..
          tostring(pos.Y) .. ", " ..
          tostring(pos.Z))

    -- 1회 스폰 시도
    SpawnRandomPrefab()
end

function OnEndPlay()
    -- 정리 작업
    SpawnedObject = nil
end

function OnBeginOverlap(OtherActor)
end

function OnEndOverlap(OtherActor)
end

function Update(dt)
end
