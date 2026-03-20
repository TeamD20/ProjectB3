// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTacticalCameraComponent.h"
#include "PBTacticalCameraActor.h"
#include "Abilities/GameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/PBGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBTacticalCamera, Log, All);

UPBTacticalCameraComponent::UPBTacticalCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

//  UActorComponent 

void UPBTacticalCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	SpawnCameraActor();
	
	if (!EnemyFactionTag.IsValid())
	{
		EnemyFactionTag = PBGameplayTags::Combat_Faction_Enemy;
	}

	UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (IsValid(CombatManager))
	{
		CombatStateChangedHandle = CombatManager->OnCombatStateChanged.AddUObject(
			this, &UPBTacticalCameraComponent::HandleCombatStateChanged);
		ActiveTurnChangedHandle = CombatManager->OnActiveTurnChanged.AddUObject(
			this, &UPBTacticalCameraComponent::HandleActiveTurnChanged);
	}
}

void UPBTacticalCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (IsValid(CombatManager))
	{
		CombatManager->OnCombatStateChanged.Remove(CombatStateChangedHandle);
		CombatManager->OnActiveTurnChanged.Remove(ActiveTurnChangedHandle);
	}

	UnsubscribeFromAbilitySystems();
	DestroyCameraActor();

	Super::EndPlay(EndPlayReason);
}

//  Public Query 

bool UPBTacticalCameraComponent::IsTacticalCameraActive() const
{
	if (!IsValid(CameraActor))
	{
		return false;
	}
	const APlayerController* PC = GetOwner<APlayerController>();
	return IsValid(PC) && PC->GetViewTarget() == CameraActor;
}

//  Combat Events 

void UPBTacticalCameraComponent::HandleCombatStateChanged(EPBCombatState NewState)
{
	if (NewState == EPBCombatState::TurnInProgress)
	{
		SubscribeToAbilitySystems();
	}
	else if (NewState == EPBCombatState::CombatEnding || NewState == EPBCombatState::OutOfCombat)
	{
		UnsubscribeFromAbilitySystems();
	}
}

void UPBTacticalCameraComponent::HandleActiveTurnChanged(AActor* NewCombatant, int32 TurnIndex)
{
	if (!IsValid(CameraActor))
	{
		return;
	}

	APlayerController* PC = GetOwner<APlayerController>();
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return;
	}
	
	if (!IsEnemyFaction(NewCombatant))
	{
		PC->SetViewTargetWithBlend(NewCombatant, BlendTime , VTBlend_Cubic);
		return;
	}
	
	const FVector EnemyPos        = NewCombatant->GetActorLocation();
	const FRotator CurrentCamRot  = PC->PlayerCameraManager->GetCameraRotation();
	const FVector CamLoc          = CalculateOrbitLocation(EnemyPos, CurrentCamRot.Yaw, CurrentCamRot.Pitch, DefaultDistance);
	const FRotator CamRot         = CalculateOrbitRotation(CamLoc, EnemyPos, CurrentCamRot);
	
	// Orbit 추적 시작 — 매 틱 적 위치 기준으로 재계산
	CameraActor->SetTrackingTarget(NewCombatant, CurrentCamRot.Yaw, DefaultPitch, DefaultDistance);

	if (PC->GetViewTarget() != CameraActor)
	{
		CameraActor->SetActorLocationAndRotation(CamLoc, CamRot);
		PC->SetViewTargetWithBlend(CameraActor, BlendTime);
	}
}

