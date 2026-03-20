// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyFollowSubsystem.h"

#include "PBPartyAIController.h"
#include "NavigationSystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "PBPartyFollowSettings.h"


void UPBPartyFollowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UPBCombatManagerSubsystem>();

	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	ScatterEQSQuery = Cast<UEnvQuery>(Settings->ScatterEQSQueryPath.TryLoad());
	TrailQueue.SetNum(TrailQueueMaxSize);

	if (UPBCombatManagerSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		CombatSubsystem->OnCombatStateChanged.AddUObject(this, &UPBPartyFollowSubsystem::OnCombatStateChanged);

		// 초기화 시점에 현재 전투 상태 동기화
		OnCombatStateChanged(CombatSubsystem->GetCombatState());
	}
}

void UPBPartyFollowSubsystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(TrailUpdateTimerHandle);

	if (UPBCombatManagerSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		CombatSubsystem->OnCombatStateChanged.RemoveAll(this);
	}

	for (FPBFollowerState& State : Followers)
	{
		if (IsValid(State.AIC))
		{
			State.AIC->OnPartyMoveCompleted.RemoveAll(this);
		}
	}

	Leader = nullptr;
	Followers.Empty();
	Super::Deinitialize();
}

void UPBPartyFollowSubsystem::NotifyLeaderMoveStarted(APBCharacterBase* InLeader)
{
	if (CurrentPhase == EPBPartyFollowPhase::CombatLocked)
	{
		return;
	}

	Leader = InLeader;
	RebuildFollowerCache();

	// Trail Queue 초기화
	TrailHead = 0;
	TrailCount = 0;
	LastRecordedLocation = IsValid(Leader) ? Leader->GetActorLocation() : FVector::ZeroVector;

	SetPhase(EPBPartyFollowPhase::Trailing);

	// Trail 기록 + 팔로워 디스패치 타이머 시작
	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	GetWorld()->GetTimerManager().SetTimer(
		TrailUpdateTimerHandle,
		this,
		&UPBPartyFollowSubsystem::OnTrailUpdateTimer,
		Settings->TrailRecordInterval,
		true
	);
}

void UPBPartyFollowSubsystem::NotifyLeaderMoveStopped()
{
	if (CurrentPhase != EPBPartyFollowPhase::Trailing)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TrailUpdateTimerHandle);
	SetPhase(EPBPartyFollowPhase::Scatter);
	RunScatterEQS();
}

void UPBPartyFollowSubsystem::SetFollowerUnlinked(APBCharacterBase* Follower, bool bUnlink)
{
	for (FPBFollowerState& State : Followers)
	{
		if (State.Character != Follower)
		{
			continue;
		}

		State.bUnlinked = bUnlink;
		if (bUnlink && IsValid(State.AIC))
		{
			State.AIC->StopFollowMove();
		}
		return;
	}
}

void UPBPartyFollowSubsystem::SetCombatLocked(bool bLocked)
{
	if (bLocked)
	{
		GetWorld()->GetTimerManager().ClearTimer(TrailUpdateTimerHandle);

		for (FPBFollowerState& State : Followers)
		{
			if (IsValid(State.AIC))
			{
				State.AIC->StopFollowMove();
			}
		}
		SetPhase(EPBPartyFollowPhase::CombatLocked);
	}
	else
	{
		TrailHead = 0;
		TrailCount = 0;
		SetPhase(EPBPartyFollowPhase::Idle);
	}
}

