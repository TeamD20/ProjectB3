// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTargetingComponent.h"

#include "AbilitySystemComponent.h"
#include "IPBCombatParticipant.h"
#include "IPBCombatTarget.h"
#include "ProjectB3/ProjectB3.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "ProjectB3/Utils/PBGameplayStatics.h"
#include "Components/MeshComponent.h"
#include "Components/SplineMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Projectiles/PBProjectileUtils.h"

APawn* UPBTargetingComponent::GetPawn() const
{
	if (AController* Controller = Cast<AController>(GetOwner()))
	{
		return Controller->GetPawn();
	}
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		return Pawn;
	}
	return nullptr;
}

bool UPBTargetingComponent::IsHostileTarget(AActor* InTarget) const
{
	const IPBCombatParticipant* A = Cast<IPBCombatParticipant>(GetPawn());
	const IPBCombatParticipant* B = Cast<IPBCombatParticipant>(InTarget);
	
	if (A == nullptr || B == nullptr)
	{
		return false;
	}
	
	FGameplayTag FactionA = A->GetFactionTag();
	FGameplayTag FactionB = B->GetFactionTag();
	
	if (FactionA.MatchesTag(FactionB) || FactionB.MatchesTag(FactionA))
	{
		return false;
	}
	
	return true;
}

void UPBTargetingComponent::EnterTargetingMode(const FPBTargetingRequest& Request)
{
	CurrentRequest = Request;
	HoverPreviewData = FPBAbilityTargetData();
	SelectedTargets.Empty();
	bIsHoverValid = false;
	bIsTargetingActive = true;

	if (bShowingAoETelegraph && CurrentRequest.Mode != EPBTargetingMode::AoE)
	{
		HideAoETelegraph();
	}

	if (!FMath::IsNearlyZero(CurrentRequest.Range))
	{
		ShowRangeTelegraph();
	}
	else
	{
		HideRangeTelegraph();
	}
}

void UPBTargetingComponent::ExitTargetingMode()
{
	ClearTargetHighlight();
	bIsTargetingActive = false;
	CurrentRequest = FPBTargetingRequest();
	HoverPreviewData = FPBAbilityTargetData();
	SelectedTargets.Empty();
	bIsHoverValid = false;
	HideAoETelegraph();
	HideRangeTelegraph();
	HideProjectilePath();
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
	{
		AActor* HitActor = HitResult.GetActor();
		bool bIsValidActor = false;
		
		if (IsValid(HitActor) && HitActor->Implements<UPBCombatTarget>())
		{
			 if (!CurrentRequest.bCanTargetSelf)
			 {
				bIsValidActor = CurrentRequest.RequestingAbility.IsValid() ? !CurrentRequest.RequestingAbility->IsTargetSelf(HitActor) : false;
			 }
			 else
			 {
				bIsValidActor = true;
			 }
		}
		
		if (bIsValidActor)
		{
			NewCandidate.TargetActors.Add(HitActor);

			// 호버 타겟이 바뀐 경우에만 하이라이트 갱신
			if (HighlightedTargetActor.Get() != HitActor)
			{
				ApplyTargetHighlight(HitActor);
			}
		}
		else
		{
			ClearTargetHighlight();
			if (CurrentRequest.bAllowGroundTarget)
			{
				// 액터 미히트 시 지면 위치로 폴백
				NewCandidate.TargetLocations.Add(HitResult.ImpactPoint);
			}
		}
		break;
	}

	case EPBTargetingMode::Location:
	case EPBTargetingMode::AoE:
		NewCandidate.TargetLocations.Add(HitResult.ImpactPoint);
		NewCandidate.AoERadius = CurrentRequest.AoERadius;
		ShowAoETelegraph(HitResult.ImpactPoint);
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

	// 투사체 경로 프리뷰 갱신
	if (CurrentRequest.ProjectileContext.bShowPath)
	{
		// 발사 지점을 매 프레임 갱신 (폰 회전에 따라 무기 소켓 위치 변동)
		if (IsValid(Ability))
		{
			const FVector DynamicLaunch = Ability->GetProjectileLaunchLocation();
			if (!DynamicLaunch.IsZero())
			{
				CurrentRequest.ProjectileContext.LaunchLocation = DynamicLaunch;
			}
		}

		// 타겟 위치 및 타겟 액터 결정
		FVector PathTarget = FVector::ZeroVector;
		AActor* CandidateActor = NewCandidate.GetSingleTargetActor();

		if (IsValid(CandidateActor))
		{
			PathTarget = CandidateActor->GetActorLocation();
		}
		else if (NewCandidate.TargetLocations.Num() > 0)
		{
			PathTarget = NewCandidate.GetSingleTargetLocation();
		}
		else
		{
			PathTarget = HitResult.ImpactPoint;
		}

		ShowProjectilePath(PathTarget, CandidateActor);
	}

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
	
	if (SelectedTargets.Num() == 0)
	{
		CancelTargeting();
	}
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
	
	// 어빌리티 취소
	if (UPBGameplayAbility_Targeted* ActivatingAbility = CurrentRequest.RequestingAbility.Get())
	{
		if (UAbilitySystemComponent* ASC = ActivatingAbility->GetAbilitySystemComponentFromActorInfo())
		{
			ASC->CancelAbilityHandle(ActivatingAbility->GetCurrentAbilitySpecHandle());
		}
	}

	ExitTargetingMode();
	OnTargetCancelled.Broadcast();
}

