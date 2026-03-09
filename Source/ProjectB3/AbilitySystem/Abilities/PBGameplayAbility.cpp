// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/Tasks/PBAbilityTask_WaitTargeting.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAbility, Log, All);

void UPBGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Payload 확인 — 존재하면 타겟 결정 완료
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
			UE_LOG(LogPBAbility, Warning, TEXT("[%s] AI 경로: 타겟이 사거리를 벗어났습니다."), *GetName());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			UE_LOG(LogPBAbility, Warning, TEXT("[%s] AI 경로: 어빌리티 Commit 실패 (자원 부족)."), *GetName());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		ExecuteAbilityLogic(TargetData);
		TryAutoEndAbility(Handle, ActorInfo, ActivationInfo);
		return;
	}

	// 플레이어 경로: 타겟팅 모드에 따라 분기
	const FVector SourceLocation = GetAvatarActorFromActorInfo()->GetActorLocation();

	switch (TargetingMode)
	{
	case EPBTargetingMode::None:
		{
			FPBAbilityTargetData TargetData;
			TargetData.TargetingMode = EPBTargetingMode::None;

			if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
			{
				UE_LOG(LogPBAbility, Warning, TEXT("[%s] None 경로: 어빌리티 Commit 실패."), *GetName());
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
			}

			ExecuteAbilityLogic(TargetData);
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
				UE_LOG(LogPBAbility, Warning, TEXT("[%s] Self 경로: 어빌리티 Commit 실패."), *GetName());
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
			}

			ExecuteAbilityLogic(TargetData);
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
		UE_LOG(LogPBAbility, Error, TEXT("[%s] 알 수 없는 타겟팅 모드입니다."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		break;
	}
}

bool UPBGameplayAbility::IsRangedAbility() const
{
	return Range > 0.f;
}

bool UPBGameplayAbility::IsTargetInRange(
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
		// 사거리 검증 불필요
		return true;

	case EPBTargetingMode::SingleTarget:
	case EPBTargetingMode::MultiTarget:
		{
			// 모든 타겟이 사거리 내에 있어야 유효
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
			return TargetData.TargetActors.Num() > 0;
		}

	case EPBTargetingMode::Location:
	case EPBTargetingMode::AoE:
		{
			const float DistSq = FVector::DistSquaredXY(SourceLocation, TargetData.TargetLocation);
			return DistSq <= FMath::Square(Range);
		}

	default:
		return false;
	}
}


FPBAbilityTargetData UPBGameplayAbility::ExtractTargetDataFromEvent(
	const FGameplayEventData& EventData) const
{
	if (const UPBTargetPayload* Payload = Cast<UPBTargetPayload>(EventData.OptionalObject))
	{
		return Payload->TargetData;
	}

	return FPBAbilityTargetData();
}

UGameplayEffect* UPBGameplayAbility::GetCooldownGameplayEffect() const
{
	// TODO: 턴제 쿨다운은 GAS 내장 CooldownTag GE를 사용하지 않는다.
	// 추후 턴 종료 시점에 별도 쿨다운 카운터로 관리 예정.
	return nullptr;
}

void UPBGameplayAbility::StartTargetingTask()
{
	UPBAbilityTask_WaitTargeting* Task = UPBAbilityTask_WaitTargeting::CreateTask(this);
	if (!IsValid(Task))
	{
		UE_LOG(LogPBAbility, Error, TEXT("[%s] WaitTargeting Task 생성 실패."), *GetName());
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, true);
		return;
	}

	Task->OnTargetConfirmed.AddUObject(this, &UPBGameplayAbility::OnTargetingConfirmed);
	Task->OnTargetCancelled.AddUObject(this, &UPBGameplayAbility::OnTargetingCancelled);
	Task->ReadyForActivation();
}

void UPBGameplayAbility::OnTargetingConfirmed(const FPBAbilityTargetData& TargetData)
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
		UE_LOG(LogPBAbility, Warning, TEXT("[%s] 타겟팅 확정: 사거리 초과."), *GetName());
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo))
	{
		UE_LOG(LogPBAbility, Warning, TEXT("[%s] 타겟팅 확정: Commit 실패."), *GetName());
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	ExecuteAbilityLogic(TargetData);
	TryAutoEndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo);
}
void UPBGameplayAbility::TryAutoEndAbility(
	const FGameplayAbilitySpecHandle& Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo& ActivationInfo)
{
	if (EndMode == EPBAbilityEndMode::Auto && IsActive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UPBGameplayAbility::OnTargetingCancelled()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo)
	{
		EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, true);
	}
}