void UPBPartyFollowSubsystem::OnFollowerMoveCompleted(APBPartyAIController* AIC, bool bSuccess)
{
	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		if (Followers[i].AIC != AIC)
		{
			continue;
		}

		if (bSuccess)
		{
			Followers[i].ConsecutiveFailCount = 0;

			if (CurrentPhase == EPBPartyFollowPhase::Scatter)
			{
				// 모든 팔로워 Idle이면 Scatter 완료
				bool bAllIdle = true;
				for (const FPBFollowerState& State : Followers)
				{
					if (!State.bUnlinked && IsValid(State.AIC) &&
						State.AIC->GetMoveState() != EPBPartyMoveState::Idle)
					{
						bAllIdle = false;
						break;
					}
				}
				if (bAllIdle)
				{
					SetPhase(EPBPartyFollowPhase::Idle);
				}
			}
			else if (CurrentPhase == EPBPartyFollowPhase::Trailing)
			{
				// 도착 즉시 다음 Trail Point 디스패치
				DispatchTrailMove(i);
			}
		}
		else
		{
			Followers[i].ConsecutiveFailCount++;
			const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();

			if (CurrentPhase == EPBPartyFollowPhase::Trailing)
			{
				if (Followers[i].ConsecutiveFailCount >= Settings->FallbackFailThreshold)
				{
					Followers[i].ConsecutiveFailCount = 0;
					if (IsValid(Leader) && IsValid(AIC))
					{
						// Fallback: 리더 현재 위치로 직행
						AIC->MoveToTrailPoint(Leader->GetActorLocation());
					}
				}
				else
				{
					DispatchTrailMove(i);
				}
			}
			else if (CurrentPhase == EPBPartyFollowPhase::Scatter)
			{
				if (Followers[i].ConsecutiveFailCount >= Settings->FallbackFailThreshold)
				{
					Followers[i].ConsecutiveFailCount = 0;
					if (IsValid(Leader) && IsValid(AIC))
					{
						AIC->MoveToScatterPosition(Leader->GetActorLocation());
					}
				}
				else
				{
					RedispatchScatterForFollower(i);
				}
			}
		}
		break;
	}
}

void UPBPartyFollowSubsystem::RebuildFollowerCache()
{
	// 기존 바인딩 해제 및 이동 중단
	for (FPBFollowerState& State : Followers)
	{
		if (IsValid(State.AIC))
		{
			State.AIC->OnPartyMoveCompleted.RemoveAll(this);
			State.AIC->StopFollowMove();
		}
	}
	Followers.Empty();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!IsValid(PC))
	{
		return;
	}

	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	for (AActor* Member : PS->GetPartyMembers())
	{
		APBCharacterBase* Char = Cast<APBCharacterBase>(Member);
		// PlayerController가 빙의 중인 리더는 팔로워 목록에서 제외
		if (!IsValid(Char) || Char == Leader)
		{
			continue;
		}

		FPBFollowerState& NewState = Followers.AddDefaulted_GetRef();
		NewState.Character = Char;
		NewState.AIC = Cast<APBPartyAIController>(Char->GetController());

		if (IsValid(NewState.AIC))
		{
			NewState.AIC->OnPartyMoveCompleted.AddUObject(this, &UPBPartyFollowSubsystem::OnFollowerMoveCompleted);
		}
	}
}

void UPBPartyFollowSubsystem::OnTrailUpdateTimer()
{
	if (CurrentPhase != EPBPartyFollowPhase::Trailing || !IsValid(Leader))
	{
		return;
	}

	// 1. Trail Point 기록
	RecordTrailPoint(Leader->GetActorLocation());

	// 2. Idle 상태 팔로워에게 Trail Point 디스패치
	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		if (Followers[i].bUnlinked || !IsValid(Followers[i].AIC))
		{
			continue;
		}

		if (Followers[i].AIC->GetMoveState() == EPBPartyMoveState::Idle)
		{
			DispatchTrailMove(i);
		}
	}
}

// Trail Queue
void UPBPartyFollowSubsystem::RecordTrailPoint(const FVector& LeaderLocation)
{
	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	const float DistSq = FVector::DistSquared(LeaderLocation, LastRecordedLocation);
	if (DistSq < Settings->MinTrailDistance * Settings->MinTrailDistance)
	{
		return;
	}

	TrailQueue[TrailHead] = LeaderLocation;
	TrailHead = (TrailHead + 1) % TrailQueueMaxSize;
	if (TrailCount < TrailQueueMaxSize)
	{
		TrailCount++;
	}

	LastRecordedLocation = LeaderLocation;
}

