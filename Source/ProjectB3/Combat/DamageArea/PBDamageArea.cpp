// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDamageArea.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "NavModifierComponent.h"
#include "Components/SphereComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/AbilitySystem/Components/PBTurnEffectComponent.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/ProjectB3.h"
#include "ProjectB3/Environment/Navigation/PBNavArea_Hazard.h"

APBDamageArea::APBDamageArea()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(200.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionComponent);

	// AI 경로 탐색 시 위험 영역 비용 부여 — NavMesh에 Hazard AreaClass 적용
	NavModifierComponent = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifierComponent"));
	NavModifierComponent->SetAreaClass(UPBNavArea_Hazard::StaticClass());
}

void APBDamageArea::InitDamageArea(const FGameplayEffectSpecHandle& InEffectSpec,
	UAbilitySystemComponent* InSourceASC,
	int32 InDurationRounds)
{
	EffectSpecHandle = InEffectSpec;
	SourceASC = InSourceASC;
	DurationRounds = InDurationRounds;

	CalculateExpectedDamage();
}

void APBDamageArea::BeginPlay()
{
	Super::BeginPlay();

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &APBDamageArea::OnAreaBeginOverlap);

	// 등록된 컴포넌트들을 지표면 높이로 스냅
	SnapComponentsToGround();

	// 라운드 기반 이펙트 갱신 및 수명 관리 등록
	if (UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		SpawnRound = CombatManager->GetCurrentRound();
		RoundChangedHandle = CombatManager->OnRoundChanged.AddUObject(
			this, &APBDamageArea::OnRoundChanged);
	}

	// EnvironmentSubsystem에 위험 영역 등록
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPBEnvironmentSubsystem* EnvSys = GI->GetSubsystem<UPBEnvironmentSubsystem>())
		{
			EnvSys->RegisterDamageArea(this);
		}
	}

	// 이미 영역 안에 있는 액터에게도 이펙트 적용
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		ApplyEffectToActor(Actor);
	}
}

void APBDamageArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// EnvironmentSubsystem에서 위험 영역 해제
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPBEnvironmentSubsystem* EnvSys = GI->GetSubsystem<UPBEnvironmentSubsystem>())
		{
			EnvSys->UnregisterDamageArea(this);
		}
	}

	if (RoundChangedHandle.IsValid())
	{
		if (UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
		{
			CombatManager->OnRoundChanged.Remove(RoundChangedHandle);
		}
		RoundChangedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void APBDamageArea::OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	ApplyEffectToActor(OtherActor);
}

void APBDamageArea::ApplyEffectToActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || !SourceASC.IsValid() || !EffectSpecHandle.IsValid())
	{
		return;
	}

	// CombatParticipant + AbilitySystemInterface 모두 구현한 액터만 대상
	IPBCombatParticipant* CombatParticipant = Cast<IPBCombatParticipant>(TargetActor);
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor);

	if (!CombatParticipant || !ASCInterface)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent();
	if (!IsValid(TargetASC))
	{
		return;
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
}

void APBDamageArea::OnRoundChanged(int32 NewRound)
{
	// 수명 만료 시 제거
	if (DurationRounds > 0 && (NewRound - SpawnRound) >= DurationRounds)
	{
		Destroy();
		return;
	}

	// 영역 내 잔류 액터에게 이펙트 재적용 (스택 갱신)
	ReapplyEffectToOverlappingActors();
}

