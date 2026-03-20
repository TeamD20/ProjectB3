// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBGameplayPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "PBPartyFollowSubsystem.h"
#include "ProjectB3/Characters/PBPlayerCharacter.h"
#include "ProjectB3/Player/PBPartyAIController.h"

void APBGameplayPlayerState::AddGold(int32 Amount)
{
	if (Amount > 0)
	{
		TotalGold += Amount;
		OnGoldChanged.Broadcast(TotalGold);
	}
}

bool APBGameplayPlayerState::SpendGold(int32 Amount)
{
	if (Amount <= 0 || TotalGold < Amount)
	{
		return false;
	}

	TotalGold -= Amount;
	OnGoldChanged.Broadcast(TotalGold);
	return true;
}

void APBGameplayPlayerState::AddPartyMember(AActor* PartyMember)
{
	if (!IsValid(PartyMember))
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& ExistingMember : PartyMembers)
	{
		if (ExistingMember.Get() == PartyMember)
		{
			return;
		}
	}

	PartyMembers.Add(PartyMember);
	OnPartyMembersChanged.Broadcast();

	if (!IsValid(SelectedPartyMember.Get()))
	{
		SelectPartyMember(PartyMember);
	}
	else
	{
		// 대기 중인 파티원은 자기 자신의 고유 PartyAIController가 조종해야 함
		if (APBPlayerCharacter* PlayerChar = Cast<APBPlayerCharacter>(PartyMember))
		{
			if (IsValid(PlayerChar->PartyAIController) && PlayerChar->GetController() != PlayerChar->PartyAIController)
			{
				PlayerChar->PartyAIController->Possess(PlayerChar);
			}
		}
	}
}

void APBGameplayPlayerState::RemovePartyMember(AActor* PartyMember)
{
	if (!IsValid(PartyMember))
	{
		return;
	}

	PartyMembers.RemoveAll(
		[PartyMember](const TWeakObjectPtr<AActor>& ExistingMember)
		{
			return !IsValid(ExistingMember.Get()) || ExistingMember.Get() == PartyMember;
		});

	OnPartyMembersChanged.Broadcast();

	if (SelectedPartyMember.Get() == PartyMember)
	{
		SelectedPartyMember.Reset();
		OnSelectedPartyMemberChanged.Broadcast(nullptr);
	}
}

void APBGameplayPlayerState::SelectPartyMember(AActor* PartyMember)
{
	if (PartyMember != nullptr && !IsValid(PartyMember))
	{
		return;
	}

	if (SelectedPartyMember.Get() == PartyMember)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	APBPlayerCharacter* OldPlayerCharacter = PC ? PC->GetPawn<APBPlayerCharacter>() : nullptr;

	SelectedPartyMember = PartyMember;
	OnSelectedPartyMemberChanged.Broadcast(SelectedPartyMember.Get());

	if (PC)
	{
		if (APBPlayerCharacter* NewCharacter = Cast<APBPlayerCharacter>(PartyMember))
		{
			// Possess 전 현재 ViewTarget을 보존한다.
			AActor* OldViewTarget = PC->GetViewTarget();

			// 이 과정에서 기존 AI 컨트롤러는 자동으로 Unpossess 됩니다.
			PC->Possess(NewCharacter);

			// Possess 후 카메라가 스냅되므로, 이전 ViewTarget으로 즉시 복원 후 블렌딩한다.
			if (IsValid(OldViewTarget) && PartyMemberCameraBlendTime > 0.0f)
			{
				PC->SetViewTarget(OldViewTarget);
				PC->SetViewTargetWithBlend(NewCharacter, PartyMemberCameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
			}
		}

		// 예전 리더(OldPawn)를 소유 캐싱된 파티 AI로 빙의 전환
		if (IsValid(OldPlayerCharacter) && OldPlayerCharacter != PC->GetPawn())
		{
			if (IsValid(OldPlayerCharacter->PartyAIController))
			{
				OldPlayerCharacter->PartyAIController->Possess(OldPlayerCharacter);
			}
		}
		
		if (UPBPartyFollowSubsystem* PartyFollowSubsystem = GetWorld()->GetSubsystem<UPBPartyFollowSubsystem>())
		{
			PartyFollowSubsystem->RebuildFollowerCache();
		}
	}
}

TArray<AActor*> APBGameplayPlayerState::GetPartyMembers() const
{
	TArray<AActor*> Result;
	Result.Reserve(PartyMembers.Num());

	for (const TWeakObjectPtr<AActor>& PartyMember : PartyMembers)
	{
		if (IsValid(PartyMember.Get()))
		{
			Result.Add(PartyMember.Get());
		}
	}

	return Result;
}

AActor* APBGameplayPlayerState::GetSelectedPartyMember() const
{
	return SelectedPartyMember.Get();
}

bool APBGameplayPlayerState::RequestAbilityActivation(FGameplayAbilitySpecHandle AbilityHandle) const
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	AActor* SelectedMember = SelectedPartyMember.Get();
	if (!IsValid(SelectedMember))
	{
		return false;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(SelectedMember);
	if (AbilitySystemInterface == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	return AbilitySystemComponent->TryActivateAbility(AbilityHandle);
}
