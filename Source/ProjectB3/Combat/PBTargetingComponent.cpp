// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTargetingComponent.h"
#include "IPBCombatTarget.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

void UPBTargetingComponent::EnterTargetingMode(const FPBTargetingRequest& Request)
{
	CurrentRequest = Request;
	HoverPreviewData = FPBAbilityTargetData();
	SelectedTargets.Empty();
	bIsHoverValid = false;
	bIsTargetingActive = true;
	
	if (bShowingTelegraph && CurrentRequest.Mode != EPBTargetingMode::AoE)
	{
		HideTelegraph();
	}
}

void UPBTargetingComponent::ExitTargetingMode()
{
	bIsTargetingActive = false;
	CurrentRequest = FPBTargetingRequest();
	HoverPreviewData = FPBAbilityTargetData();
	SelectedTargets.Empty();
	bIsHoverValid = false;
	HideTelegraph();
}

void UPBTargetingComponent::UpdateTargetingFromHit(const FHitResult& HitResult)
{
	// 호버 프리뷰 전용. 타겟 추가는 AddTargetSelection(MultiTarget), 확정은 ConfirmTarget에서 수행.
	if (!bIsTargetingActive)
	{
		return;
	}

	// 히트 결과로부터 모드에 맞는 Candidate 구성
	FPBAbilityTargetData NewCandidate;
	NewCandidate.TargetingMode = CurrentRequest.Mode;

	switch (CurrentRequest.Mode)
	{
	case EPBTargetingMode::SingleTarget:
	case EPBTargetingMode::MultiTarget:
		if (IsValid(HitResult.GetActor()) && HitResult.GetActor()->Implements<UPBCombatTarget>())
		{
			NewCandidate.TargetActors.Add(HitResult.GetActor());
		}
		else if (CurrentRequest.bAllowGroundTarget)
		{
			// 액터 미히트 시 지면 위치로 폴백
			NewCandidate.TargetLocations.Add(HitResult.ImpactPoint);
		}
		break;

	case EPBTargetingMode::Location:
	case EPBTargetingMode::AoE:
		NewCandidate.TargetLocations.Add(HitResult.ImpactPoint);
		NewCandidate.AoERadius = CurrentRequest.AoERadius;
		ShowTelegraph(HitResult.ImpactPoint);
		break;

	default:
		break;
	}

	// 사거리 검증을 요청한 어빌리티에 위임
	bool bInRange = false;
	UPBGameplayAbility_Targeted* Ability = CurrentRequest.RequestingAbility.Get();
	if (IsValid(Ability))
	{
		bInRange = Ability->IsTargetInRange(CurrentRequest.OriginLocation, NewCandidate);
	}
	else
	{
		// 어빌리티 참조가 없으면 범위 제한 없이 구조체 유효성만 검사
		bInRange = NewCandidate.IsValid();
	}

	HoverPreviewData = NewCandidate;
	bIsHoverValid = bInRange && NewCandidate.IsValid();

	OnTargetPreviewUpdated.Broadcast(HoverPreviewData, bInRange);
}

void UPBTargetingComponent::AddTargetSelection()
{
	// MultiTarget 전용. 호버 중인 액터를 후보에 추가 (같은 타겟 중복 지정 허용).
	// MaxTargetCount 달성 시 ConfirmTarget 자동 호출 (0이면 무제한).
	if (!bIsTargetingActive || CurrentRequest.Mode != EPBTargetingMode::MultiTarget)
	{
		return;
	}

	// 호버 중인 액터를 후보에 추가 (같은 타겟 중복 지정 허용)
	AActor* Target = HoverPreviewData.GetSingleTargetActor();
	if (!IsValid(Target) || !bIsHoverValid)
	{
		return;
	}

	SelectedTargets.Add(Target);

	OnSelectionChanged.Broadcast(MakeMultiTargetData());

	// MaxTargetCount 달성 시 자동 확정 (0이면 무제한)
	const int32 MaxCount = CurrentRequest.MaxTargetCount;
	if (MaxCount > 0 && SelectedTargets.Num() >= MaxCount)
	{
		ConfirmTarget();
	}
}

