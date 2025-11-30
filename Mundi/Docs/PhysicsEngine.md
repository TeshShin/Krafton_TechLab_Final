# PhysicsEngine 상세 분석 문서

## 목차
1. [개요](#1-개요)
2. [파일 구조](#2-파일-구조)
3. [클래스 구조와 상속 관계](#3-클래스-구조와-상속-관계)
4. [물리 시뮬레이션 흐름](#4-물리-시뮬레이션-흐름)
5. [충돌 감지 시스템](#5-충돌-감지-시스템)
6. [강체(Rigid Body) 구현](#6-강체rigid-body-구현)
7. [물리 월드 관리](#7-물리-월드-관리)
8. [제약 조건(Constraints)](#8-제약-조건constraints)
9. [수학적 기법 및 좌표계 변환](#9-수학적-기법-및-좌표계-변환)
10. [외부 라이브러리](#10-외부-라이브러리)
11. [함수별 상세 설명](#11-함수별-상세-설명)
12. [아키텍처 흐름도](#12-아키텍처-흐름도)
13. [현재 구현 상태](#13-현재-구현-상태)

---

## 1. 개요

Mundi 엔진의 물리 시스템은 **NVIDIA PhysX 5.x SDK**를 래핑한 구조로 구현되어 있습니다. 강체 물리 시뮬레이션, 충돌 감지, 제약 조건 등의 기능을 제공합니다.

### 주요 특징
- PhysX SDK 기반의 안정적인 물리 시뮬레이션
- 멀티스레드 지원 (CPU 논리 코어 수 기반)
- CCD(Continuous Collision Detection) 지원
- 스켈레탈 메시별 물리 에셋 관리
- 에디터 통합 (리플렉션 시스템 연동)

---

## 2. 파일 구조

```
Source/Runtime/Engine/PhysicsEngine/
├── PhysicsTypes.h/cpp        # 기본 열거형 및 인터페이스 정의
├── PhysXPublic.h             # PhysX 타입 변환 및 Lock 클래스
├── PhysXSupport.h/cpp        # PhysX SDK 초기화/종료
├── PhysScene.h/cpp           # 물리 씬 관리, 시뮬레이션 루프
├── BodyInstance.h/cpp        # 개별 강체 인스턴스
├── BodySetup.h/cpp           # 충돌 형태 및 물리 재질 관리
├── FBodySetup.h              # 스켈레톤 본 바디 설정 데이터
├── FConstraintSetup.h        # 제약 조건 설정 데이터
├── PhysicsAsset.h/cpp        # 물리 데이터 에셋
├── AggregateGeom.h/cpp       # 기하학 집합체
├── ShapeElem.h/cpp           # 기본 충돌체 인터페이스
└── BoxElem.h/cpp             # 박스 충돌체 요소
```

### 파일별 역할

| 파일 | 역할 |
|------|------|
| **PhysicsTypes.h/cpp** | `EPhysicsShapeType`, `EConstraintType` 열거형 및 인터페이스 정의 |
| **PhysXPublic.h** | PhysX 씬 읽기/쓰기 Lock 클래스, 엔진↔PhysX 타입 변환 함수 |
| **PhysXSupport.h/cpp** | PhysX SDK 초기화/종료, 전역 포인터 관리 |
| **PhysScene.h/cpp** | 물리 씬 관리, 시뮬레이션 루프 (StartFrame → Tick → Wait → Process) |
| **BodyInstance.h/cpp** | 개별 강체 인스턴스 (PhysX PxRigidActor 래퍼) |
| **FBodySetup.h** | 스켈레톤 본에 연결된 바디 설정 데이터 구조 |
| **BodySetup.h/cpp** | UBodySetup 클래스 - 충돌 형태 및 물리 재질 관리 |
| **FConstraintSetup.h** | 두 바디 간의 제약 조건 설정 (BallAndSocket, Hinge) |
| **PhysicsAsset.h/cpp** | 스켈레탈 메시의 물리 데이터 에셋 |
| **AggregateGeom.h/cpp** | 여러 기하학적 요소를 모으는 집합체 |
| **BoxElem.h/cpp** | 박스 충돌체 요소 |
| **ShapeElem.h/cpp** | 기본 충돌체 인터페이스 (Sphere, Box, Capsule 등) |

---

## 3. 클래스 구조와 상속 관계

```
UObject
├── UResourceBase
│   └── UPhysicsAsset          # 물리 에셋 - 바디 및 제약조건 관리
│
└── UBodySetup                  # 충돌체 설정
    └── FKAggregateGeom         # 기하학 집합체
        └── FKShapeElem         # 기본 형태
            └── FKBoxElem       # 박스

FBodyInstance                   # 강체 인스턴스 - PhysX 래퍼
├── PhysX::PxRigidActor
├── PhysX::PxRigidDynamic      # 동적 강체
└── PhysX::PxRigidStatic       # 정적 강체

FPhysScene                      # 물리 씬 관리
└── PhysX::PxScene

// 설정 데이터 구조체
├── FBodySetup                  # 바디 설정 데이터
├── FConstraintSetup            # 제약조건 설정 데이터
├── FKBoxElem                   # 박스 요소
├── FKAggregateGeom             # 기하학 집합체
└── FKShapeElem                 # 기본 형태
```

---

## 4. 물리 시뮬레이션 흐름

### 4.1 초기화 흐름

```cpp
InitGamePhys()                           // [PhysXSupport.cpp]
├── PxCreateFoundation()                 // PhysX 기초 설정
├── PxCreatePvd()                        // Visual Debugger 생성
├── PxCreatePhysics()                    // PhysX SDK 생성
│   └── PScale: 길이(1.0f=1m), 속도(10.0f)
├── PxInitExtensions()                   // 확장 모듈 초기화
├── PxCreateCooking()                    // 메시 쿠킹 (1mm 웰드 임계값)
└── PxDefaultCpuDispatcher               // 멀티스레드 디스패처

FPhysScene::InitPhysScene()              // [PhysScene.cpp]
├── PxSceneDesc 설정
│   ├── PCM(Persistent Contact Manifold) 활성화
│   ├── 안정화 모드 활성화
│   ├── Active Actors 추적 활성화
│   ├── CCD(Continuous Collision Detection) 활성화
│   └── 중력: (0, 0, -9.81)
└── PxScene 생성

FBodyInstance::InitBody()                // [BodyInstance.cpp]
├── bSimulatePhysics 확인
├── PxRigidDynamic 또는 PxRigidStatic 생성
├── UBodySetup::AddShapesToRigidActor()  // 형태 추가
├── 동적일 경우:
│   ├── PxRigidBodyExt::setMassAndUpdateInertia()
│   └── setLinearDamping(), setAngularDamping()
└── PhysScene에 Actor 추가
```

### 4.2 프레임 업데이트 흐름

```cpp
FPhysScene::StartFrame()                 // [PhysScene.cpp]
├── DeltaTime 획득
└── TickPhysScene(DeltaTime)
    └── PhysXScene->simulate(DeltaTime)
        └── bPhysXSceneExecuting = true

// [물리 시뮬레이션 비동기 실행 중...]

FPhysScene::EndFrame()                   // [PhysScene.cpp]
├── WaitPhysScene()
│   └── PhysXScene->fetchResults(true)   // 시뮬레이션 완료 대기
└── ProcessPhysScene()
    └── SyncComponentsToBodies()
        ├── getActiveActors()            // 활동 중인 액터 목록
        └── GetUnrealWorldTransform() → SetWorldTransform()
```

### 4.3 PhysX 내부 처리

PhysX SDK가 처리하는 주요 알고리즘:
- **적분(Integration)**: 위치, 속도, 가속도 계산
- **충돌 감지**: Broadphase (AABB), Narrowphase (Shape vs Shape)
- **제약 조건 해결**: Iterative constraint solver
- **CCD**: Swept volume 사용

---

## 5. 충돌 감지 시스템

### 5.1 충돌 필터링

```cpp
// PhysScene.cpp
SceneDesc.filterShader = PxDefaultSimulationFilterShader;
```

PhysX의 기본 필터 셰이더를 사용하며, 충돌 그룹 및 응답 설정이 가능합니다.

### 5.2 충돌 형태 추가 프로세스

`UBodySetup::AddShapesToRigidActor_AssumesLocked()` 함수:

```cpp
1. FKBoxElem 반복
2. GetCollisionEnabled() 확인
   ├── NoCollision       → 건너뜀
   ├── QueryOnly         → SIMULATION_SHAPE=false, SCENE_QUERY_SHAPE=true
   ├── PhysicsOnly       → SIMULATION_SHAPE=true, SCENE_QUERY_SHAPE=false
   └── QueryAndPhysics   → 둘 다 true
3. GetPxGeometry()       → PhysX 형태로 변환 (스케일 적용)
4. setLocalPose()        → 로컬 변환 설정
5. attachShape()         → RigidActor에 부착
```

### 5.3 충돌 타입

| 타입 | 설명 |
|------|------|
| `NoCollision` | 충돌 없음 |
| `QueryOnly` | 쿼리만 가능 (레이캐스트 등) |
| `PhysicsOnly` | 물리 시뮬레이션만 |
| `QueryAndPhysics` | 모든 충돌 처리 |

---

## 6. 강체(Rigid Body) 구현

### 6.1 강체 타입

```cpp
// FBodyInstance.cpp
if (bSimulatePhysics)
{
    RigidActor = GPhysXSDK->createRigidDynamic(PhysicsTransform);
}
else
{
    RigidActor = GPhysXSDK->createRigidStatic(PhysicsTransform);
}
```

| 타입 | 설명 | 특성 |
|------|------|------|
| **Dynamic** | 시뮬레이션되는 강체 | 질량, 속도, 가속도 업데이트 |
| **Static** | 정적 배경 | 이동하지 않음, 성능 최적화 |
| **Kinematic** | 애니메이션된 강체 | 코드로 움직임, 물리 반응 없음 |

### 6.2 강체 속성 (FBodySetup.h)

```cpp
float Mass = 1.0f;              // 질량 (kg)
float LinearDamping = 0.01f;    // 선형 저항
float AngularDamping = 0.0f;    // 회전 저항
bool bEnableGravity = true;     // 중력 적용 여부
```

### 6.3 질량 및 관성 계산

```cpp
// BodyInstance.cpp
if (bOverrideMass)
{
    // 사용자 정의 질량
    PxRigidBodyExt::setMassAndUpdateInertia(*DynamicActor, MassInKgOverride);
}
else
{
    // 밀도 기반 자동 계산 (1x1x1 박스 = 10kg 기준)
    PxRigidBodyExt::updateMassAndInertia(*DynamicActor, 10.0f);
}
```

### 6.4 강체 제어 API

| 함수 | 설명 |
|------|------|
| `SetLinearVelocity(FVector, bool bAddToCurrent)` | 선형 속도 설정 (절대/누적) |
| `AddForce(FVector, EForceMode)` | 힘 추가 (Force/Acceleration) |
| `AddTorque(FVector, EForceMode)` | 회전 토크 추가 |
| `IsDynamic()` | 동적 강체 여부 확인 |
| `GetUnrealWorldTransform()` | 월드 트랜스폼 조회 |

---

## 7. 물리 월드 관리

### 7.1 PhysX SDK 전역 포인터

```cpp
// PhysXSupport.h
extern PxFoundation*            GPhysXFoundation;     // 기초 시스템
extern PxPhysics*               GPhysXSDK;            // 물리 엔진
extern PxCooking*               GPhysXCooking;        // 메시 쿠킹
extern PxDefaultCpuDispatcher*  GPhysXDispatcher;     // 멀티스레드
extern PxPvd*                   GPhysXVisualDebugger; // 디버거
extern FPhysXAllocator*         GPhysXAllocator;      // 메모리
extern FPhysXErrorCallback*     GPhysXErrorCallback;  // 에러처리
```

### 7.2 FPhysScene 클래스

```cpp
class FPhysScene
{
    PxScene* PhysXScene;        // PhysX 씬 인스턴스
    UWorld* OwningWorld;        // 소유 월드
    bool bPhysXSceneExecuting;  // 시뮬레이션 중 플래그
};
```

### 7.3 씬 설정 상세

```cpp
// PhysScene.cpp
PxSceneDesc SceneDesc(GPhysXSDK->getTolerancesScale());
SceneDesc.cpuDispatcher = GPhysXDispatcher;
SceneDesc.filterShader = PxDefaultSimulationFilterShader;

// 안정성 플래그
SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;           // 지속적 접촉 매니폴드
SceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION; // 안정화 모드
SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS; // 활동 추적
SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;           // 연속 충돌 감지

SceneDesc.gravity = PxVec3(0, 0, -9.81);               // 하향 중력 (m/s²)
```

---

## 8. 제약 조건(Constraints)

### 8.1 제약 조건 타입

```cpp
// PhysicsTypes.h
enum class EConstraintType : uint8
{
    BallAndSocket,  // 구형 조인트 (3축 회전)
    Hinge,          // 힌지 조인트 (1축 회전)
};
```

### 8.2 제약 조건 설정 데이터

```cpp
// FConstraintSetup.h
struct FConstraintSetup
{
    FName JointName;            // 조인트 이름
    int32 ParentBodyIndex;      // 부모 바디 인덱스
    int32 ChildBodyIndex;       // 자식 바디 인덱스

    EConstraintType ConstraintType;  // 조인트 타입

    // 각도 제한 (도 단위)
    float Swing1Limit = 45.0f;       // Y축 회전 제한
    float Swing2Limit = 45.0f;       // Z축 회전 제한
    float TwistLimitMin = -45.0f;    // X축 최소 회전
    float TwistLimitMax = 45.0f;     // X축 최대 회전

    // 스프링 속성
    float Stiffness = 0.0f;          // 강성 (k)
    float Damping = 0.0f;            // 감쇠 (c)
};
```

### 8.3 제약 조건 관리 API

| 메서드 | 기능 |
|--------|------|
| `AddConstraint()` | 제약조건 추가 (유효성 검사) |
| `RemoveConstraint()` | 제약조건 제거 |
| `FindConstraintIndex()` | 두 바디 간 제약조건 검색 |

> **참고**: 현재 제약 조건은 데이터 구조만 정의되어 있으며, 실제 PhysX PxJoint 생성은 미구현 상태입니다.

---

## 9. 수학적 기법 및 좌표계 변환

### 9.1 엔진 ↔ PhysX 좌표 변환

```cpp
// PhysXPublic.h

// 벡터 변환
inline PxVec3 U2PVector(const FVector& UVec)
{
    return PxVec3(UVec.X, UVec.Y, UVec.Z);
}

inline FVector P2UVector(const PxVec3& PVec)
{
    return FVector(PVec.x, PVec.y, PVec.z);
}

// 행렬 변환
// Future Engine: Row-major, Left-multiplication
// PhysX: Column-major, Right-multiplication
// → 메모리상 동일하므로 직접 복사 가능
inline PxMat44 U2PMatrix(const FMatrix& UMat)
{
    PxMat44 Result;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            Result[r][c] = UMat.M[r][c];
    return Result;
}
```

### 9.2 스케일 적용

```cpp
// BodySetup.cpp
FVector ScaledCenter;
ScaledCenter.X = ElemTM.Translation.X * Scale3D.X;
ScaledCenter.Y = ElemTM.Translation.Y * Scale3D.Y;
ScaledCenter.Z = ElemTM.Translation.Z * Scale3D.Z;

PxTransform PxLocalPose(U2PVector(ScaledCenter), U2PQuat(ElemTM.Rotation));
NewShape->setLocalPose(PxLocalPose);
```

### 9.3 박스 기하학 계산

```cpp
// BoxElem.h
PxBoxGeometry GetPxGeometry(const FVector& Scale3D) const
{
    float HalfX = (X * 0.5f) * AbsScaleX;  // 지름 → 반지름
    float HalfY = (Y * 0.5f) * AbsScaleY;
    float HalfZ = (Z * 0.5f) * AbsScaleZ;

    // 최소 범위 0.01m 보장 (안정성)
    return PxBoxGeometry(
        FMath::Max(HalfX, 0.01f),
        FMath::Max(HalfY, 0.01f),
        FMath::Max(HalfZ, 0.01f)
    );
}
```

---

## 10. 외부 라이브러리

### 10.1 PhysX SDK

**사용 버전**: PhysX 5.x (`PX_PHYSICS_VERSION`)

**주요 모듈**:
- `PxFoundation` - 기초 시스템 (메모리, 에러 처리)
- `PxPhysics` - 물리 엔진 인스턴스
- `PxScene` - 물리 씬
- `PxCooking` - 메시 쿠킹 (삼각형 메시 → 물리용 형태)
- `PxDefaultCpuDispatcher` - 멀티스레드 작업 분배
- `PxPvd` - Visual Debugger (연결: 127.0.0.1:5425)

### 10.2 메모리 관리

```cpp
// PhysXSupport.h
class FPhysXAllocator : public PxDefaultAllocator
{
    // 현재: PhysX 기본 할당자 사용
    // 확장: 커스텀 메모리 풀 구현 가능
};
```

### 10.3 에러 처리

```cpp
// PhysXSupport.h
class FPhysXErrorCallback : public PxErrorCallback
{
    virtual void reportError(
        PxErrorCode::Enum code,
        const char* message,
        const char* file,
        int line
    );
    // 에러 코드별 분류 및 엔진 로그 시스템 연동
};
```

---

## 11. 함수별 상세 설명

### 11.1 PhysXSupport.cpp

| 함수 | 역할 | 세부 사항 |
|------|------|----------|
| `InitGamePhys()` | PhysX SDK 초기화 | Foundation→SDK→Cooking→Dispatcher 순서 |
| `TermGamePhys()` | PhysX SDK 정리 | 역순으로 해제 |

### 11.2 PhysScene.cpp

| 함수 | 역할 | 세부 사항 |
|------|------|----------|
| `InitPhysScene()` | 씬 생성 및 설정 | 플래그/중력 설정 |
| `TermPhysScene()` | 씬 해제 | Actor 제거 후 씬 release |
| `StartFrame()` | 시뮬레이션 시작 | TickPhysScene(DeltaTime) 호출 |
| `TickPhysScene()` | 물리 시뮬레이션 실행 | simulate() 비동기 실행 |
| `WaitPhysScene()` | 시뮬레이션 완료 대기 | fetchResults(true) 블로킹 |
| `ProcessPhysScene()` | 결과 처리 | SyncComponentsToBodies() |
| `SyncComponentsToBodies()` | 트랜스폼 동기화 | 활동 Actor → 컴포넌트 업데이트 |

### 11.3 BodyInstance.cpp

| 함수 | 역할 | 세부 사항 |
|------|------|----------|
| `InitBody()` | 강체 생성/초기화 | Dynamic/Static 선택, 형태 추가 |
| `TermBody()` | 강체 해제 | 씬에서 제거, 메모리 해제 |
| `GetUnrealWorldTransform()` | 월드 트랜스폼 조회 | PxTransform → FTransform |
| `SetLinearVelocity()` | 선형 속도 설정 | 절대/누적 모드 |
| `AddForce()` | 힘 추가 | Force/Acceleration 모드 |
| `AddTorque()` | 토크 추가 | Force/Acceleration 모드 |
| `IsDynamic()` | 동적 여부 확인 | is<PxRigidDynamic>() |

### 11.4 BodySetup.cpp

| 함수 | 역할 | 세부 사항 |
|------|------|----------|
| `AddShapesToRigidActor_AssumesLocked()` | 형태 추가 | 충돌 설정, 로컬 포즈 |
| `AddBox()` | 테스트용 박스 추가 | FKBoxElem 생성 |

### 11.5 PhysicsAsset.cpp

| 함수 | 역할 | 세부 사항 |
|------|------|----------|
| `AddBody()` | 바디 추가 | 중복 검사, 인덱스 맵 업데이트 |
| `RemoveBody()` | 바디 제거 | 연결 제약 제거, 인덱스 재조정 |
| `FindBodyIndex()` | 바디 검색 (이름) | 캐시된 맵 사용 |
| `FindBodyIndexByBone()` | 바디 검색 (본 인덱스) | 선형 탐색 |
| `AddConstraint()` | 제약 추가 | 중복/유효성 검증 |
| `RemoveConstraint()` | 제약 제거 | 배열에서 제거 |
| `ClearAll()` | 모두 초기화 | 바디와 제약 비우기 |
| `RebuildIndexMap()` | 캐시 재구성 | 바디 이름→인덱스 매핑 |

---

## 12. 아키텍처 흐름도

```
UWorld (게임 월드)
    │
    ▼
FPhysScene (물리 씬)
    │
    ▼
┌─────────────────────────────────────────────┐
│                 PhysX SDK                    │
├─────────────────────────────────────────────┤
│ PxFoundation (기초)                          │
│ PxPhysics (엔진)                             │
│ PxScene (씬)                                 │
│   └── PxRigidActor[] (강체들)                │
│       ├── PxShape[] (충돌 형태)              │
│       └── PxJoint[] (제약조건)               │
│ PxCooking (메시 변환)                        │
│ PxDispatcher (멀티스레드)                    │
└─────────────────────────────────────────────┘

[월드 업데이트 루프]

StartFrame()          ┐
    │                 │ 비동기 실행
    ▼                 │
TickPhysScene()       │ (병렬 처리)
    │                 │
    ▼                 │
[시뮬레이션 실행]     │
    │                 │
    ▼                 ┘
EndFrame()
    │
    ▼
WaitPhysScene()       ← 완료 대기
    │
    ▼
ProcessPhysScene()
    ├── SyncComponentsToBodies()
    │   └── GetUnrealWorldTransform() → SetWorldTransform()
    │
    ▼
다음 프레임으로
```

---

## 13. 현재 구현 상태

### 13.1 구현 완료

- ✅ PhysX SDK 초기화/종료
- ✅ 단순 강체 생성 (Dynamic/Static)
- ✅ 박스 충돌체
- ✅ 기본 물리 시뮬레이션
- ✅ 트랜스폼 동기화
- ✅ 선형 속도, 힘, 토크 제어
- ✅ 물리 에셋 데이터 구조

### 13.2 미구현/개선 필요

- ❌ **제약 조건 실제 구현** (PxJoint 생성 필요)
- ❌ **Sphere/Capsule 충돌체** (FKShapeElem 상속만 선언됨)
- ❌ **충돌 이벤트 콜백** (Contact/Trigger reports)
- ❌ **씬 쿼리** (Raycast, Sweep, Overlap)
- ❌ **물리 재질 관리자** (현재 고정값 사용)
- ❌ **디버그 렌더링** (PhysScene.cpp TODO)
- ❌ **커스텀 메모리 할당자**
- ❌ **IPhysicsAssetPreview 구현** (에디터 프리뷰)

---

## 14. 성능 최적화 포인트

| 최적화 | 설명 |
|--------|------|
| **멀티스레드** | CPU 논리 코어 수만큼 dispatcher 스레드 설정 |
| **CCD 활성화** | 고속 물체의 충돌 누락 방지 |
| **PCM 활성화** | 지속적 접촉 매니폴드 (안정성) |
| **Active Actors 추적** | 비활동 물체 제외로 업데이트 최소화 |
| **Broadphase 필터링** | 충돌 채널별 선택적 처리 |
| **스케일 검증** | 최소 크기 0.01m 보장으로 stability 확보 |

---

## 15. 사용 예시

### 15.1 물리 시스템 초기화

```cpp
// 게임 시작 시
InitGamePhys();

// 월드별 물리 씬 생성
World->PhysScene = new FPhysScene();
World->PhysScene->InitPhysScene();
```

### 15.2 강체 생성

```cpp
// 컴포넌트에서 강체 초기화
FBodyInstance BodyInstance;
BodyInstance.bSimulatePhysics = true;

UBodySetup* BodySetup = NewObject<UBodySetup>();
BodySetup->AddBox(FVector(100, 100, 100), FTransform::Identity);

BodyInstance.InitBody(BodySetup, ComponentTransform, PrimitiveComponent, PhysScene);
```

### 15.3 프레임 업데이트

```cpp
// 메인 루프
void UWorld::Tick(float DeltaTime)
{
    PhysScene->StartFrame();

    // 렌더링 등 다른 작업...

    PhysScene->EndFrame();
}
```

### 15.4 강체 제어

```cpp
// 힘 적용
BodyInstance.AddForce(FVector(0, 0, 1000), EForceMode::Force);

// 속도 설정
BodyInstance.SetLinearVelocity(FVector(100, 0, 0), false);

// 토크 적용
BodyInstance.AddTorque(FVector(0, 100, 0), EForceMode::Force);
```

---

## 16. 참고 자료

- NVIDIA PhysX SDK Documentation
- Unreal Engine Physics System Architecture
- Game Physics Engine Development (Ian Millington)
