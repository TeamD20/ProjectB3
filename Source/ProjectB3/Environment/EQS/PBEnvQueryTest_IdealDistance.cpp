// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryTest_IdealDistance.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "ProjectB3/AI/PBUtilityClearinghouse.h"

UPBEnvQueryTest_IdealDistance::UPBEnvQueryTest_IdealDistance()
{
	// SimpleGrid(Point) 아이템 호환 명시 — 미설정 시 "can't use test" 제거됨
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();

	// Score 전용 — 필터링은 AbilityRange 테스트가 담당
	Cost = EEnvTestCost::Low;

	// float 기반 스코어링 활성화
	SetWorkOnFloatValues(true);

	// 기본 타겟 Context는 없음 — 에디터에서 설정
	TargetContext = nullptr;
}

void UPBEnvQueryTest_IdealDistance::RunTest(FEnvQueryInstance& QueryInstance) const
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

	// Clearinghouse에서 이상적 교전 거리 읽기
	const float IdealDistance = Clearinghouse->GetEQSIdealDistance();

	// IdealDistance <= 0 = 선호도 없음 → 모든 포인트 동일 점수
	if (IdealDistance <= 0.f)
	{
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			It.SetScore(TestPurpose, FilterType, 1.0f, 0.f, 1.f);
		}
		return;
	}

	// 타겟 Context에서 대상 위치 수집
	if (!TargetContext)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EQS] PBEnvQueryTest_IdealDistance: TargetContext가 설정되지 "
			     "않았습니다! EQS 에셋에서 TargetContext를 "
			     "PBEnvQueryContext_Target으로 설정하세요. "
			     "(미설정 시 스코어링이 동작하지 않습니다)"));
		return;
	}

	TArray<FVector> TargetLocations;
	QueryInstance.PrepareContext(TargetContext, TargetLocations);

	if (TargetLocations.Num() == 0)
	{
		return;
	}

	// 타겟이 여러 명이면 첫 번째 타겟 사용 (Attack Position은 단일 타겟)
	const FVector TargetPos = TargetLocations[0];
	const float InvNormRange = 1.0f / FMath::Max(NormalizationRange, 1.0f);

	// 각 후보 포인트 스코어링:
	// Score = 1.0 - Clamp(|Dist2D - IdealDistance| / NormalizationRange, 0, 1)
	// → 이상 거리에 정확히 있으면 1.0, 멀어질수록 0.0에 수렴
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemPos = GetItemLocation(QueryInstance, It.GetIndex());
		const float Dist2D = FVector::Dist2D(ItemPos, TargetPos);
		const float Deviation = FMath::Abs(Dist2D - IdealDistance);
		const float Score = 1.0f - FMath::Clamp(Deviation * InvNormRange, 0.f, 1.f);

		It.SetScore(TestPurpose, FilterType, Score, 0.f, 1.f);
	}
}

FText UPBEnvQueryTest_IdealDistance::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("PB Ideal Distance (2D)"));
}

FText UPBEnvQueryTest_IdealDistance::GetDescriptionDetails() const
{
	return FText::FromString(
		TEXT("Clearinghouse의 EQSIdealDistance 기반으로 "
		     "이상적 교전 거리에 가까운 후보일수록 높은 점수를 부여한다."));
}