bool UPBPartyFollowSubsystem::GetTrailPointForFollower(int32 FollowerIndex, FVector& OutPoint) const
{
	if (TrailCount == 0)
	{
		// 아직 어떠한 이동 경로 기록도 없음 (방금 이동을 시작한 직후)
		return false;
	}

	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	// 팔로워 i는 Head에서 (i+1) * TrailSpacing 만큼 과거의 지점을 추적
	// 체인 순서상 뒤에 있는 팔로워일수록 리더의 더 이전 위치를 따라감
	const int32 DesiredOffset = (FollowerIndex + 1) * Settings->TrailSpacing;
	
	// 아직 원하는 만큼 Trail이 충분히 쌓이지 않은 경우, 저장된 가장 오래된 과거 지점이라도 바라보고 움직이도록 Clamp 적용
	const int32 Offset = FMath::Min(DesiredOffset, TrailCount - 1);

	// Head-1이 가장 최근 기록, Head-1-Offset이 Offset만큼 과거 지점
	const int32 Idx = (TrailHead - 1 - Offset + TrailQueueMaxSize * 2) % TrailQueueMaxSize;
	OutPoint = TrailQueue[Idx];
	return true;
}

void UPBPartyFollowSubsystem::DispatchTrailMove(int32 FollowerIndex)
{
	FPBFollowerState& State = Followers[FollowerIndex];
	if (!IsValid(State.AIC) || !IsValid(State.Character))
	{
		return;
	}

	FVector TrailPoint;
	if (!GetTrailPointForFollower(FollowerIndex, TrailPoint))
	{
		return;
	}

	const FVector FromLoc = State.Character->GetActorLocation();
	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();

	// StopDistance 이내는 이동 불필요
	if (FVector::DistSquared(FromLoc, TrailPoint) < Settings->StopDistance * Settings->StopDistance)
	{
		return;
	}

	// Trail Point 사전 경로 유효성 검증
	if (!IsPointReachable(FromLoc, TrailPoint))
	{
		State.ConsecutiveFailCount++;
		if (State.ConsecutiveFailCount >= Settings->FallbackFailThreshold)
		{
			State.ConsecutiveFailCount = 0;
			if (IsValid(Leader))
			{
				State.AIC->MoveToTrailPoint(Leader->GetActorLocation());
			}
		}
		return;
	}

	State.ConsecutiveFailCount = 0;
	State.AIC->MoveToTrailPoint(TrailPoint);
}

// Scatter — EQS
void UPBPartyFollowSubsystem::RunScatterEQS()
{
	if (!IsValid(Leader))
	{
		return;
	}

	int32 ActiveCount = 0;
	for (const FPBFollowerState& State : Followers)
	{
		if (!State.bUnlinked && IsValid(State.AIC))
		{
			ActiveCount++;
		}
	}

	if (ActiveCount == 0)
	{
		return;
	}

	if (!IsValid(ScatterEQSQuery))
	{
		// EQS 에셋 미설정 시 C++ NavMesh 원형 후보로 Fallback
		OnScatterEQSFinished(nullptr);
		return;
	}

	// 비동기 EQS 쿼리 실행 (리더를 Querier로 사용)
	FEnvQueryRequest QueryRequest(ScatterEQSQuery, Leader);
	PendingEQSQueryID = QueryRequest.Execute(
		EEnvQueryRunMode::AllMatching,
		this,
		&UPBPartyFollowSubsystem::OnScatterEQSFinished
	);
}

