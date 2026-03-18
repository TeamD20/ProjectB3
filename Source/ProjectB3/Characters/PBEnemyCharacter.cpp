#include "PBEnemyCharacter.h"
#include "Components/StateTreeComponent.h"
#include "ProjectB3/PBGameplayTags.h"
#include "StateTreeEvents.h"

/*~ 생성자 ~*/

APBEnemyCharacter::APBEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

/*~ AActor Interface ~*/

void APBEnemyCharacter::BeginPlay()
{
	// 부모가 전체 초기화 파이프라인 실행:
	// InitAbilityActorInfo → InitTags(ClassTag) → GrantInitialAbilities(Stats+Common+Class) → GrantDefaultItems
	Super::BeginPlay();

	UE_LOG(
		LogTemp, Display,
		TEXT("=== PBEnemyCharacter [%s] Spawned and Ready ==="),
		*GetName());
}

/*~ APBCharacterBase Interface ~*/

void APBEnemyCharacter::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists)
{
	Super::HandleGameplayTagUpdated(ChangedTag, TagExists);

	// 사망 시 StateTree 즉시 정지 → 진행 중인/대기 중인 AI 행동 차단
	if (ChangedTag == PBGameplayTags::Character_State_Dead && TagExists)
	{
		if (AController* CharacterController = GetController())
		{
			if (UStateTreeComponent* StateTreeComp =
				CharacterController->FindComponentByClass<UStateTreeComponent>())
			{
				StateTreeComp->StopLogic(TEXT("Character Dead"));
			}
		}
	}
}

/*~ IPBCombatParticipant Interface ~*/

void APBEnemyCharacter::OnTurnBegin()
{
	// 부모 호출: Action/BonusAction/Movement 자원 리셋
	Super::OnTurnBegin();

	UE_LOG(LogTemp, Display,
	       TEXT("=== %s: OnTurnBegin 호출 (자원 리셋 완료) ==="),
	       *GetName());
}

void APBEnemyCharacter::OnTurnActivated()
{
	UE_LOG(LogTemp, Display,
	       TEXT("=== %s: OnTurnActivated 호출, StateTree 이벤트 전송 ==="),
	       *GetName());

	// 실제 행동 차례가 되었을 때 StateTree에 이벤트 전송
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

void APBEnemyCharacter::OnActionInterrupted()
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

float APBEnemyCharacter::GetBaseMovementSpeed() const
{
	// 이동속도 9m (900cm, D&D 5e 30ft 기준)
	return 900.0f;
}