void UPBTacticalCameraComponent::HandleAbilityExecutionStarted(
	const UGameplayAbility* Ability, const FPBAbilityTargetData& TargetData)
{
	if (!IsValid(CameraActor))
	{
		return;
	}

	AActor* Caster = Ability->GetAvatarActorFromActorInfo();
	if (!IsValid(Caster))
	{
		return;
	}

	// 모든 피사체 위치 수집 (시전자 포함)
	TArray<FVector> SubjectLocations;
	SubjectLocations.Add(Caster->GetActorLocation());

	switch (TargetData.TargetingMode)
	{
	case EPBTargetingMode::SingleTarget:
		{
			AActor* Target = TargetData.GetSingleTargetActor();
			if (IsValid(Target))
			{
				SubjectLocations.Add(Target->GetActorLocation());
			}
			break;
		}
	case EPBTargetingMode::MultiTarget:
		{
			for (AActor* T : TargetData.GetAllTargetActors())
			{
				if (IsValid(T))
				{
					SubjectLocations.Add(T->GetActorLocation());
				}
			}
			break;
		}
	case EPBTargetingMode::AoE:
	case EPBTargetingMode::Location:
		SubjectLocations.Add(TargetData.GetSingleTargetLocation());
		break;
	default:
		break;
	}

	// 가시성 검사 — 모든 피사체가 이미 유효 뷰포트 안이면 카메라 이동 불필요
	if (AreSubjectsInView(SubjectLocations))
	{
		return;
	}

	// FocusPoint = 모든 피사체의 중심
	FVector FocusPoint = FVector::ZeroVector;
	for (const FVector& L : SubjectLocations)
	{
		FocusPoint += L;
	}
	FocusPoint /= static_cast<float>(SubjectLocations.Num());

	// 피사체 간 최대 거리 산출 → 적응형 Pitch 결정
	float SubjectSpread = 0.0f;
	for (int32 i = 0; i < SubjectLocations.Num(); ++i)
	{
		for (int32 j = i + 1; j < SubjectLocations.Num(); ++j)
		{
			SubjectSpread = FMath::Max(SubjectSpread, FVector::Dist(SubjectLocations[i], SubjectLocations[j]));
		}
	}

	float DesiredPitch = CalculateDesiredPitch(SubjectSpread);
	const FRotator CurrentCamRot = CameraActor->GetActorRotation();
	const float Yaw = CurrentCamRot.Yaw;

	float Height = CalculateSkillFramingHeight(FocusPoint, Yaw, DesiredPitch, SubjectLocations);

	// 극단적 줌아웃 시 수직 탑다운 폴백 — 최대 거리의 80% 초과 시 -90°로 전환
	if (Height > FramingMaxDistance * 0.8f)
	{
		DesiredPitch = -90.0f;
		Height = CalculateSkillFramingHeight(FocusPoint, Yaw, DesiredPitch, SubjectLocations);
	}

	const FVector CamLoc   = CalculateOrbitLocation(FocusPoint, Yaw, DesiredPitch, Height);
	const FRotator CamRot  = CalculateOrbitRotation(CamLoc, FocusPoint, CurrentCamRot);

	CameraActor->SetTargetTransform(FTransform(CamRot, CamLoc));
}

