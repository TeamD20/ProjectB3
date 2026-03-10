// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_Targeted.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/Tasks/PBAbilityTask_WaitTargeting.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAbilityTargeted, Log, All);

void UPBGameplayAbility_Targeted::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Payload 확인 — 존재하면 타겟 결정 완료 (AI 경로)
	const UPBTargetPayload* Payload = TriggerEventData
		? Cast<UPBTargetPayload>(TriggerEventData->OptionalObject)
		: nullptr;

	if (IsValid(Payload))
	{
		const FPBAbilityTargetData& TargetData = Payload->TargetData;

		// 사거리 검증
		const FVector SourceLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
		if (!IsTargetInRange(SourceLocation, TargetData))
		{
			UE_LOG(LogPBAbilityTargeted, Warning, TEXT("[%s] AI 경로: 타겟이 사거리를 벗어났습니다."), *GetName());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			UE_LOG(LogPBAbilityTargeted, Warning, TEXT("[%s] AI 경로: 어빌리티 Commit 실패 (자원 부족)."), *GetName());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		K2_ExecuteTargetLogic(TargetData);
		TryAutoEndAbility(Handle, ActorInfo, ActivationInfo);
		return;
	}

	// 플레이어 경로: 타겟팅 모드에 따라 분기
	switch (TargetingMode)
	{
	case EPBTargetingMode::None:
		{
			FPBAbilityTargetData TargetData;
			TargetData.TargetingMode = EPBTargetingMode::None;

			if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
			{
				UE_LOG(LogPBAbilityTargeted, Warning, TEXT("[%s] None 경로: 어빌리티 Commit 실패."), *GetName());
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
			}

			K2_ExecuteTargetLogic(TargetData);
			TryAutoEndAbility(Handle, ActorInfo, ActivationInfo);
			break;
		}

	case EPBTargetingMode::Self:
		{
			FPBAbilityTargetData TargetData;
			TargetData.TargetingMode = EPBTargetingMode::Self;
			TargetData.TargetActors.Add(TWeakObjectPtr<AActor>(GetAvatarActorFromActorInfo()));

			if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
			{
				UE_LOG(LogPBAbilityTargeted, Warning, TEXT("[%s] Self 경로: 어빌리티 Commit 실패."), *GetName());
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
			}

			K2_ExecuteTargetLogic(TargetData);
			TryAutoEndAbility(Handle, ActorInfo, ActivationInfo);
			break;
		}

	case EPBTargetingMode::SingleTarget:
	case EPBTargetingMode::MultiTarget:
	case EPBTargetingMode::Location:
	case EPBTargetingMode::AoE:
		{
			// AbilityTask로 비동기 타겟팅 진입
			StartTargetingTask();
			break;
		}

	default:
		UE_LOG(LogPBAbilityTargeted, Error, TEXT("[%s] 알 수 없는 타겟팅 모드입니다."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		break;
	}
}

FPBTargetingRequest UPBGameplayAbility_Targeted::MakeTargetingRequest() const
{
	FPBTargetingRequest Request;
	Request.RequestingAbility  = const_cast<UPBGameplayAbility_Targeted*>(this);
	Request.Mode               = TargetingMode;
	Request.OriginLocation     = GetAvatarActorFromActorInfo()->GetActorLocation();
	Request.AoERadius          = AoERadius;
	Request.MaxTargetCount     = MaxTargetCount;
	Request.bAllowGroundTarget = bAllowGroundTarget;
	return Request;
}

void UPBGameplayAbility_Targeted::K2_ExecuteTargetLogic_Implementation(const FPBAbilityTargetData& TargetData)
{
	ExecuteTargetLogic(TargetData);
}

void UPBGameplayAbility_Targeted::ExecuteTargetLogic(const FPBAbilityTargetData& TargetData)
{
}

bool UPBGameplayAbility_Targeted::IsTargetInRange(
	const FVector& SourceLocation,
	const FPBAbilityTargetData& TargetData) const
{
	// 사거리 무제한
	if (!IsRangedAbility())
	{
		return true;
	}

	switch (TargetData.TargetingMode)
	{
	case EPBTargetingMode::None:
	case EPBTargetingMode::Self:
		return true;

	case EPBTargetingMode::SingleTarget:
	case EPBTargetingMode::MultiTarget:
		{
			// 모든 타겟 액터가 사거리 내에 있어야 유효
			for (const TWeakObjectPtr<AActor>& Weak : TargetData.TargetActors)
			{
				if (!Weak.IsValid())
				{
					return false;
				}
				const float DistSq = FVector::DistSquaredXY(SourceLocation, Weak->GetActorLocation());
				if (DistSq > FMath::Square(Range))
				{
					return false;
				}
			}
			// bAllowGroundTarget 폴백: 위치 타겟도 사거리 검증
			for (const FVector& Loc : TargetData.TargetLocations)
			{
				const float DistSq = FVector::DistSquaredXY(SourceLocation, Loc);
				if (DistSq > FMath::Square(Range))
				{
					return false;
				}
			}
			return TargetData.HasTarget();
		}

	case EPBTargetingMode::Location:
	case EPBTargetingMode::AoE:
		{
			if (TargetData.TargetLocations.Num() == 0)
			{
				return false;
			}
			const float DistSq = FVector::DistSquaredXY(SourceLocation, TargetData.TargetLocations[0]);
			return DistSq <= FMath::Square(Range);
		}

	default:
		return false;
	}
}


void UPBGameplayAbility_Targeted::StartTargetingTask()
{
	UPBAbilityTask_WaitTargeting* Task = UPBAbilityTask_WaitTargeting::CreateTask(this);
	if (!IsValid(Task))
	{
		UE_LOG(LogPBAbilityTargeted, Error, TEXT("[%s] WaitTargeting Task 생성 실패."), *GetName());
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		return;
	}

	Task->OnTargetConfirmed.AddUObject(this, &UPBGameplayAbility_Targeted::OnTargetingConfirmed);
	Task->OnTargetCancelled.AddUObject(this, &UPBGameplayAbility_Targeted::OnTargetingCancelled);
	Task->ReadyForActivation();
}

void UPBGameplayAbility_Targeted::OnTargetingConfirmed(const FPBAbilityTargetData& TargetData)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return;
	}

	// 사거리 재검증
	const FVector SourceLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	if (!IsTargetInRange(SourceLocation, TargetData))
	{
		UE_LOG(LogPBAbilityTargeted, Warning, TEXT("[%s] 타겟팅 확정: 사거리 초과."), *GetName());
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogPBAbilityTargeted, Warning, TEXT("[%s] 타겟팅 확정: Commit 실패."), *GetName());
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	K2_ExecuteTargetLogic(TargetData);
	TryAutoEndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo);
}

void UPBGameplayAbility_Targeted::OnTargetingCancelled()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo)
	{
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
	}
}

void UPBGameplayAbility_Targeted::TryAutoEndAbility(
	const FGameplayAbilitySpecHandle& Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo& ActivationInfo)
{
	if (EndMode == EPBTargetedAbilityEndMode::Auto && IsActive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}