void UPBTargetingComponent::EnsureAoETelegraphComponent()
{
	// 이미 생성되어 있거나 에셋이 없으면 스킵
	if (IsValid(AoETelegraphNiagaraComp) || !IsValid(AoETelegraphNiagaraSystem))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// PC에 부착하면 렌더링이 되지 않으므로 전용 액터를 월드에 소환
	if (!IsValid(TelegraphActor))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TelegraphActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		                                           SpawnParams);
	}
	if (!IsValid(TelegraphActor))
	{
		return;
	}

	AoETelegraphNiagaraComp = NewObject<UNiagaraComponent>(TelegraphActor);
	AoETelegraphNiagaraComp->SetAsset(AoETelegraphNiagaraSystem);
	AoETelegraphNiagaraComp->bAutoActivate = false;
	TelegraphActor->SetRootComponent(AoETelegraphNiagaraComp);
	AoETelegraphNiagaraComp->RegisterComponent();
	AoETelegraphNiagaraComp->DeactivateImmediate();
}

void UPBTargetingComponent::EnsureRangeTelegraphComponent()
{
	// 이미 생성되어 있거나 에셋이 없으면 스킵
	if (IsValid(RangeTelegraphNiagaraComp) || !IsValid(RangeTelegraphNiagaraSystem))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// PC에 부착하면 렌더링이 되지 않으므로 전용 액터를 월드에 소환
	if (!IsValid(TelegraphActor))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TelegraphActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
		                                           SpawnParams);
	}
	if (!IsValid(TelegraphActor))
	{
		return;
	}

	RangeTelegraphNiagaraComp = NewObject<UNiagaraComponent>(TelegraphActor);
	RangeTelegraphNiagaraComp->SetAsset(RangeTelegraphNiagaraSystem);
	RangeTelegraphNiagaraComp->bAutoActivate = false;
	TelegraphActor->SetRootComponent(RangeTelegraphNiagaraComp);
	RangeTelegraphNiagaraComp->RegisterComponent();
	RangeTelegraphNiagaraComp->DeactivateImmediate();
}

void UPBTargetingComponent::ShowAoETelegraph(const FVector& Location)
{
	EnsureAoETelegraphComponent();
	if (!IsValid(AoETelegraphNiagaraComp))
	{
		return;
	}

	AoETelegraphNiagaraComp->SetWorldLocation(Location + FVector(0.0f, 0.0f, 2.0f)); // 지면에서 살짝 띄움
	AoETelegraphNiagaraComp->SetVariableFloat(TEXT("User.Radius"), CurrentRequest.AoERadius * 2.0f);

	if (!AoETelegraphNiagaraComp->IsActive())
	{
		AoETelegraphNiagaraComp->Activate(true);
	}

	bShowingAoETelegraph = true;
}

void UPBTargetingComponent::HideAoETelegraph()
{
	if (IsValid(AoETelegraphNiagaraComp) && AoETelegraphNiagaraComp->IsActive())
	{
		AoETelegraphNiagaraComp->DeactivateImmediate();
	}

	bShowingAoETelegraph = false;
}

void UPBTargetingComponent::ShowRangeTelegraph()
{
	EnsureRangeTelegraphComponent();
	if (!IsValid(RangeTelegraphNiagaraComp))
	{
		return;
	}

	FVector TelegraphLocation = CurrentRequest.OriginLocation;
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		const FVector TraceStart = CurrentRequest.OriginLocation + FVector(0.0f, 0.0f, 100.0f);
		const FVector TraceEnd = CurrentRequest.OriginLocation - FVector(0.0f, 0.0f, 1000.0f);

		FCollisionQueryParams QueryParams;
		if (IsValid(TelegraphActor))
		{
			QueryParams.AddIgnoredActor(TelegraphActor);
		}

		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, PBTraceChannel::Ground, QueryParams))
		{
			TelegraphLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, 2.0f); // 지면에서 살짝 띄움
		}
	}

	RangeTelegraphNiagaraComp->SetWorldLocation(TelegraphLocation);
	RangeTelegraphNiagaraComp->SetVariableFloat(TEXT("User.Radius"), CurrentRequest.Range * 2.f);

	if (!RangeTelegraphNiagaraComp->IsActive())
	{
		RangeTelegraphNiagaraComp->Activate(true);
	}
}