float UPBTacticalCameraComponent::CalculateSkillFramingHeight(
	const FVector& FocusPoint, float InYaw, float InPitch, const TArray<FVector>& SubjectLocations) const
{
	if (SubjectLocations.IsEmpty())
	{
		return FramingMinDistance;
	}

	APlayerController* PC = GetOwner<APlayerController>();
	if (!IsValid(PC) || !IsValid(CameraActor))
	{
		return FramingMinDistance;
	}

	// 뷰포트 크기 취득
	int32 ViewSizeX = 0, ViewSizeY = 0;
	PC->GetViewportSize(ViewSizeX, ViewSizeY);
	if (ViewSizeX == 0 || ViewSizeY == 0)
	{
		return FramingMinDistance;
	}

	const float AspectRatio  = static_cast<float>(ViewSizeX) / static_cast<float>(ViewSizeY);
	const float VFovDeg      = CameraActor->GetCameraComponent()->FieldOfView;
	const float TanHalfVFov  = FMath::Tan(FMath::DegreesToRadians(VFovDeg * 0.5f));
	const float TanHalfHFov  = TanHalfVFov * AspectRatio;

	// 방향별 유효 반 FOV — 스크린 중심에서 각 패딩 경계까지의 실제 탄젠트
	// 전체 폭 = 2 * TanHalfHFov * D, 좌측 패딩 = Left * 전체 폭
	// → 중심→좌측 유효 = TanHalfHFov * (1 - 2*Left)
	const float EffTanLeft   = TanHalfHFov * FMath::Max(0.1f, 1.0f - 2.0f * ScreenPadding.Left);
	const float EffTanRight  = TanHalfHFov * FMath::Max(0.1f, 1.0f - 2.0f * ScreenPadding.Right);
	const float EffTanTop    = TanHalfVFov * FMath::Max(0.1f, 1.0f - 2.0f * ScreenPadding.Top);
	const float EffTanBottom = TanHalfVFov * FMath::Max(0.1f, 1.0f - 2.0f * ScreenPadding.Bottom);

	// 카메라 스크린 축 계산 (적응형 Pitch 사용)
	const FRotationMatrix CamMat(FRotator(InPitch, InYaw, 0.0f));
	const FVector CamForward = CamMat.GetScaledAxis(EAxis::X);	// 카메라 전방 (피사체 깊이 계산용)
	const FVector CamRight   = CamMat.GetScaledAxis(EAxis::Y);	// 스크린 수평 축
	const FVector CamUp      = CamMat.GetScaledAxis(EAxis::Z);	// 스크린 수직 축

	float RequiredH = FramingMinDistance;

	for (const FVector& Loc : SubjectLocations)
	{
		const FVector Rel = Loc - FocusPoint;

		// 원근 투영 깊이 보정: FocusPoint 기준 카메라 전방 오프셋
		// (피사체가 FocusPoint보다 카메라에 가까울수록 더 넓게 보이므로 거리 보정 필요)
		const float DepthOffset = FVector::DotProduct(Rel, CamForward);

		// 부호 있는 스크린 축 투영 — 방향에 따라 해당 측 패딩 적용
		const float ProjH = FVector::DotProduct(Rel, CamRight);	// + = 우측, - = 좌측
		const float ProjV = FVector::DotProduct(Rel, CamUp);		// + = 상단, - = 하단

		// 수평: 피사체가 우측이면 Right 패딩, 좌측이면 Left 패딩 기준
		const float AvailH = (ProjH >= 0.0f) ? EffTanRight : EffTanLeft;
		const float ExtH = FMath::Abs(ProjH) + SubjectWorldPadding;
		if (AvailH > KINDA_SMALL_NUMBER)
		{
			RequiredH = FMath::Max(RequiredH, ExtH / AvailH + DepthOffset);
		}

		// 수직: 피사체가 상단이면 Top 패딩, 하단이면 Bottom 패딩 기준
		const float AvailV = (ProjV >= 0.0f) ? EffTanTop : EffTanBottom;
		const float ExtV = FMath::Abs(ProjV) + SubjectWorldPadding;
		if (AvailV > KINDA_SMALL_NUMBER)
		{
			RequiredH = FMath::Max(RequiredH, ExtV / AvailV + DepthOffset);
		}
	}

	return FMath::Clamp(RequiredH, FramingMinDistance, FramingMaxDistance);
}

//  Camera Actor 

void UPBTacticalCameraComponent::SpawnCameraActor()
{
	if (IsValid(CameraActor))
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	CameraActor  = GetWorld()->SpawnActor<APBTacticalCameraActor>(Params);
}

void UPBTacticalCameraComponent::DestroyCameraActor()
{
	if (IsValid(CameraActor))
	{
		CameraActor->Destroy();
		CameraActor = nullptr;
	}
}

//  ASC Subscription 

void UPBTacticalCameraComponent::SubscribeToAbilitySystems()
{
	UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!IsValid(CombatManager))
	{
		return;
	}

	for (const FPBInitiativeEntry& Entry : CombatManager->GetInitiativeOrder())
	{
		AActor* Combatant = Entry.Combatant.Get();
		if (!IsValid(Combatant))
		{
			continue;
		}

		UPBAbilitySystemComponent* PBASC = Combatant->FindComponentByClass<UPBAbilitySystemComponent>();
		if (!IsValid(PBASC))
		{
			continue;
		}

		FASCEntry NewEntry;
		NewEntry.ASC                  = PBASC;
		NewEntry.ExecutionStartedHandle = PBASC->OnAbilityExecutionStarted.AddUObject(
			this, &UPBTacticalCameraComponent::HandleAbilityExecutionStarted);
		ASCEntries.Add(NewEntry);
	}
}

