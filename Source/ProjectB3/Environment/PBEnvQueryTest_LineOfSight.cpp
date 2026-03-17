// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryTest_LineOfSight.h"
#include "PBEnvironmentSubsystem.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "Engine/GameInstance.h"

UPBEnvQueryTest_LineOfSight::UPBEnvQueryTest_LineOfSight()
{
	// Filter 모드로 기본 설정 — 시야가 없는 후보를 제거
	FilterType = EEnvTestFilterType::Match;
	Cost = EEnvTestCost::High;

	// 점수 산출 없이 필터 전용
	SetWorkOnFloatValues(false);

	// 기본 타겟 Context는 없음 — 에디터에서 설정
	TargetContext = nullptr;
}

void UPBEnvQueryTest_LineOfSight::RunTest(FEnvQueryInstance& QueryInstance) const
{
	// EnvironmentSubsystem 취득
	const UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!IsValid(QueryOwner))
	{
		return;
	}

	const UWorld* World = QueryOwner->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	const UPBEnvironmentSubsystem* EnvSubsystem = GameInstance->GetSubsystem<UPBEnvironmentSubsystem>();
	if (!IsValid(EnvSubsystem))
	{
		return;
	}

	// 타겟 Context에서 대상 액터 수집
	if (!TargetContext)
	{
		return;
	}

	TArray<AActor*> TargetActors;
	QueryInstance.PrepareContext(TargetContext, TargetActors);

	if (TargetActors.Num() == 0)
	{
		return;
	}

	// 각 후보 포인트에서 타겟에 대해 LoS 판정
	// bRequireAllTargets=true: 모든 타겟에 LoS 필요 (Attack용)
	// bRequireAllTargets=false: 1명이라도 LoS 있으면 통과 (Fallback용)
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector CandidatePos = GetItemLocation(QueryInstance, It.GetIndex());
		bool bResult = bRequireAllTargets; // All: true에서 시작, Any: false에서 시작

		for (const AActor* Target : TargetActors)
		{
			if (!IsValid(Target))
			{
				continue;
			}

			const FPBLoSResult LoSResult = EnvSubsystem->CheckLineOfSight(CandidatePos, Target);

			if (bRequireAllTargets)
			{
				// All 모드: 하나라도 실패하면 false
				if (!LoSResult.bHasLineOfSight)
				{
					bResult = false;
					break;
				}
			}
			else
			{
				// Any 모드: 하나라도 성공하면 true
				if (LoSResult.bHasLineOfSight)
				{
					bResult = true;
					break;
				}
			}
		}

		It.SetScore(TestPurpose, FilterType, bResult, true);
	}
}

FText UPBEnvQueryTest_LineOfSight::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("PB Line of Sight"));
}

FText UPBEnvQueryTest_LineOfSight::GetDescriptionDetails() const
{
	return FText::FromString(TEXT("EnvironmentSubsystem의 CheckLineOfSight를 사용한 시야 필터"));
}
