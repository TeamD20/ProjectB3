// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryTest_AbilityRange.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "ProjectB3/AI/PBUtilityClearinghouse.h"

UPBEnvQueryTest_AbilityRange::UPBEnvQueryTest_AbilityRange()
{
	// SimpleGrid(Point) 아이템 호환 명시 — 미설정 시 "can't use test" 제거됨
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

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
		UE_LOG(LogTemp, Error, TEXT("[EQS] AbilityRange: QueryOwner 무효 — 조기 리턴"));
		return;
	}

	const UWorld* World = QueryOwner->GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Error, TEXT("[EQS] AbilityRange: World 무효 — 조기 리턴"));
		return;
	}

	const UPBUtilityClearinghouse* Clearinghouse =
		World->GetSubsystem<UPBUtilityClearinghouse>();
	if (!IsValid(Clearinghouse))
	{
		UE_LOG(LogTemp, Error, TEXT("[EQS] AbilityRange: Clearinghouse 무효 — 조기 리턴"));
		return;
	}

	// Clearinghouse에서 동적 사거리 읽기
	const float MaxRange = Clearinghouse->GetEQSAbilityMaxRange();

	// MaxRange <= 0 = 사거리 무제한 → 모든 포인트 통과
	if (MaxRange <= 0.f)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[EQS] AbilityRange: MaxRange=%.0f (무제한) — 전체 통과"),
			MaxRange);
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			It.SetScore(TestPurpose, FilterType, true, true);
		}
		return;
	}

	// 타겟 Context에서 대상 위치 수집
	if (!TargetContext)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EQS] AbilityRange: TargetContext=null — 필터링 스킵!"));
		return;
	}

	TArray<FVector> TargetLocations;
	QueryInstance.PrepareContext(TargetContext, TargetLocations);

	if (TargetLocations.Num() == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EQS] AbilityRange: PrepareContext → 타겟 0개 — 필터링 스킵!"));
		return;
	}

	const float MaxRangeSq = FMath::Square(MaxRange);
	int32 PassCount = 0;
	int32 FailCount = 0;

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
		if (bInRange) { ++PassCount; } else { ++FailCount; }
	}

	UE_LOG(LogTemp, Display,
		TEXT("[EQS] AbilityRange 결과: MaxRange=%.0f, Target=(%s), "
		     "통과=%d, 탈락=%d, TestPurpose=%d"),
		MaxRange,
		*TargetLocations[0].ToCompactString(),
		PassCount, FailCount,
		static_cast<int32>(TestPurpose));
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
