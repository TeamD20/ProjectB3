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

	// 각 후보 포인트에서 모든 타겟에 대해 LoS 판정
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector CandidatePos = GetItemLocation(QueryInstance, It.GetIndex());
		bool bHasLoS = true;

		for (const AActor* Target : TargetActors)
		{
			if (!IsValid(Target))
			{
				continue;
			}

			const FPBLoSResult LoSResult = EnvSubsystem->CheckLineOfSight(CandidatePos, Target);
			if (!LoSResult.bHasLineOfSight)
			{
				bHasLoS = false;
				break;
			}
		}

		It.SetScore(TestPurpose, FilterType, bHasLoS, true);
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