void UPBPartyFollowSubsystem::OnScatterEQSFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (!IsValid(Leader))
	{
		return;
	}

	const FVector LeaderLoc = Leader->GetActorLocation();
	TArray<FVector> Candidates;

	if (Result.IsValid() && Result->IsSuccessful())
	{
		// EQS 결과에서 위치 배열 추출 (Score 내림차순으로 이미 정렬됨)
		Result->GetAllAsLocations(Candidates);
	}
	else
	{
		// EQS 실패 또는 미설정 — NavMesh 원형 후보 C++ 계산
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		const int32 CandidateCount = FMath::Max(Followers.Num() * 6, 16);
		const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();

		for (int32 i = 0; i < CandidateCount; ++i)
		{
			const float Angle = (2.f * PI * i) / CandidateCount;
			const float Radius = FMath::RandRange(Settings->ScatterInnerRadius, Settings->ScatterRadius);
			FVector Candidate = LeaderLoc + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);

			// NavMesh 위에 투영
			FNavLocation ProjectedLoc;
			if (IsValid(NavSys) && NavSys->ProjectPointToNavigation(Candidate, ProjectedLoc, FVector(50.f, 50.f, 200.f)))
			{
				Candidates.Add(ProjectedLoc.Location);
			}
		}
	}

	PendingEQSQueryID = INDEX_NONE;

	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	// 활성 팔로워 판별 및 이미 가까운 팔로워 정지 처리
	int32 ActiveCount = 0;
	TArray<int32> FollowersToScatter;
	FollowersToScatter.Reserve(Followers.Num());

	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		FPBFollowerState& State = Followers[i];
		if (!State.bUnlinked && IsValid(State.AIC) && IsValid(State.Character))
		{
			float DistToLeaderSq = FVector::DistSquared(State.Character->GetActorLocation(), LeaderLoc);
			
			// 충분히 가깝되, 너무 바짝 붙어있지 않은 경우 제자리 정지
			if (DistToLeaderSq <= FMath::Square(Settings->ScatterRadius) &&
				DistToLeaderSq >= FMath::Square(Settings->ScatterInnerRadius))
			{
				State.ConsecutiveFailCount = 0;
				State.AIC->StopFollowMove();
			}
			else
			{
				ActiveCount++;
				FollowersToScatter.Add(i);
			}
		}
	}

	// 모두가 이미 좋은 위치에 있다면 스캐터 불필요
	if (ActiveCount == 0 || Candidates.IsEmpty())
	{
		return;
	}

	// 이미 위치가 좋은 팔로워는 현재 위치를 점유 지점으로 간주해 간격 제약에 반영
	TArray<FVector> OccupiedPositions;
	OccupiedPositions.Reserve(Followers.Num());
	TSet<int32> FollowersToScatterSet;
	for (int32 FollowerIndex : FollowersToScatter)
	{
		FollowersToScatterSet.Add(FollowerIndex);
	}

	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		const FPBFollowerState& State = Followers[i];
		if (State.bUnlinked || !IsValid(State.Character))
		{
			continue;
		}

		if (!FollowersToScatterSet.Contains(i))
		{
			OccupiedPositions.Add(State.Character->GetActorLocation());
		}
	}

	TSet<int32> UsedCandidateIndices;
	for (int32 FollowerIdx : FollowersToScatter)
	{
		FPBFollowerState& State = Followers[FollowerIdx];
		int32 PickedCandidateIndex = INDEX_NONE;
		if (!TryPickScatterPointForFollower(State, Candidates, UsedCandidateIndices, OccupiedPositions, PickedCandidateIndex))
		{
			continue;
		}

		UsedCandidateIndices.Add(PickedCandidateIndex);
		OccupiedPositions.Add(Candidates[PickedCandidateIndex]);

		State.ConsecutiveFailCount = 0;
		State.AIC->MoveToScatterPosition(Candidates[PickedCandidateIndex]);
	}
}

void UPBPartyFollowSubsystem::RedispatchScatterForFollower(int32 FollowerIndex)
{
	FPBFollowerState& State = Followers[FollowerIndex];
	if (!IsValid(State.AIC) || !IsValid(Leader))
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	const FVector LeaderLoc = Leader->GetActorLocation();
	TArray<FVector> Candidates;
	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();

	// 12개 후보로 재계산
	for (int32 i = 0; i < 12; ++i)
	{
		const float Angle = (2.f * PI * i) / 12;
		const float Radius = FMath::RandRange(Settings->ScatterInnerRadius, Settings->ScatterRadius);
		FVector Candidate = LeaderLoc + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);

		FNavLocation ProjectedLoc;
		if (IsValid(NavSys) && NavSys->ProjectPointToNavigation(Candidate, ProjectedLoc, FVector(50.f, 50.f, 200.f)))
		{
			Candidates.Add(ProjectedLoc.Location);
		}
	}

	TSet<int32> UsedCandidateIndices;
	TArray<FVector> OccupiedPositions;
	OccupiedPositions.Reserve(Followers.Num());

	for (int32 i = 0; i < Followers.Num(); ++i)
	{
		if (i == FollowerIndex)
		{
			continue;
		}

		const FPBFollowerState& OtherState = Followers[i];
		if (!OtherState.bUnlinked && IsValid(OtherState.Character))
		{
			OccupiedPositions.Add(OtherState.Character->GetActorLocation());
		}
	}

	int32 PickedCandidateIndex = INDEX_NONE;
	if (TryPickScatterPointForFollower(State, Candidates, UsedCandidateIndices, OccupiedPositions, PickedCandidateIndex))
	{
		State.AIC->MoveToScatterPosition(Candidates[PickedCandidateIndex]);
	}
}

