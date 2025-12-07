#include "pch.h"
#include "FireActor.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "SphereComponent.h"
#include "ResourceManager.h"
#include "JsonSerializer.h"

AFireActor::AFireActor()
	: bIsActive(true)
	, FireIntensity(1.0f)
{
	ObjectName = "Fire Actor";

	// 불 파티클 컴포넌트 생성
	FireParticle = CreateDefaultSubobject<UParticleSystemComponent>("FireParticle");
	if (FireParticle)
	{
		RootComponent = FireParticle;
		FireParticle->bAutoActivate = true;

		// IntenseFire 파티클 로드
		UParticleSystem* FireEffect = UResourceManager::GetInstance().Load<UParticleSystem>("Data/Particles/IntenseFire.particle");
		if (FireEffect)
		{
			FireParticle->SetTemplate(FireEffect);
		}
	}

	// 데미지 감지용 스피어 컴포넌트 생성
	DamageSphere = CreateDefaultSubobject<USphereComponent>("DamageSphere");
	if (DamageSphere && FireParticle)
	{
		DamageSphere->SetupAttachment(FireParticle);
		DamageSphere->SetSphereRadius(FireRadius);
		DamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DamageSphere->SetSimulatePhysics(false);
		DamageSphere->SetGenerateOverlapEvents(true);
		DamageSphere->CollisionMask = CollisionMasks::Pawn;  // 캐릭터만 감지
	}
}

AFireActor::~AFireActor()
{
}

void AFireActor::BeginPlay()
{
	Super::BeginPlay();

	if (bIsActive && FireParticle)
	{
		FireParticle->ActivateSystem();
	}
}

void AFireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 불이 활성화 상태이고 데미지 스피어가 있으면 오버랩된 액터에 데미지
	// (데미지 로직은 Lua 스크립트에서 처리하거나 추후 구현)
}

void AFireActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	for (UActorComponent* Component : OwnedComponents)
	{
		if (UParticleSystemComponent* Particle = Cast<UParticleSystemComponent>(Component))
		{
			FireParticle = Particle;
		}
		else if (USphereComponent* Sphere = Cast<USphereComponent>(Component))
		{
			DamageSphere = Sphere;
		}
	}
}

void AFireActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FireParticle = Cast<UParticleSystemComponent>(RootComponent);

		for (UActorComponent* Component : OwnedComponents)
		{
			if (USphereComponent* Sphere = Cast<USphereComponent>(Component))
			{
				DamageSphere = Sphere;
				break;
			}
		}

		// 불 상태 로드
		FJsonSerializer::ReadBool(InOutHandle, "bIsActive", bIsActive, true);
		FJsonSerializer::ReadFloat(InOutHandle, "FireIntensity", FireIntensity, 1.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "DamagePerSecond", DamagePerSecond, 10.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "FireRadius", FireRadius, 2.0f);
		FJsonSerializer::ReadFloat(InOutHandle, "WaterDamageMultiplier", WaterDamageMultiplier, 1.0f);

		// 로딩 후 반경 업데이트
		if (DamageSphere)
		{
			DamageSphere->SetSphereRadius(FireRadius);
		}
	}
	else
	{
		// 불 상태 저장
		InOutHandle["bIsActive"] = bIsActive;
		InOutHandle["FireIntensity"] = FireIntensity;
		InOutHandle["DamagePerSecond"] = DamagePerSecond;
		InOutHandle["FireRadius"] = FireRadius;
		InOutHandle["WaterDamageMultiplier"] = WaterDamageMultiplier;
	}
}

void AFireActor::SetFireActive(bool bActive)
{
	bIsActive = bActive;

	if (FireParticle)
	{
		if (bActive)
		{
			FireParticle->ResetParticles();
			FireParticle->ActivateSystem();
		}
		else
		{
			FireParticle->DeactivateSystem();
		}
	}
}

void AFireActor::SetFireIntensity(float Intensity)
{
	FireIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);

	// 파티클 스케일 조절로 세기 표현
	// Actor 자체의 스케일로 조절
	float Scale = 0.5f + FireIntensity * 0.5f;  // 0.5 ~ 1.0 스케일
	SetActorScale(FVector(Scale, Scale, Scale));

	// 불이 완전히 꺼지면 비활성화
	if (FireIntensity <= 0.0f)
	{
		SetFireActive(false);
	}
}

void AFireActor::ApplyWaterDamage(float DamageAmount)
{
	if (!bIsActive)
	{
		UE_LOG("ApplyWaterDamage: Fire is not active, skipping");
		return;
	}

	// 물 데미지를 불 세기에 적용
	float ActualDamage = DamageAmount * WaterDamageMultiplier;
	float NewIntensity = FireIntensity - ActualDamage;

	UE_LOG("ApplyWaterDamage: DamageAmount=%.4f, Multiplier=%.2f, ActualDamage=%.4f, OldIntensity=%.2f, NewIntensity=%.2f",
		DamageAmount, WaterDamageMultiplier, ActualDamage, FireIntensity, NewIntensity);

	SetFireIntensity(NewIntensity);
}