void UPBTargetingComponent::HideRangeTelegraph()
{
	if (IsValid(RangeTelegraphNiagaraComp) && RangeTelegraphNiagaraComp->IsActive())
	{
		RangeTelegraphNiagaraComp->DeactivateImmediate();
	}
}

void UPBTargetingComponent::ApplyTargetHighlight(AActor* TargetActor)
{
	ClearTargetHighlight();

	if (!IsValid(TargetActor))
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	UPBGameplayStatics::GetAllMeshComponents(TargetActor, Meshes);

	SavedTargetCustomDepthStates.Reset();
	for (UMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			SavedTargetCustomDepthStates.Add(TObjectPtr<UMeshComponent>(Mesh), Mesh->bRenderCustomDepth);
			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue(IsHostileTarget(TargetActor) ? PBStencilValues::TARGET_HOSTILE : PBStencilValues::TARGET_FRIENDLY);
		}
	}

	HighlightedTargetActor = TargetActor;
}

void UPBTargetingComponent::ClearTargetHighlight()
{
	if (!HighlightedTargetActor.IsValid())
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	UPBGameplayStatics::GetAllMeshComponents(HighlightedTargetActor.Get(), Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		Mesh->SetCustomDepthStencilValue(PBStencilValues::DEFAULT);

		const bool* bWasEnabled = SavedTargetCustomDepthStates.Find(TObjectPtr<UMeshComponent>(Mesh));
		Mesh->SetRenderCustomDepth(bWasEnabled ? *bWasEnabled : false);
	}

	SavedTargetCustomDepthStates.Reset();
	HighlightedTargetActor.Reset();
}

FPBAbilityTargetData UPBTargetingComponent::MakeMultiTargetData() const
{
	FPBAbilityTargetData Data;
	Data.TargetingMode = EPBTargetingMode::MultiTarget;
	Data.TargetActors = SelectedTargets;
	return Data;
}

// 투사체 경로 프리뷰 

void UPBTargetingComponent::EnsureProjectilePathPool()
{
	if (!IsValid(TelegraphActor))
	{
		UWorld* World = GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TelegraphActor = World->SpawnActor<AActor>(
			AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
}

USplineMeshComponent* UPBTargetingComponent::GetOrCreatePathSegment(int32 Index)
{
	if (Index < ProjectilePathPool.Num() && IsValid(ProjectilePathPool[Index]))
	{
		return ProjectilePathPool[Index];
	}

	EnsureProjectilePathPool();
	if (!IsValid(TelegraphActor))
	{
		return nullptr;
	}

	USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(TelegraphActor);
	if (IsValid(PathSegmentMesh))
	{
		Segment->SetStaticMesh(PathSegmentMesh);
	}
	Segment->SetMobility(EComponentMobility::Movable);
	Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Segment->SetVisibility(false);
	Segment->RegisterComponent();

	if (Index < ProjectilePathPool.Num())
	{
		ProjectilePathPool[Index] = Segment;
	}
	else
	{
		ProjectilePathPool.Add(Segment);
	}

	return Segment;
}

bool UPBTargetingComponent::CheckProjectileVisibility(const FVector& Start, const FVector& End, AActor* IgnoreActor) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return true;
	}

	// 오프셋 적용: 발사 지점·타겟 지점 근처의 오탐 방지
	const FVector Direction = (End - Start).GetSafeNormal();
	const FVector TraceStart = Start + Direction * VisibilityTraceSourceOffset;
	const FVector TraceEnd = End - Direction * VisibilityTraceTargetHorizontalOffset
		+ FVector(0.f, 0.f, VisibilityTraceTargetZOffset);

	FHitResult Hit;
	FCollisionQueryParams Params;

	APawn* Pawn = GetPawn();
	if (IsValid(Pawn))
	{
		Params.AddIgnoredActor(Pawn);

		// 장착 장비 등 Pawn의 자식 액터도 무시
		TArray<AActor*> ChildActors;
		Pawn->GetAttachedActors(ChildActors);
		for (AActor* Child : ChildActors)
		{
			if (IsValid(Child))
			{
				Params.AddIgnoredActor(Child);
			}
		}
	}
	if (IsValid(TelegraphActor))
	{
		Params.AddIgnoredActor(TelegraphActor);
	}
	if (IsValid(IgnoreActor))
	{
		Params.AddIgnoredActor(IgnoreActor);
	}

	const bool bHit = World->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd,
		ECC_Visibility,
		Params);

	return !bHit;
}