bool UPBPartyFollowSubsystem::IsValidScatterCandidate(const FVector& Candidate, const TArray<FVector>& OccupiedPositions, float MinSpacing) const
{
	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	if (!IsValid(Leader))
	{
		return false;
	}

	const float DistToLeader = FVector::Dist(Candidate, Leader->GetActorLocation());
	if (DistToLeader < Settings->ScatterInnerRadius || DistToLeader > Settings->ScatterRadius)
	{
		return false;
	}

	for (const FVector& Occupied : OccupiedPositions)
	{
		if (FVector::DistSquared(Candidate, Occupied) < FMath::Square(MinSpacing))
		{
			return false;
		}
	}

	return true;
}

bool UPBPartyFollowSubsystem::TryPickScatterPointForFollower(const FPBFollowerState& FollowerState, const TArray<FVector>& Candidates, const TSet<int32>& UsedCandidateIndices, const TArray<FVector>& OccupiedPositions, int32& OutCandidateIndex) const
{
	OutCandidateIndex = INDEX_NONE;
	if (!IsValid(Leader) || !IsValid(FollowerState.Character))
	{
		return false;
	}

	const UPBPartyFollowSettings* Settings = GetDefault<UPBPartyFollowSettings>();
	const FVector FollowerLoc = FollowerState.Character->GetActorLocation();
	float BestInnerDelta = TNumericLimits<float>::Max();
	float BestDistToFollowerSq = TNumericLimits<float>::Max();

	for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		if (UsedCandidateIndices.Contains(CandidateIndex))
		{
			continue;
		}

		const FVector& Candidate = Candidates[CandidateIndex];
		if (!IsValidScatterCandidate(Candidate, OccupiedPositions, Settings->ScatterMinSpacing))
		{
			continue;
		}

		const float DistToLeader = FVector::Dist(Candidate, Leader->GetActorLocation());
		const float InnerDelta = FMath::Abs(DistToLeader - Settings->ScatterInnerRadius);
		const float DistToFollowerSq = FVector::DistSquared(Candidate, FollowerLoc);

		if (InnerDelta < BestInnerDelta ||
			(FMath::IsNearlyEqual(InnerDelta, BestInnerDelta) && DistToFollowerSq < BestDistToFollowerSq))
		{
			BestInnerDelta = InnerDelta;
			BestDistToFollowerSq = DistToFollowerSq;
			OutCandidateIndex = CandidateIndex;
		}
	}

	if (OutCandidateIndex != INDEX_NONE)
	{
		return true;
	}

	// 간격 제약을 만족하는 후보가 없으면, InnerRadius 근접 + 자기 최근접 기준으로 완화 선택
	for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		if (UsedCandidateIndices.Contains(CandidateIndex))
		{
			continue;
		}

		const FVector& Candidate = Candidates[CandidateIndex];
		const float DistToLeader = FVector::Dist(Candidate, Leader->GetActorLocation());
		if (DistToLeader < Settings->ScatterInnerRadius || DistToLeader > Settings->ScatterRadius)
		{
			continue;
		}

		const float InnerDelta = FMath::Abs(DistToLeader - Settings->ScatterInnerRadius);
		const float DistToFollowerSq = FVector::DistSquared(Candidate, FollowerLoc);

		if (InnerDelta < BestInnerDelta ||
			(FMath::IsNearlyEqual(InnerDelta, BestInnerDelta) && DistToFollowerSq < BestDistToFollowerSq))
		{
			BestInnerDelta = InnerDelta;
			BestDistToFollowerSq = DistToFollowerSq;
			OutCandidateIndex = CandidateIndex;
		}
	}

	return OutCandidateIndex != INDEX_NONE;
}

bool UPBPartyFollowSubsystem::IsPointReachable(const FVector& From, const FVector& To) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!IsValid(NavSys))
	{
		return true;
	}

	const ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
	if (!IsValid(NavData))
	{
		return true;
	}

	FPathFindingQuery Query(nullptr, *NavData, From, To);
	return NavSys->TestPathSync(Query);
}

void UPBPartyFollowSubsystem::SetPhase(EPBPartyFollowPhase NewPhase)
{
	CurrentPhase = NewPhase;
}

void UPBPartyFollowSubsystem::OnCombatStateChanged(EPBCombatState NewState)
{
	SetCombatLocked(NewState != EPBCombatState::OutOfCombat);
}