void UPBTacticalCameraComponent::UnsubscribeFromAbilitySystems()
{
	for (FASCEntry& Entry : ASCEntries)
	{
		if (UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(Entry.ASC.Get()))
		{
			PBASC->OnAbilityExecutionStarted.Remove(Entry.ExecutionStartedHandle);
		}
	}
	ASCEntries.Empty();
}

//  Orbit Math 

FVector UPBTacticalCameraComponent::CalculateOrbitLocation(
	const FVector& FocusPoint, float Yaw, float Pitch, float Distance) const
{
	const FRotator OrbitRotation(Pitch, Yaw, 0.0f);
	return FocusPoint + OrbitRotation.Vector() * -Distance;
}

FRotator UPBTacticalCameraComponent::CalculateOrbitRotation(
	const FVector& CameraLocation, const FVector& FocusPoint, const FRotator& CurrentRotation) const
{
	const FRotator RawRot = (FocusPoint - CameraLocation).Rotation();
	// CurrentRotation에서 RawRot까지의 최소 델타를 더해 보간 방향이 항상 최단경로가 되도록 한다
	const FRotator Delta = (RawRot - CurrentRotation).GetNormalized();
	return CurrentRotation + Delta;
}

float UPBTacticalCameraComponent::CalculateDesiredDistance(float SubjectDistance) const
{
	const float Alpha = FMath::GetMappedRangeValueClamped(
		FVector2D(FramingNearThreshold, FramingFarThreshold),
		FVector2D(0.0f, 1.0f),
		SubjectDistance);
	return FMath::Lerp(FramingMinDistance, FramingMaxDistance, Alpha);
}

float UPBTacticalCameraComponent::CalculateDesiredPitch(float SubjectDistance) const
{
	const float Alpha = FMath::GetMappedRangeValueClamped(
		FVector2D(FramingNearThreshold, FramingFarThreshold),
		FVector2D(0.0f, 1.0f),
		SubjectDistance);
	return FMath::Lerp(FramingNearPitch, FramingFarPitch, Alpha);
}

//  Visibility 

bool UPBTacticalCameraComponent::AreSubjectsInView(const TArray<FVector>& SubjectLocations) const
{
	APlayerController* PC = GetOwner<APlayerController>();
	if (!IsValid(PC))
	{
		return false;
	}

	int32 ViewSizeX = 0, ViewSizeY = 0;
	PC->GetViewportSize(ViewSizeX, ViewSizeY);
	if (ViewSizeX == 0 || ViewSizeY == 0)
	{
		return false;
	}

	// 패딩을 제외한 유효 스크린 영역 (픽셀)
	const float MinX = ScreenPadding.Left   * static_cast<float>(ViewSizeX);
	const float MaxX = (1.0f - ScreenPadding.Right)  * static_cast<float>(ViewSizeX);
	const float MinY = ScreenPadding.Top    * static_cast<float>(ViewSizeY);
	const float MaxY = (1.0f - ScreenPadding.Bottom) * static_cast<float>(ViewSizeY);

	for (const FVector& Loc : SubjectLocations)
	{
		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(Loc, ScreenPos))
		{
			return false;	// 카메라 뒤편
		}

		if (ScreenPos.X < MinX || ScreenPos.X > MaxX || ScreenPos.Y < MinY || ScreenPos.Y > MaxY)
		{
			return false;
		}
	}

	return true;
}

//  Utility 

bool UPBTacticalCameraComponent::IsEnemyFaction(AActor* Actor) const
{
	const IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Actor);
	if (!Participant)
	{
		return false;
	}
	return Participant->GetFactionTag().MatchesTag(EnemyFactionTag);
}
