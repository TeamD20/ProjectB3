#include "PBAIMockCharacter.h"
#include "Components/StateTreeComponent.h"
#include "StateTreeEvents.h"

/*~ 생성자 ~*/

APBAIMockCharacter::APBAIMockCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

/*~ AActor Interface ~*/

void APBAIMockCharacter::BeginPlay()
{
	// 부모가 전체 초기화 파이프라인 실행:
	// InitAbilityActorInfo → InitTags(ClassTag) → GrantInitialAbilities(Stats+Common+Class) → GrantDefaultItems
	Super::BeginPlay();

	UE_LOG(
		LogTemp, Display,
		TEXT("=== PB Mock Character [%s] Spawned and Ready for AI Testing ==="),
		*GetName());
}

/*~ IPBCombatParticipant Interface ~*/

void APBAIMockCharacter::OnTurnBegin()
{
	// 부모 호출: Action/BonusAction/Movement 자원 리셋
	Super::OnTurnBegin();

	UE_LOG(LogTemp, Display,
	       TEXT("=== %s: OnTurnBegin 호출 (자원 리셋 완료) ==="),
	       *GetName());
}

void APBAIMockCharacter::OnTurnActivated()
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

float APBAIMockCharacter::GetBaseMovementSpeed() const
{
	// 이동속도 9m (900cm, D&D 5e 30ft 기준)
	return 900.0f;
}
