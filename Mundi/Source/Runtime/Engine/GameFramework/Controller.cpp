// ────────────────────────────────────────────────────────────────────────────
// Controller.cpp
// Controller 클래스 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "Controller.h"
#include "Pawn.h"

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

AController::AController()
	: PossessedPawn(nullptr)
	, ControlRotation(FQuat::Identity())
	, ControlPitch(0.0f)
	, ControlYaw(0.0f)
{
}

AController::~AController()
{
	// 빙의 해제
	if (PossessedPawn)
	{
		UnPossess();
	}
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void AController::BeginPlay()
{
	Super::BeginPlay();
}

void AController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

// ────────────────────────────────────────────────────────────────────────────
// Pawn 빙의 관련
// ────────────────────────────────────────────────────────────────────────────

void AController::Possess(APawn* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	// 이미 빙의한 Pawn이 있으면 먼저 빙의 해제
	if (PossessedPawn)
	{
		UnPossess();
	}

	// 새 Pawn 빙의
	PossessedPawn = InPawn;
	PossessedPawn->PossessedBy(this);

	// 이벤트 호출
	OnPossess(InPawn);
}

void AController::UnPossess()
{
	if (!PossessedPawn)
	{
		return;
	}

	// 이벤트 호출
	OnUnPossess();

	// Pawn에게 빙의 해제 알림
	PossessedPawn->UnPossessed();
	PossessedPawn = nullptr;
}

void AController::OnPossess(APawn* InPawn)
{
	// 기본 구현: 아무것도 하지 않음
	// 파생 클래스에서 오버라이드
}

void AController::OnUnPossess()
{
	// 기본 구현: 아무것도 하지 않음
	// 파생 클래스에서 오버라이드
}

// ────────────────────────────────────────────────────────────────────────────
// Control Rotation
// ────────────────────────────────────────────────────────────────────────────

void AController::SetControlRotation(const FQuat& NewRotation)
{
	// 쿼터니언에서 Pitch/Yaw 추출
	FVector Euler = NewRotation.GetNormalized().ToEulerZYXDeg();
	ControlPitch = FMath::Clamp(Euler.Y, -89.0f, 89.0f);
	ControlYaw = Euler.Z;
	while (ControlYaw >= 360.0f) ControlYaw -= 360.0f;
	while (ControlYaw < 0.0f) ControlYaw += 360.0f;
	UpdateControlRotation();
}

void AController::AddControlRotation(const FQuat& DeltaRotation)
{
	FQuat NewRot = (ControlRotation * DeltaRotation).GetNormalized();
	SetControlRotation(NewRot);
}

void AController::AddYawInput(float DeltaYaw)
{
	if (DeltaYaw != 0.0f)
	{
		ControlYaw += DeltaYaw;
		while (ControlYaw >= 360.0f) ControlYaw -= 360.0f;
		while (ControlYaw < 0.0f) ControlYaw += 360.0f;
		UpdateControlRotation();
	}
}

void AController::AddPitchInput(float DeltaPitch)
{
	if (DeltaPitch != 0.0f)
	{
		ControlPitch += DeltaPitch;
		ControlPitch = FMath::Clamp(ControlPitch, -89.0f, 89.0f);
		UpdateControlRotation();
	}
}

void AController::AddRollInput(float DeltaRoll)
{
	// Roll은 카메라 제어에서 사용하지 않음 (항상 0 유지)
	// 필요시 별도 ControlRoll 변수 추가
}

void AController::UpdateControlRotation()
{
	// Roll = 0, Pitch, Yaw로 쿼터니언 생성 (변환은 한 번만)
	ControlRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, ControlPitch, ControlYaw));
}

// ────────────────────────────────────────────────────────────────────────────
// 복제
// ────────────────────────────────────────────────────────────────────────────

void AController::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}

void AController::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// TODO: 필요한 멤버 변수 직렬화 추가
}
