// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryTest_CoverScore.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "Engine/GameInstance.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"

UPBEnvQueryTest_CoverScore::UPBEnvQueryTest_CoverScore()
{
	// SimpleGrid(Point) 아이템 호환
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

	// Score 모드: 점수 산출 테스트 (필터가 아님)
	Cost = EEnvTestCost::High;
	SetWorkOnFloatValues(true);

	// 기본 타겟 Context — 에디터에서 설정
	TargetContext = nullptr;
}

void UPBEnvQueryTest_CoverScore::RunTest(FEnvQueryInstance& QueryInstance) const
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
		UE_LOG(LogTemp, Error,
			TEXT("[EQS] CoverScore: TargetContext=null — 스코어링 스킵!"));
		return;
	}

	TArray<AActor*> TargetActors;
	QueryInstance.PrepareContext(TargetContext, TargetActors);

	if (TargetActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[EQS] CoverScore: 타겟 액터 0명 — 스코어링 스킵."));
		return;
	}

	const float TotalEnemies = static_cast<float>(TargetActors.Num());

	// 각 후보 위치에서 적에 대한 LoS 검사 → 엄폐 점수 산출
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector CandidatePos = GetItemLocation(QueryInstance, It.GetIndex());

		int32 VisibleCount = 0;
		for (const AActor* Target : TargetActors)
		{
			if (!IsValid(Target))
			{
				continue;
			}

			const FPBLoSResult LoSResult = EnvSubsystem->CheckLineOfSight(CandidatePos, Target);
			if (LoSResult.bHasLineOfSight)
			{
				++VisibleCount;
			}
		}

		// Score = 엄폐 비율 (0.0 = 전부 노출, 1.0 = 완전 엄폐)
		const float CoverRatio = (TotalEnemies - static_cast<float>(VisibleCount)) / TotalEnemies;
		It.SetScore(TestPurpose, FilterType, CoverRatio, 0.0f, 1.0f);
	
	}

	UE_LOG(LogTemp, Display,
		TEXT("[EQS] CoverScore 완료: 타겟=%d명, 후보 포인트 스코어링 완료"),
		TargetActors.Num());
}

FText UPBEnvQueryTest_CoverScore::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("PB Cover Score"));
}

FText UPBEnvQueryTest_CoverScore::GetDescriptionDetails() const
{
	return FText::FromString(TEXT("적에게 노출되지 않는 비율 기반 엄폐 점수 (0.0~1.0)"));
}