void UPBTargetingComponent::ShowProjectilePath(const FVector& TargetLocation, AActor* TargetActor)
{
	const FPBProjectilePathContext& Ctx = CurrentRequest.ProjectileContext;
	if (!Ctx.bShowPath)
	{
		return;
	}

	const FVector& P0 = Ctx.LaunchLocation;
	const FVector& P2 = TargetLocation;
	const FVector P1 = PBProjectileUtils::CalcMidControlPoint(
		P0, P2, Ctx.ArcHeightRatio, Ctx.MinArcHeight, Ctx.MaxArcHeight);

	// 장애물 체크 — 타겟 액터는 무시, 양쪽 끝 오프셋으로 지면/무기 근접 오탐 방지
	bProjectilePathBlocked = !CheckProjectileVisibility(P0, P2, TargetActor);

	// 사거리 기반 OutOfRange 판정용 XY 기준점 및 거리
	const bool bHasRange = !FMath::IsNearlyZero(CurrentRequest.Range);
	const FVector2D OriginXY(CurrentRequest.OriginLocation.X, CurrentRequest.OriginLocation.Y);
	const float RangeSq = CurrentRequest.Range * CurrentRequest.Range;

	// Bezier 곡선 → 스플라인 메시 세그먼트 배치
	const int32 NumSeg = ProjectilePathSegmentCount;
	for (int32 i = 0; i < NumSeg; ++i)
	{
		USplineMeshComponent* Segment = GetOrCreatePathSegment(i);
		if (!IsValid(Segment))
		{
			continue;
		}

		const float t0 = static_cast<float>(i) / NumSeg;
		const float t1 = static_cast<float>(i + 1) / NumSeg;

		const FVector SegStart = PBProjectileUtils::BezierPoint(P0, P1, P2, t0);
		const FVector SegEnd = PBProjectileUtils::BezierPoint(P0, P1, P2, t1);

		// Bezier 탄젠트를 세그먼트 수로 나눠 Hermite 스플라인 탄젠트로 변환
		const FVector StartTangent = PBProjectileUtils::BezierTangent(P0, P1, P2, t0) / NumSeg;
		const FVector EndTangent = PBProjectileUtils::BezierTangent(P0, P1, P2, t1) / NumSeg;

		Segment->SetStartAndEnd(SegStart, StartTangent, SegEnd, EndTangent);

		// 머테리얼 결정: Blocked/OutOfRange(공유) vs Clear
		UMaterialInterface* SegMaterial = nullptr;
		if (bProjectilePathBlocked)
		{
			SegMaterial = BlockedPathMaterial.Get();
		}
		else if (bHasRange)
		{
			// 세그먼트 중점의 XY 거리가 사거리 밖이면 Blocked와 동일 머테리얼
			const FVector SegMid = (SegStart + SegEnd) * 0.5f;
			const float DistSq = FVector2D::DistSquared(OriginXY, FVector2D(SegMid.X, SegMid.Y));
			SegMaterial = (DistSq > RangeSq) ? BlockedPathMaterial.Get() : ClearPathMaterial.Get();
		}
		else
		{
			SegMaterial = ClearPathMaterial.Get();
		}

		if (IsValid(SegMaterial))
		{
			Segment->SetMaterial(0, SegMaterial);
		}
		Segment->SetVisibility(true);
	}

	// 남은 풀 세그먼트 숨기기
	for (int32 i = NumSeg; i < ProjectilePathPool.Num(); ++i)
	{
		if (IsValid(ProjectilePathPool[i]))
		{
			ProjectilePathPool[i]->SetVisibility(false);
		}
	}

	bShowingProjectilePath = true;
}

void UPBTargetingComponent::HideProjectilePath()
{
	if (!bShowingProjectilePath)
	{
		return;
	}

	for (USplineMeshComponent* Segment : ProjectilePathPool)
	{
		if (IsValid(Segment))
		{
			Segment->SetVisibility(false);
		}
	}

	bShowingProjectilePath = false;
	bProjectilePathBlocked = false;
}

void UPBTargetingComponent::NativeCollectPrewarmTargets(FPBPrewarmTargets& InOutTargets)
{
	if (IsValid(AoETelegraphNiagaraSystem))
	{
		InOutTargets.NiagaraAssets.AddUnique(TSoftObjectPtr<UNiagaraSystem>(AoETelegraphNiagaraSystem));
	}
	if (IsValid(RangeTelegraphNiagaraSystem))
	{
		InOutTargets.NiagaraAssets.AddUnique(TSoftObjectPtr<UNiagaraSystem>(RangeTelegraphNiagaraSystem));
	}
}