void APBDamageArea::ReapplyEffectToOverlappingActors()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	// 같은 GE 타입 장판 중복 적용 방지 (D&D 규칙: 동일 주문 효과 미중첩)
	UPBEnvironmentSubsystem* EnvSys = nullptr;
	int32 CurrentRound = 0;

	if (UGameInstance* GI = GetGameInstance())
	{
		EnvSys = GI->GetSubsystem<UPBEnvironmentSubsystem>();
	}
	if (UWorld* World = GetWorld())
	{
		if (const UPBCombatManagerSubsystem* CombatMgr = World->GetSubsystem<UPBCombatManagerSubsystem>())
		{
			CurrentRound = CombatMgr->GetCurrentRound();
		}
	}

	const UGameplayEffect* GEDef = EffectSpecHandle.IsValid()
		? EffectSpecHandle.Data->Def
		: nullptr;

	for (AActor* Actor : OverlappingActors)
	{
		// 같은 라운드에 같은 GE 타입이 이미 적용된 대상이면 스킵
		if (EnvSys && GEDef && !EnvSys->TryMarkAreaEffectApplied(Actor, GEDef, CurrentRound))
		{
			continue;
		}
		ApplyEffectToActor(Actor);
	}
}

void APBDamageArea::AddGroundSnapComponent(USceneComponent* Component)
{
	if (IsValid(Component))
	{
		GroundSnapComponents.AddUnique(Component);
	}
}

float APBDamageArea::GetAreaRadius() const
{
	return CollisionComponent->GetScaledSphereRadius();
}

// GE Modifier 중 IncomingDamage 어트리뷰트 합산
static float SumIncomingDamageModifiers(const UGameplayEffect* GEDef, const FGameplayEffectSpec& Spec)
{
	float Damage = 0.f;
	const FGameplayAttribute IncomingDamageAttr = UPBCharacterAttributeSet::GetIncomingDamageAttribute();

	for (int32 i = 0; i < GEDef->Modifiers.Num(); ++i)
	{
		if (GEDef->Modifiers[i].Attribute != IncomingDamageAttr)
		{
			continue;
		}

		float Magnitude = 0.f;
		GEDef->Modifiers[i].ModifierMagnitude.AttemptCalculateMagnitude(Spec, Magnitude);
		Damage += Magnitude;
	}

	return Damage;
}

void APBDamageArea::CalculateExpectedDamage()
{
	ExpectedDamage = 0.f;

	if (!EffectSpecHandle.IsValid())
	{
		return;
	}

	const FGameplayEffectSpec& Spec = *EffectSpecHandle.Data.Get();
	const UGameplayEffect* GEDef = Spec.Def;
	if (!IsValid(GEDef))
	{
		return;
	}

	float TotalDamage = 0.f;

	// 메인 GE의 IncomingDamage Modifier 합산
	TotalDamage += SumIncomingDamageModifiers(GEDef, Spec);

	// TurnEffectComponent의 GE들의 IncomingDamage Modifier 합산
	if (const UPBTurnEffectComponent* TurnEffectComp = GEDef->FindComponent<UPBTurnEffectComponent>())
	{
		for (const TSubclassOf<UGameplayEffect>& TurnEffectClass : TurnEffectComp->GetTurnEffects())
		{
			const UGameplayEffect* TurnEffectCDO = TurnEffectClass.GetDefaultObject();
			if (!IsValid(TurnEffectCDO))
			{
				continue;
			}

			// TurnEffect는 InitializeFromLinkedSpec으로 원본 Spec의 SetByCaller를 상속하므로 동일 Spec 사용
			TotalDamage += SumIncomingDamageModifiers(TurnEffectCDO, Spec);
		}
	}

	// 스택 곱연산
	const int32 StackCount = FMath::Max(1, Spec.StackCount);
	ExpectedDamage = TotalDamage * StackCount;
}

void APBDamageArea::SnapComponentsToGround()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const float TraceDistance = 1000.f;

	for (USceneComponent* Component : GroundSnapComponents)
	{
		if (!IsValid(Component))
		{
			continue;
		}

		const FVector Origin = Component->GetComponentLocation();
		const FVector TraceStart = FVector(Origin.X, Origin.Y, Origin.Z + TraceDistance);
		const FVector TraceEnd = FVector(Origin.X, Origin.Y, Origin.Z - TraceDistance);

		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
			PBTraceChannel::Ground, QueryParams))
		{
			Component->SetWorldLocation(HitResult.ImpactPoint);
		}
	}
}
