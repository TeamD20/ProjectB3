#include "PBAIMockCharacter.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "Components/StateTreeComponent.h"
#include "GameplayEffect.h"
#include "PBGE_RestoreTurnResources.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "StateTreeEvents.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"

/*~ 생성자 ~*/

APBAIMockCharacter::APBAIMockCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

/*~ AActor Interface ~*/

void APBAIMockCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 샌드박스 환경 구동 시작을 알리는 로그 출력
	UE_LOG(
		LogTemp, Display,
		TEXT("=== PB Mock Character [%s] Spawned and Ready for AI Testing ==="),
		*GetName());

	if (IsValid(AbilitySystemComponent))
	{
		// 1. ASC 초기화
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 2. 공격 스킬 부여
		if (IsValid(DefaultAttackAbility))
		{
			AbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(DefaultAttackAbility, 1, INDEX_NONE,
				                     this));
		}

		// 3.GameplayEffect를 생성하여 속성 초기화
		UGameplayEffect* InitGE = NewObject<UGameplayEffect>(
			GetTransientPackage(), FName(TEXT("DynamicInitHealthGE")));
		InitGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		// Action (AP) 초기화: 1.0f
		FGameplayModifierInfo ActionModInfo;
		ActionModInfo.Attribute =
			UPBTurnResourceAttributeSet::GetActionAttribute();
		ActionModInfo.ModifierOp = EGameplayModOp::Override;
		FScalableFloat ActionScalableFloat;
		ActionScalableFloat.SetValue(1.0f);
		ActionModInfo.ModifierMagnitude =
			FGameplayEffectModifierMagnitude(ActionScalableFloat);
		InitGE->Modifiers.Add(ActionModInfo);

		// BonusAction 초기화: 1.0f
		FGameplayModifierInfo BonusActionModInfo;
		BonusActionModInfo.Attribute =
			UPBTurnResourceAttributeSet::GetBonusActionAttribute();
		BonusActionModInfo.ModifierOp = EGameplayModOp::Override;
		FScalableFloat BonusActionScalableFloat;
		BonusActionScalableFloat.SetValue(1.0f);
		BonusActionModInfo.ModifierMagnitude =
			FGameplayEffectModifierMagnitude(BonusActionScalableFloat);
		InitGE->Modifiers.Add(BonusActionModInfo);

		// Reaction 초기화: 1.0f
		FGameplayModifierInfo ReactionModInfo;
		ReactionModInfo.Attribute =
			UPBTurnResourceAttributeSet::GetReactionAttribute();
		ReactionModInfo.ModifierOp = EGameplayModOp::Override;
		FScalableFloat ReactionScalableFloat;
		ReactionScalableFloat.SetValue(1.0f);
		ReactionModInfo.ModifierMagnitude =
			FGameplayEffectModifierMagnitude(ReactionScalableFloat);
		InitGE->Modifiers.Add(ReactionModInfo);

		// Movement 초기화: 900.0f (9m)
		FGameplayModifierInfo MovementModInfo;
		MovementModInfo.Attribute =
			UPBTurnResourceAttributeSet::GetMovementAttribute();
		MovementModInfo.ModifierOp = EGameplayModOp::Override;
		FScalableFloat MovementScalableFloat;
		MovementScalableFloat.SetValue(900.0f);
		MovementModInfo.ModifierMagnitude =
			FGameplayEffectModifierMagnitude(MovementScalableFloat);
		InitGE->Modifiers.Add(MovementModInfo);

		// 속성 초기치 부여
		AbilitySystemComponent->ApplyGameplayEffectToSelf(
			InitGE, 1.0f, AbilitySystemComponent->MakeEffectContext());

		// 턴 자원 초기화 전용 GE 발동 (가득 채우기)
		UGameplayEffect* RestoreGE = NewObject<UPBGE_RestoreTurnResources>(
			GetTransientPackage(), UPBGE_RestoreTurnResources::StaticClass());

		if (RestoreGE)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(
				RestoreGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
		}
	}
}

/*~ IPBCombatParticipant Interface ~*/

int32 APBAIMockCharacter::GetInitiativeModifier() const
{
	// 샌드박스: 임시로 0 반환
	return 0;
}

bool APBAIMockCharacter::HasInitiativeAdvantage() const { return false; }

void APBAIMockCharacter::OnCombatBegin()
{
}

void APBAIMockCharacter::OnCombatEnd()
{
}

void APBAIMockCharacter::OnRoundBegin()
{
}

void APBAIMockCharacter::OnTurnBegin()
{
	UE_LOG(LogTemp, Display,
	       TEXT("=== %s: OnTurnBegin 호출, StateTree 이벤트 전송 ==="),
	       *GetName());

	// 턴 시작 시 StateTree에 Event.Combat.TurnStarted 이벤트 전송
	if (AController* CharacterController = GetController())
	{
		if (UStateTreeComponent* StateTreeComp =
			CharacterController->FindComponentByClass<UStateTreeComponent>())
		{
			FStateTreeEvent Event;
			Event.Tag =
				FGameplayTag::RequestGameplayTag(
					TEXT("Event.Combat.TurnStarted"));
			StateTreeComp->SendStateTreeEvent(Event);
		}
	}
}

void APBAIMockCharacter::OnTurnEnd()
{
}

bool APBAIMockCharacter::CanReact() const { return false; }

void APBAIMockCharacter::OnReactionOpportunity(
	const FPBReactionContext& Context)
{
	// 추후 구현 (반응 시스템 고도화 시 연동)
}

void APBAIMockCharacter::OnActionInterrupted()
{
	UE_LOG(LogTemp, Display,
	       TEXT("=== %s: OnActionInterrupted 호출, StateTree 이벤트 전송 ==="),
	       *GetName());

	// 행동 인터럽트 시 StateTree에 Event.Combat.ActionInterrupted 이벤트 전송
	if (AController* CharacterController = GetController())
	{
		if (UStateTreeComponent* StateTreeComp =
			CharacterController->FindComponentByClass<UStateTreeComponent>())
		{
			FStateTreeEvent Event;
			Event.Tag = FGameplayTag::RequestGameplayTag(
				TEXT("Event.Combat.ActionInterrupted"));
			StateTreeComp->SendStateTreeEvent(Event);
		}
	}
}

bool APBAIMockCharacter::IsIncapacitated() const
{
	// 샌드박스: 무력화 상태 아님 처리
	return false;
}

FGameplayTag APBAIMockCharacter::GetFactionTag() const
{
	// 부모(PBCharacterBase)의 CombatIdentity.FactionTag 사용.
	// BP 인스턴스별로 다른 진영 태그를 설정하면 공유 턴 그룹이 생성되지 않음.
	// CombatIdentity가 비어있으면 고유 Enemy 태그로 폴백.
	if (CombatIdentity.FactionTag.IsValid())
	{
		return CombatIdentity.FactionTag;
	}

	// 폴백: 기존 동작 유지
	return FGameplayTag::RequestGameplayTag(TEXT("Combat.Faction.Enemy"));
}

float APBAIMockCharacter::GetBaseMovementSpeed() const
{
	// 샌드박스: 이동속도 9m 반환 (초기 이동력 900과 일치)
	return 900.0f;
}

FText APBAIMockCharacter::GetCombatDisplayName() const
{
	return FText::FromString(GetName());
}

TSoftObjectPtr<UTexture2D> APBAIMockCharacter::GetCombatPortrait() const
{
	return nullptr;
}
