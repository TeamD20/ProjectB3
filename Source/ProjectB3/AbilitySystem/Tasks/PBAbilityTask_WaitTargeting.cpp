// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilityTask_WaitTargeting.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "ProjectB3/Combat/PBTargetingComponent.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"

UPBAbilityTask_WaitTargeting* UPBAbilityTask_WaitTargeting::CreateTask(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UPBAbilityTask_WaitTargeting>(OwningAbility);
}

void UPBAbilityTask_WaitTargeting::Activate()
{
	UPBGameplayAbility_Targeted* PBAbility = Cast<UPBGameplayAbility_Targeted>(Ability);
	if (!IsValid(PBAbility))
	{
		EndTask();
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = PBAbility->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return;
	}
	
	APBGameplayPlayerController* PC = nullptr;
	if (APawn* Avatar = Cast<APawn>(ActorInfo->AvatarActor.Get()))
	{
		PC = Avatar->GetController<APBGameplayPlayerController>();
	}

	if (!IsValid(PC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WaitTargeting] APBGameplayPlayerController를 찾을 수 없습니다."));
		EndTask();
		return;
	}

	WeakPC = PC;
	WeakTargetingComp = PC->GetTargetingComponent();

	// 어빌리티로부터 타겟팅 요청 구성
	const FPBTargetingRequest Request = PBAbility->MakeTargetingRequest();

	// 델리게이트 바인딩 후 PC를 통해 타겟팅 모드 진입
	UPBTargetingComponent* TargetingComp = WeakTargetingComp.Get();
	TargetingComp->OnTargetConfirmed.AddUObject(this, &UPBAbilityTask_WaitTargeting::HandleTargetConfirmed);
	TargetingComp->OnTargetCancelled.AddUObject(this, &UPBAbilityTask_WaitTargeting::HandleTargetCancelled);
	PC->EnterTargetingMode(Request);
}

void UPBAbilityTask_WaitTargeting::OnDestroy(bool bInOwnerFinished)
{
	UPBTargetingComponent* TargetingComp = WeakTargetingComp.Get();
	if (IsValid(TargetingComp))
	{
		TargetingComp->OnTargetConfirmed.RemoveAll(this);
		TargetingComp->OnTargetCancelled.RemoveAll(this);
	}

	// 외부 중단(어빌리티 취소 등) 시 PC 모드를 None으로 종료
	APBGameplayPlayerController* PC = WeakPC.Get();
	if (IsValid(PC) && PC->GetControllerMode() == EPBPlayerControllerMode::Targeting)
	{
		PC->ExitCurrentMode();
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UPBAbilityTask_WaitTargeting::HandleTargetConfirmed(const FPBAbilityTargetData& TargetData)
{
	// TargetingComponent가 이미 ExitTargetingMode를 완료한 뒤 호출됨
	OnTargetConfirmed.Broadcast(TargetData);
	EndTask();
}

void UPBAbilityTask_WaitTargeting::HandleTargetCancelled()
{
	OnTargetCancelled.Broadcast();
	EndTask();
}
