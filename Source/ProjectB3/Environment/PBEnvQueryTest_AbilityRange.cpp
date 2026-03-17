// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryTest_AbilityRange.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "ProjectB3/AI/PBUtilityClearinghouse.h"

UPBEnvQueryTest_AbilityRange::UPBEnvQueryTest_AbilityRange()
{
	// Filter 모드 기본 설정 — 사거리 밖 후보를 제거
	FilterType = EEnvTestFilterType::Match;
	Cost = EEnvTestCost::Low;

	// 점수 산출 없이 필터 전용
	SetWorkOnFloatValues(false);

	// 기본 타겟 Context는 없음 — 에디터에서 설정
	TargetContext = nullptr;
}

void UPBEnvQueryTest_AbilityRange::RunTest(FEnvQueryInstance& QueryInstance) const
{
	// Clearinghouse 서브시스템 취득
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

	const UPBUtilityClearinghouse* Clearinghouse =
		World->GetSubsystem<UPBUtilityClearinghouse>();
	if (!IsValid(Clearinghouse))
	{
		return;
	}

	// Clearinghouse에서 동적 사거리 읽기
	const float MaxRange = Clearinghouse->GetEQSAbilityMaxRange();

	// MaxRange <= 0 = 사거리 무제한 → 모든 포인트 통과
	if (MaxRange <= 0.f)
	{
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			It.SetScore(TestPurpose, FilterType, true, true);
		}
		return;
	}

	// 타겟 Context에서 대상 위치 수집
	if (!TargetContext)
	{
		return;
	}

	TArray<FVector> TargetLocations;
	QueryInstance.PrepareContext(TargetContext, TargetLocations);

	if (TargetLocations.Num() == 0)
	{
		return;
	}

	const float MaxRangeSq = FMath::Square(MaxRange);

	// 각 후보 포인트에서 모든 타겟에 대해 2D 거리(XY) 사거리 판정
	// IsTargetInRange(FVector::DistSquaredXY)와 동일한 기준
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemPos = GetItemLocation(QueryInstance, It.GetIndex());
		bool bInRange = true;

		for (const FVector& TargetPos : TargetLocations)
		{
			const float DistSqXY = FVector::DistSquaredXY(ItemPos, TargetPos);
			if (DistSqXY > MaxRangeSq)
			{
				bInRange = false;
				break;
			}
		}

		It.SetScore(TestPurpose, FilterType, bInRange, true);
	}
}

FText UPBEnvQueryTest_AbilityRange::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("PB Ability Range (2D)"));
}

FText UPBEnvQueryTest_AbilityRange::GetDescriptionDetails() const
{
	return FText::FromString(
		TEXT("Clearinghouse의 EQSAbilityMaxRange를 기반으로 "
		     "타겟과의 2D 거리(XY)가 사거리 이내인 후보만 통과시킨다."));
}
