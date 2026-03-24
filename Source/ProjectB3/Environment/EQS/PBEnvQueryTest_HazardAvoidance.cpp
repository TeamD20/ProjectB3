// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryTest_HazardAvoidance.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "Engine/GameInstance.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"

UPBEnvQueryTest_HazardAvoidance::UPBEnvQueryTest_HazardAvoidance()
{
	// SimpleGrid(Point) 아이템 호환
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

	// Score 모드: 필터가 아닌 점수 산출 (모든 포인트가 장판 안일 수 있으므로)
	Cost = EEnvTestCost::Low;
	SetWorkOnFloatValues(true);
}

void UPBEnvQueryTest_HazardAvoidance::RunTest(FEnvQueryInstance& QueryInstance) const
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

	// 각 후보 위치에서 위험 영역 포함 여부 판정 → 점수 산출
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector CandidatePos = GetItemLocation(QueryInstance, It.GetIndex());

		const FPBHazardQueryResult HazardResult = EnvSubsystem->QueryHazardAtPoint(CandidatePos);

		// Score = 위험 영역 밖이면 1.0, 안이면 0.0
		const float Score = HazardResult.bIsInHazard ? 0.0f : 1.0f;
		It.SetScore(TestPurpose, FilterType, Score, 0.0f, 1.0f);
	}
}

FText UPBEnvQueryTest_HazardAvoidance::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("PB Hazard Avoidance"));
}

FText UPBEnvQueryTest_HazardAvoidance::GetDescriptionDetails() const
{
	return FText::FromString(TEXT("위험 영역(DamageArea) 내 후보 포인트 감점 (0.0=위험, 1.0=안전)"));
}