void UPBTargetingComponent::RemoveLastTarget()
{
	if (!bIsTargetingActive || CurrentRequest.Mode != EPBTargetingMode::MultiTarget)
	{
		return;
	}
	if (SelectedTargets.Num() == 0)
	{
		return;
	}

	SelectedTargets.RemoveAt(SelectedTargets.Num() - 1);

	OnSelectionChanged.Broadcast(MakeMultiTargetData());
}

void UPBTargetingComponent::ConfirmTarget()
{
	// MultiTarget: 누적된 후보 목록으로 확정.
	// 그 외: 현재 호버 프리뷰로 확정. 유효하지 않으면 CancelTargeting.
	if (!bIsTargetingActive)
	{
		return;
	}

	FPBAbilityTargetData ConfirmedData;

	if (CurrentRequest.Mode == EPBTargetingMode::MultiTarget)
	{
		// MultiTarget: 누적된 선택 목록으로 확정
		if (SelectedTargets.Num() == 0)
		{
			CancelTargeting();
			return;
		}
		ConfirmedData = MakeMultiTargetData();
	}
	else
	{
		// 그 외 모드: 현재 Candidate로 확정
		if (!bIsHoverValid)
		{
			CancelTargeting();
			return;
		}
		ConfirmedData = HoverPreviewData;
	}

	// ExitTargetingMode 먼저 — 콜백 내 재진입 방지
	ExitTargetingMode();
	OnTargetConfirmed.Broadcast(ConfirmedData);
}

void UPBTargetingComponent::CancelTargeting()
{
	if (!bIsTargetingActive)
	{
		return;
	}

	ExitTargetingMode();
	OnTargetCancelled.Broadcast();
}

void UPBTargetingComponent::EnsureTelegraphComponent()
{
	// 이미 생성되어 있거나 에셋이 없으면 스킵
	if (IsValid(TelegraphNiagaraComp) || !IsValid(TelegraphNiagaraSystem))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// PC에 부착하면 렌더링이 되지 않으므로 전용 액터를 월드에 소환
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TelegraphActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!IsValid(TelegraphActor))
	{
		return;
	}

	TelegraphNiagaraComp = NewObject<UNiagaraComponent>(TelegraphActor);
	TelegraphNiagaraComp->SetAsset(TelegraphNiagaraSystem);
	TelegraphNiagaraComp->bAutoActivate = false;
	TelegraphActor->SetRootComponent(TelegraphNiagaraComp);
	TelegraphNiagaraComp->RegisterComponent();
	TelegraphNiagaraComp->Deactivate();
}

void UPBTargetingComponent::ShowTelegraph(const FVector& Location)
{
	EnsureTelegraphComponent();
	if (!IsValid(TelegraphNiagaraComp))
	{
		return;
	}

	TelegraphNiagaraComp->SetWorldLocation(Location);
	TelegraphNiagaraComp->SetVariableFloat(TEXT("User.Radius"), CurrentRequest.AoERadius);

	if (!TelegraphNiagaraComp->IsActive())
	{
		TelegraphNiagaraComp->Activate(true);
	}
	
	bShowingTelegraph = true;
}

void UPBTargetingComponent::HideTelegraph()
{
	if (IsValid(TelegraphNiagaraComp) && TelegraphNiagaraComp->IsActive())
	{
		TelegraphNiagaraComp->Deactivate();
	}
	
	bShowingTelegraph = false;
}

FPBAbilityTargetData UPBTargetingComponent::MakeMultiTargetData() const
{
	FPBAbilityTargetData Data;
	Data.TargetingMode = EPBTargetingMode::MultiTarget;
	Data.TargetActors = SelectedTargets;
	return Data;
}
