// PBGenerateSequenceTask.cpp

#include "PBGenerateSequenceTask.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "PBUtilityClearinghouse.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"
#include "StateTreeExecutionContext.h"
#include "VisualLogger/VisualLogger.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTree, Log, All);

UPBGenerateSequenceTask::UPBGenerateSequenceTask(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// EQS 타임아웃 체크를 위해 Tick 활성화
	bShouldCallTick = true;
}

/*~ EQS 상태 관리 ~*/

void UPBGenerateSequenceTask::ResetEQSState()
{
	bWaitingForEQS = false;
	PendingEQSQueryCount = 0;
	EQSTimeoutRemaining = 0.0f;
	PendingAttackMoveIndices.Empty();
	CurrentAttackMoveIndex = INDEX_NONE;
	FallbackMoveActionIndex = INDEX_NONE;
}

/*~ Move 분리 헬퍼 (Phase 3) ~*/

void UPBGenerateSequenceTask::InjectMoveActions(FPBActionSequence& Sequence)
{
	// DFS 결과에서 MovementCost > 0인 행동 앞에 물리적 Move 노드를 삽입한다.
	// 역순 순회하여 삽입 시 인덱스가 밀리지 않도록 한다.
	for (int32 i = Sequence.Actions.Num() - 1; i >= 0; --i)
	{
		FPBSequenceAction& Action = Sequence.Actions[i];
		if (Action.Cost.MovementCost <= 0.0f)
		{
			continue;
		}

		// 이동 노드 생성: 타겟 방향으로의 이동
		FPBSequenceAction MoveAction;
		MoveAction.ActionType = EPBActionType::Move;
		MoveAction.TargetActor = Action.TargetActor;
		MoveAction.TargetLocation = IsValid(Action.TargetActor)
			? Action.TargetActor->GetActorLocation()
			: FVector::ZeroVector;
		MoveAction.Cost.MovementCost = Action.Cost.MovementCost;

		// 원래 행동에서 MovementCost 제거 (이동 비용은 Move 노드가 담당)
		Action.Cost.MovementCost = 0.0f;

		// Move 노드를 행동 바로 앞에 삽입
		Sequence.Actions.Insert(MoveAction, i);

		UE_LOG(LogPBStateTree, Display,
			TEXT("[InjectMove] 행동 [%d] 앞에 Move 노드 삽입 "
				"(MP=%.0f, Target=%s)"),
			i,
			MoveAction.Cost.MovementCost,
			IsValid(MoveAction.TargetActor)
				? *MoveAction.TargetActor->GetName()
				: TEXT("None"));
	}
}

/*~ EQS 쿼리 발사 (공통 헬퍼) ~*/

void UPBGenerateSequenceTask::LaunchEQSQueries()
{
	// 시퀀스 내 Move 행동이 있고 쿼리 에셋이 할당되어 있으면,
	// EQS 비동기 쿼리로 DFS/Fallback 좌표를 지형/엄폐 기반 최적 위치로 보정한다.
	for (int32 i = 0; i < GeneratedSequence.Actions.Num(); ++i)
	{
		const FPBSequenceAction& Action = GeneratedSequence.Actions[i];
		if (Action.ActionType != EPBActionType::Move)
		{
			continue;
		}

		if (IsValid(Action.TargetActor) && IsValid(AttackPositionQuery))
		{
			// 타겟 접근 이동 → 큐에 적재 (직렬 실행)
			PendingAttackMoveIndices.Add(i);
		}
		else if (!IsValid(Action.TargetActor) && IsValid(FallbackPositionQuery))
		{
			// Fallback 후퇴 이동 → EQS로 최적 후퇴 위치 탐색
			FallbackMoveActionIndex = i;
			++PendingEQSQueryCount;

			CachedClearinghouse->RunFallbackPositionQuery(
				FallbackPositionQuery, SelfActor,
				FPBEQSQueryFinished::CreateUObject(
					this,
					&UPBGenerateSequenceTask::OnFallbackPositionQueryFinished));
		}
	}

	// Attack 큐에서 첫 번째를 발사 (직렬 실행 — 파라미터 오염 방지)
	if (PendingAttackMoveIndices.Num() > 0)
	{
		++PendingEQSQueryCount; // Attack 직렬 전체를 1개 슬롯으로 카운트
		LaunchNextAttackQuery();
	}

	if (PendingEQSQueryCount > 0)
	{
		bWaitingForEQS = true;
		EQSTimeoutRemaining = UPBUtilityClearinghouse::EQSQueryTimeoutSeconds;
		GeneratedSequence.bIsReady = false; // Execute 대기

		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS] EQS 쿼리 시작 (Attack 큐: %d, Fallback: %s). "
			     "시퀀스 준비 대기 중..."),
			PendingAttackMoveIndices.Num(),
			FallbackMoveActionIndex != INDEX_NONE ? TEXT("Y") : TEXT("N"));
	}
}

/*~ Attack EQS 직렬 실행 헬퍼 ~*/

void UPBGenerateSequenceTask::LaunchNextAttackQuery()
{
	// 큐에서 다음 인덱스를 꺼내 단일 Attack EQS 쿼리를 발사한다.
	// Clearinghouse의 EQS 파라미터(TargetActor/MaxRange/IdealDist)가
	// 단일 변수이므로, 한 번에 하나씩만 실행해야 오염을 방지할 수 있다.
	if (PendingAttackMoveIndices.Num() == 0)
	{
		return;
	}

	// 큐 선두를 꺼내 현재 처리 대상으로 설정
	CurrentAttackMoveIndex = PendingAttackMoveIndices[0];
	PendingAttackMoveIndices.RemoveAt(0);

	const FPBSequenceAction& MoveAction =
		GeneratedSequence.Actions[CurrentAttackMoveIndex];

	// Move 다음 행동의 어빌리티 CDO에서 사거리 + 이상적 교전 거리 추출
	float AbilityMaxRange = 0.f;
	float IdealDistance = 0.f;
	static constexpr float MeleeRangeThreshold = 300.f;
	static constexpr float MeleeIdealDistance = 100.f;
	static constexpr float RangedIdealRatio = 0.85f;

	const int32 NextIdx = CurrentAttackMoveIndex + 1;
	if (NextIdx < GeneratedSequence.Actions.Num())
	{
		const FPBSequenceAction& NextAction =
			GeneratedSequence.Actions[NextIdx];
		UAbilitySystemComponent* ASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
				SelfActor);
		if (IsValid(ASC))
		{
			const FGameplayAbilitySpec* Spec =
				ASC->FindAbilitySpecFromHandle(
					NextAction.AbilitySpecHandle);
			if (Spec)
			{
				const UPBGameplayAbility_Targeted* TargetedCDO =
					Cast<UPBGameplayAbility_Targeted>(Spec->Ability);
				if (TargetedCDO)
				{
					AbilityMaxRange = TargetedCDO->GetRange();

					// 이상적 교전 거리 계산
					if (AbilityMaxRange <= 0.f)
					{
						static constexpr float UnlimitedIdealDistance = 800.f;
						IdealDistance = UnlimitedIdealDistance;
					}
					else if (AbilityMaxRange <= MeleeRangeThreshold)
					{
						IdealDistance = MeleeIdealDistance;
					}
					else
					{
						IdealDistance = AbilityMaxRange * RangedIdealRatio;
					}
				}
			}
		}
	}

	UE_LOG(LogPBStateTree, Display,
		TEXT("[EQS] Attack 쿼리 발사: Move[%d] → 타겟(%s), "
		     "MaxRange=%.0f, IdealDist=%.0f (큐 잔여: %d)"),
		CurrentAttackMoveIndex,
		*GetNameSafe(MoveAction.TargetActor),
		AbilityMaxRange, IdealDistance,
		PendingAttackMoveIndices.Num());

	CachedClearinghouse->RunAttackPositionQuery(
		AttackPositionQuery, SelfActor, MoveAction.TargetActor,
		AbilityMaxRange,
		IdealDistance,
		FPBEQSQueryFinished::CreateUObject(
			this,
			&UPBGenerateSequenceTask::OnAttackPositionQueryFinished));
}

/*~ 상태 진입 실행 로직 ~*/		

EStateTreeRunStatus UPBGenerateSequenceTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition)
{
	// 1. 구동 주체 유효성 검증
	if (!IsValid(SelfActor))
	{
		UE_LOG(LogPBStateTree, Error,
		       TEXT("GenerateSequenceTask: SelfActor가 매핑되지 않았거나 유효하지 "
			       "않습니다."));
		return EStateTreeRunStatus::Failed;
	}

	// 2. 월드 생태계를 통한 Clearinghouse 서브시스템 획득
	UWorld* World = SelfActor->GetWorld();
	if (!IsValid(World))
	{
		return EStateTreeRunStatus::Failed;
	}

	CachedClearinghouse =
		World->GetSubsystem<UPBUtilityClearinghouse>();
	if (!IsValid(CachedClearinghouse))
	{
		UE_LOG(LogPBStateTree, Error,
		       TEXT("GenerateSequenceTask: Clearinghouse 서브시스템을 찾을 수 "
			       "없습니다."));
		return EStateTreeRunStatus::Failed;
	}

	// 3. 시퀀스 초기화 (이전 루프 잔여 데이터 제거)
	GeneratedSequence.Actions.Empty();
	GeneratedSequence.CurrentActionIndex = 0;
	GeneratedSequence.TotalUtilityScore = 0.0f;
	GeneratedSequence.bIsReady = true; // 기본: 즉시 준비 완료
	ResetEQSState();

	// 4. 어빌리티 시스템 및 턴 자원 확인
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);

	if (!IsValid(ASC))
	{
		return EStateTreeRunStatus::Failed;
	}

	const UPBTurnResourceAttributeSet* TurnResourceSet =
		ASC->GetSet<UPBTurnResourceAttributeSet>();
	float CurrentAction = 0.0f;
	float CurrentBonusAction = 0.0f;
	float CurrentMovement = 0.0f;

	if (TurnResourceSet)
	{
		CurrentAction = TurnResourceSet->GetAction();
		CurrentBonusAction = TurnResourceSet->GetBonusAction();
		CurrentMovement = TurnResourceSet->GetMovement();
	}

	UE_LOG(LogPBStateTree, Display,
	       TEXT("=== DFS 다중 행동 탐색 개시 [자원 AP: %.0f, BA: %.0f, "
		       "MP: %.0f] ==="),
	       CurrentAction, CurrentBonusAction, CurrentMovement);

	// 5. CachedActionScoreMap 채우기 (Top-K 타겟 평가)
	// GetTopKTargets 내부에서 EvaluateActionScore를 호출하여
	// CachedActionScoreMap이 채워진다 — DFS의 GetCandidateActions가 참조.
	const TArray<FPBTargetScore> TopTargets =
		CachedClearinghouse->GetTopKTargets(3);

	// 5-0.5. AoE 최적 배치 평가 (CachedAoECandidates 채우기)
	// AoE 어빌리티가 있으면 후보 중심점(적 위치 + 센트로이드)별 NetScore를 산출.
	// DFS의 GetCandidateActions에서 AoE 후보로 참조.
	CachedClearinghouse->EvaluateAoEPlacements();

	// 5-0.6. MultiTarget 최적 분배 평가 (CachedMultiTargetCandidates 채우기)
	// 매직 미사일 등 MultiTarget 어빌리티의 전수 열거(Stars & Bars) + Top-K 필터링.
	// DFS의 GetCandidateActions에서 MultiTarget 후보로 참조.
	CachedClearinghouse->EvaluateMultiTargetPlacements();

	// 5-1. CachedHealScoreMap + CachedBuffScoreMap 채우기 (아군 Heal/Buff 평가)
	// DFS의 GetCandidateActions가 Heal/Buff 후보 생성 시 참조.
	for (const TWeakObjectPtr<AActor>& WeakAlly :
	     CachedClearinghouse->GetCachedAllies())
	{
		if (WeakAlly.IsValid())
		{
			CachedClearinghouse->EvaluateHealScore(WeakAlly.Get());
			CachedClearinghouse->EvaluateBuffScore(WeakAlly.Get());
		}
	}

	// Heal/Buff 후보 존재 여부 확인 (적이 없어도 아군 지원이 있으면 DFS 실행)
	bool bHasAllySupportCandidates = false;
	auto CheckMapForCandidates = [](const TMap<AActor*, FPBTargetScore>& Map) -> bool
	{
		for (const auto& Pair : Map)
		{
			if (Pair.Value.GetActionScore() > 0.0f)
			{
				return true;
			}
		}
		return false;
	};
	bHasAllySupportCandidates =
		CheckMapForCandidates(CachedClearinghouse->GetCachedHealScores())
		|| CheckMapForCandidates(CachedClearinghouse->GetCachedBuffScores());

	if (TopTargets.IsEmpty() && !bHasAllySupportCandidates)
	{
		UE_LOG(LogPBStateTree, Warning,
		       TEXT("GenerateSequenceTask: 유효한 타겟도 아군 지원 대상도 없습니다."));

		// 타겟 없지만 이동력 남아있으면 방어적 후퇴
		// 단, 현재 위치가 이미 유리하면 Fallback 생략
		if (CurrentMovement > 10.0f && !ShouldSkipFallback(CurrentMovement))
		{
			const FVector FallbackPos =
				CachedClearinghouse->CalculateFallbackPosition(
					SelfActor, CurrentMovement);

			if (!FallbackPos.IsZero())
			{
				FPBSequenceAction FallbackAction;
				FallbackAction.ActionType = EPBActionType::Move;
				FallbackAction.TargetActor = nullptr;
				FallbackAction.TargetLocation = FallbackPos;
				FallbackAction.Cost.MovementCost = CurrentMovement;
				GeneratedSequence.Actions.Add(FallbackAction);

				UE_LOG(LogPBStateTree, Display,
				       TEXT("[Fallback] 타겟 없음, 방어적 후퇴만 실행. "
					       "목표: (%s)"),
				       *FallbackPos.ToCompactString());

				// Fallback 이동 후 잔여 AP로 단일 행동 탐색
				TryAppendActionAfterFallback(
					FallbackPos, CurrentAction, CurrentBonusAction);

				// EQS로 후퇴 위치 최적화 시도
				LaunchEQSQueries();

				return EStateTreeRunStatus::Running;
			}
		}

		return EStateTreeRunStatus::Failed;
	}

	// 6. DFS 초기 컨텍스트 구성
	FPBUtilityContext InitialContext;
	InitialContext.RemainingAP = CurrentAction;
	InitialContext.RemainingBA = CurrentBonusAction;
	InitialContext.RemainingMP = CurrentMovement;
	InitialContext.AccumulatedMP = 0.0f;
	InitialContext.LastActionLocation = SelfActor->GetActorLocation();

	// 7. DFS 탐색 실행
	TArray<FPBSequenceAction> CurrentPath;
	TArray<FPBSequenceAction> BestPath;
	float BestScore = 0.0f; // 기준선: "아무것도 안 하기"의 점수

	// B&B 가지치기용 단일 행동 최대 점수 사전 계산 (DFS 내 반복 순회 방지)
	const float MaxSingleScore = CachedClearinghouse->CalcMaxSingleScore();

	CachedClearinghouse->SearchBestSequence(
		InitialContext, CurrentPath, 0.0f,
		BestScore, BestPath, 0, MaxSingleScore);

	UE_LOG(LogPBStateTree, Display,
	       TEXT("[DFS] 탐색 완료: BestScore=%.2f, 행동 수=%d"),
	       BestScore, BestPath.Num());

	// 8. 결과 반영
	if (BestPath.Num() > 0)
	{
		GeneratedSequence.Actions = BestPath;
		GeneratedSequence.TotalUtilityScore = BestScore;

		// Phase 3: DFS 결과에서 MovementCost > 0인 행동 앞에 Move 노드 삽입
		InjectMoveActions(GeneratedSequence);

		// Fallback 후퇴 검토: 시퀀스 실행 후 잔여 이동력으로 방어적 후퇴
		float ConsumedMP = 0.0f;
		for (const FPBSequenceAction& Action : BestPath)
		{
			ConsumedMP += Action.Cost.MovementCost;
		}
		const float RemainingMPAfterSequence = CurrentMovement - ConsumedMP;

		if (RemainingMPAfterSequence > 10.0f
			&& !ShouldSkipFallback(RemainingMPAfterSequence))
		{
			const FVector FallbackPos =
				CachedClearinghouse->CalculateFallbackPosition(
					SelfActor, RemainingMPAfterSequence);

			if (!FallbackPos.IsZero())
			{
				FPBSequenceAction FallbackAction;
				FallbackAction.ActionType = EPBActionType::Move;
				FallbackAction.TargetActor = nullptr;
				FallbackAction.TargetLocation = FallbackPos;
				FallbackAction.Cost.MovementCost = RemainingMPAfterSequence;
				GeneratedSequence.Actions.Add(FallbackAction);

				UE_LOG(LogPBStateTree, Display,
				       TEXT("[Fallback] 시퀀스 후 방어적 후퇴 추가. "
					       "목표: (%s)"),
				       *FallbackPos.ToCompactString());
			}
		}
	}
	else
	{
		// DFS가 유효한 행동을 찾지 못함 → Fallback 시도
		// 단, 현재 위치가 이미 유리하면 Fallback 생략
		if (CurrentMovement > 10.0f && !ShouldSkipFallback(CurrentMovement))
		{
			const FVector FallbackPos =
				CachedClearinghouse->CalculateFallbackPosition(
					SelfActor, CurrentMovement);

			if (!FallbackPos.IsZero())
			{
				FPBSequenceAction FallbackAction;
				FallbackAction.ActionType = EPBActionType::Move;
				FallbackAction.TargetActor = nullptr;
				FallbackAction.TargetLocation = FallbackPos;
				FallbackAction.Cost.MovementCost = CurrentMovement;
				GeneratedSequence.Actions.Add(FallbackAction);

				UE_LOG(LogPBStateTree, Display,
				       TEXT("[Fallback] DFS 행동 없음, 방어적 후퇴만 실행. "
					       "목표: (%s)"),
				       *FallbackPos.ToCompactString());

				// Fallback 이동 후 잔여 AP로 단일 행동 탐색
				TryAppendActionAfterFallback(
					FallbackPos, CurrentAction, CurrentBonusAction);
			}
		}

		if (GeneratedSequence.Actions.IsEmpty())
		{
			UE_LOG(LogPBStateTree, Warning,
			       TEXT("GenerateSequenceTask: DFS 및 Fallback 모두 행동을 "
				       "생성하지 못했습니다. 턴 종료."));
			return EStateTreeRunStatus::Failed;
		}
	}

	// 9. EQS 위치 최적화 (Phase 3)
	LaunchEQSQueries();

	// 10. 최종 결과 로깅
	UE_LOG(LogPBStateTree, Display,
	       TEXT("\n============================================="));
	UE_LOG(LogPBStateTree, Display,
	       TEXT("AI [%s] 생성된 시퀀스: %d개 행동, TotalScore: %.2f%s"),
	       *SelfActor->GetName(), GeneratedSequence.Actions.Num(),
	       GeneratedSequence.TotalUtilityScore,
	       bWaitingForEQS ? TEXT(" [EQS 대기중]") : TEXT(""));

	for (int32 i = 0; i < GeneratedSequence.Actions.Num(); ++i)
	{
		const FPBSequenceAction& SeqAction = GeneratedSequence.Actions[i];
		const FString TargetName = IsValid(SeqAction.TargetActor)
			? SeqAction.TargetActor->GetName()
			: TEXT("위치이동");
		const TCHAR* TypeStr =
			SeqAction.ActionType == EPBActionType::Attack  ? TEXT("Attack") :
			SeqAction.ActionType == EPBActionType::Move    ? TEXT("Move") :
			SeqAction.ActionType == EPBActionType::Heal    ? TEXT("Heal") :
			SeqAction.ActionType == EPBActionType::Buff    ? TEXT("Buff") :
			SeqAction.ActionType == EPBActionType::Debuff  ? TEXT("Debuff") :
			SeqAction.ActionType == EPBActionType::Control ? TEXT("Control") :
			TEXT("Other");

		UE_LOG(LogPBStateTree, Display,
		       TEXT("  [%d] %s → %s (AP=%.0f, MP=%.0f)"),
		       i + 1, TypeStr, *TargetName,
		       SeqAction.Cost.ActionCost, SeqAction.Cost.MovementCost);
	}

	UE_LOG(LogPBStateTree, Display,
	       TEXT("============================================="));

	// Visual Logger: 최종 시퀀스 타임라인 기록
	{
		FString SeqSummary;
		for (int32 i = 0; i < GeneratedSequence.Actions.Num(); ++i)
		{
			const FPBSequenceAction& A = GeneratedSequence.Actions[i];
			const TCHAR* T =
				A.ActionType == EPBActionType::Attack  ? TEXT("Atk") :
				A.ActionType == EPBActionType::Move    ? TEXT("Mov") :
				A.ActionType == EPBActionType::Heal    ? TEXT("Heal") :
				TEXT("Other");
			const FString TgtName = IsValid(A.TargetActor)
				? A.TargetActor->GetName() : TEXT("Pos");
			SeqSummary += FString::Printf(TEXT("[%d]%s→%s "), i, T, *TgtName);
		}
		UE_VLOG(SelfActor, LogPBStateTree, Log,
			TEXT("[Generate] Score=%.2f %s%s"),
			GeneratedSequence.TotalUtilityScore, *SeqSummary,
			bWaitingForEQS ? TEXT("[EQS pending]") : TEXT("[Ready]"));
	}

	// StateTree 하위 State(Execute)가 유지되도록 Running 반환
	// (Succeeded를 반환하면 하위 State가 즉시 강제 종료됨)
	return EStateTreeRunStatus::Running;
}

/*~ Tick: EQS 타임아웃 감시 ~*/

EStateTreeRunStatus UPBGenerateSequenceTask::Tick(
	FStateTreeExecutionContext& Context, const float DeltaTime)
{
	if (bWaitingForEQS)
	{
		EQSTimeoutRemaining -= DeltaTime;

		if (EQSTimeoutRemaining <= 0.0f)
		{
			UE_LOG(LogPBStateTree, Warning,
				TEXT("[EQS] 타임아웃 (%.1f초 초과). "
				     "기존 DFS 좌표로 즉시 진행합니다."),
				UPBUtilityClearinghouse::EQSQueryTimeoutSeconds);

			// EQS 결과 무시하고 기존 좌표로 진행
			bWaitingForEQS = false;
			PendingEQSQueryCount = 0;
			GeneratedSequence.bIsReady = true;
		}
	}

	// Generate는 항상 Running 유지 (하위 Execute State가 동작하도록)
	return EStateTreeRunStatus::Running;
}

/*~ 상태 퇴장: EQS 정리 ~*/

void UPBGenerateSequenceTask::ExitState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition)
{
	// 진행 중인 EQS 대기 상태 정리
	// (쿼리 자체는 Clearinghouse에서 완료되지만 콜백은 무시됨)
	ResetEQSState();
	CachedClearinghouse = nullptr;
}

/*~ EQS 콜백 핸들러 ~*/

void UPBGenerateSequenceTask::OnAttackPositionQueryFinished(
	bool bSuccess, const FVector& Location)
{
	if (!bWaitingForEQS)
	{
		// 타임아웃으로 이미 진행됨 — 늦은 콜백 무시
		return;
	}

	if (bSuccess
		&& CurrentAttackMoveIndex != INDEX_NONE
		&& GeneratedSequence.Actions.IsValidIndex(CurrentAttackMoveIndex))
	{
		// ─── 경로거리 검증: EQS 위치까지 실제 도달 가능한가? ───
		// PathExist 필터는 경로 "존재"만 확인 — 경로 "길이"가 잔여 MP 내인지는 미확인.
		// MP 부족 시 클램핑 위치에서 LoS 사전 검증 → 실패 시 Move+Attack 무효화.
		bool bPositionValid = true;
		FVector FinalDestination = Location;

		if (IsValid(SelfActor) && IsValid(SelfActor->GetWorld()))
		{
			const UGameInstance* GI = SelfActor->GetWorld()->GetGameInstance();
			const UPBEnvironmentSubsystem* EnvSub = GI
				? GI->GetSubsystem<UPBEnvironmentSubsystem>() : nullptr;
			UAbilitySystemComponent* ASC =
				UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
					SelfActor);

			if (EnvSub && ASC)
			{
				bool bMPFound = false;
				const float RemainingMP = ASC->GetGameplayAttributeValue(
					UPBTurnResourceAttributeSet::GetMovementAttribute(),
					bMPFound);

				if (bMPFound && RemainingMP > 0.f)
				{
					const FPBPathCostResult PathResult =
						EnvSub->CalculatePathCost(
							SelfActor->GetActorLocation(), Location);

					if (PathResult.bIsValid
						&& PathResult.TotalCost > RemainingMP)
					{
						// MP 부족 → 클램핑 위치 산출
						TArray<FVector> ClampedPath;
						FinalDestination =
							EnvSub->CalculateClampedDestination(
								PathResult.PathPoints, RemainingMP,
								ClampedPath);

						// 클램핑 위치에서 타겟까지 LoS 확인
						const int32 AttackIdx = CurrentAttackMoveIndex + 1;
						AActor* TargetActor = nullptr;
						if (GeneratedSequence.Actions.IsValidIndex(AttackIdx))
						{
							TargetActor =
								GeneratedSequence.Actions[AttackIdx]
									.TargetActor;
						}

						if (IsValid(TargetActor))
						{
							const FPBLoSResult LoS =
								EnvSub->CheckLineOfSight(
									FinalDestination, TargetActor);

							if (!LoS.bHasLineOfSight)
							{
								bPositionValid = false;
								UE_LOG(LogPBStateTree, Warning,
									TEXT("[EQS] 경로거리(%.0f) > "
									     "잔여MP(%.0f) → "
									     "클램핑 위치(%s)에서 "
									     "타겟(%s) LoS 없음. "
									     "Move+Attack 무효화."),
									PathResult.TotalCost, RemainingMP,
									*FinalDestination.ToCompactString(),
									*TargetActor->GetName());
							}
							else
							{
								UE_LOG(LogPBStateTree, Display,
									TEXT("[EQS] 경로거리(%.0f) > "
									     "잔여MP(%.0f) → "
									     "클램핑 위치(%s)에서 LoS 확보. "
									     "클램핑 좌표로 이동."),
									PathResult.TotalCost, RemainingMP,
									*FinalDestination.ToCompactString());
							}
						}
					}
				}
			}
		}

		if (bPositionValid)
		{
			// EQS 최적 위치 (또는 MP 클램핑 좌표)로 Move 좌표 교체
			GeneratedSequence.Actions[CurrentAttackMoveIndex].TargetLocation =
				FinalDestination;

			UE_LOG(LogPBStateTree, Display,
				TEXT("[EQS] Attack Move [%d] 좌표 교체 완료: (%s)%s"),
				CurrentAttackMoveIndex + 1,
				*FinalDestination.ToCompactString(),
				(FinalDestination != Location)
					? TEXT(" (MP 클램핑 적용)")
					: TEXT(""));
		}
		else
		{
			// 클램핑 위치에서 LoS 불가 → Move+Attack 무효화
			if (GeneratedSequence.Actions.IsValidIndex(CurrentAttackMoveIndex))
			{
				GeneratedSequence.Actions[CurrentAttackMoveIndex].ActionType =
					EPBActionType::None;
			}
			const int32 AttackIdx = CurrentAttackMoveIndex + 1;
			if (GeneratedSequence.Actions.IsValidIndex(AttackIdx))
			{
				GeneratedSequence.Actions[AttackIdx].ActionType =
					EPBActionType::None;
			}

			// 대안 탐색: 현재 위치에서 다른 타겟에게 행동 가능한지 확인
			TryReplaceInvalidatedActions(CurrentAttackMoveIndex, AttackIdx);
		}
	}
	else
	{
		// EQS 실패 → LoS 확보 위치를 찾지 못함
		// Move+Attack 무효화 (벽 뒤 타겟에게 맹목적으로 전진 방지)
		if (CurrentAttackMoveIndex != INDEX_NONE)
		{
			if (GeneratedSequence.Actions.IsValidIndex(CurrentAttackMoveIndex))
			{
				GeneratedSequence.Actions[CurrentAttackMoveIndex].ActionType =
					EPBActionType::None;
			}
			const int32 AttackIdx = CurrentAttackMoveIndex + 1;
			if (GeneratedSequence.Actions.IsValidIndex(AttackIdx))
			{
				GeneratedSequence.Actions[AttackIdx].ActionType =
					EPBActionType::None;
			}

			// 대안 탐색: 현재 위치에서 다른 타겟에게 행동 가능한지 확인
			TryReplaceInvalidatedActions(CurrentAttackMoveIndex, AttackIdx);
		}

		UE_LOG(LogPBStateTree, Warning,
			TEXT("[EQS] Attack Position 쿼리 실패 — "
			     "Move[%d]+Attack[%d] 무효화 (벽 뒤 맹목 전진 방지)"),
			CurrentAttackMoveIndex + 1, CurrentAttackMoveIndex + 2);
	}

	// 큐에 남은 Attack 쿼리가 있으면 다음 발사, 없으면 카운트 감소
	CurrentAttackMoveIndex = INDEX_NONE;
	if (PendingAttackMoveIndices.Num() > 0)
	{
		LaunchNextAttackQuery();
	}
	else
	{
		// Attack 직렬 전체 완료 → 슬롯 해제
		--PendingEQSQueryCount;
		CheckAllEQSComplete();
	}
}

void UPBGenerateSequenceTask::OnFallbackPositionQueryFinished(
	bool bSuccess, const FVector& Location)
{
	if (!bWaitingForEQS)
	{
		return;
	}

	if (bSuccess
		&& FallbackMoveActionIndex != INDEX_NONE
		&& GeneratedSequence.Actions.IsValidIndex(FallbackMoveActionIndex))
	{
		// EQS 최적 위치로 Fallback 좌표 교체
		GeneratedSequence.Actions[FallbackMoveActionIndex].TargetLocation =
			Location;

		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS] Fallback Move [%d] 좌표 교체 완료: (%s)"),
			FallbackMoveActionIndex + 1, *Location.ToCompactString());
	}
	else
	{
		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS] Fallback Position 쿼리 실패/무효. "
			     "기존 CalculateFallbackPosition 좌표를 유지합니다."));
	}

	FallbackMoveActionIndex = INDEX_NONE;
	--PendingEQSQueryCount;
	CheckAllEQSComplete();
}

void UPBGenerateSequenceTask::CheckAllEQSComplete()
{
	if (PendingEQSQueryCount <= 0)
	{
		bWaitingForEQS = false;
		GeneratedSequence.bIsReady = true;

		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS] 모든 EQS 쿼리 완료. 시퀀스 준비 완료."));

		// Visual Logger: EQS 완료 후 최종 좌표 기록
		for (int32 i = 0; i < GeneratedSequence.Actions.Num(); ++i)
		{
			const FPBSequenceAction& A = GeneratedSequence.Actions[i];
			if (A.ActionType == EPBActionType::Move && !A.TargetLocation.IsZero())
			{
				UE_VLOG_LOCATION(SelfActor, LogPBStateTree, Log,
					A.TargetLocation, 40.0f, FColor::Green,
					TEXT("EQS Move[%d]"), i);
			}
		}
	}
}

/*~ Fallback 헬퍼 ~*/

void UPBGenerateSequenceTask::TryAppendActionAfterFallback(
	const FVector& FallbackPos, float RemainingAP, float RemainingBA)
{
	// Fallback 이동 후 잔여 AP가 있으면, 새 위치 기준으로 단일 행동 탐색
	if (RemainingAP < 1.0f || !IsValid(CachedClearinghouse))
	{
		return;
	}

	// Fallback 위치 기준 컨텍스트 구성 (이동력 0 = 추가 이동 불허)
	FPBUtilityContext PostFallbackCtx;
	PostFallbackCtx.RemainingAP = RemainingAP;
	PostFallbackCtx.RemainingBA = RemainingBA;
	PostFallbackCtx.RemainingMP = 0.0f;
	PostFallbackCtx.AccumulatedMP = 0.0f;
	PostFallbackCtx.LastActionLocation = FallbackPos;

	// 후보 행동 생성 (사거리 내 Attack/Heal만, Move는 MP=0이라 불가)
	TArray<FPBSequenceAction> Candidates =
		CachedClearinghouse->GetCandidateActions(PostFallbackCtx);

	if (Candidates.IsEmpty())
	{
		UE_LOG(LogPBStateTree, Display,
			TEXT("[Fallback+Action] Fallback 위치에서 실행 가능한 행동 없음."));
		return;
	}

	// 최고 점수 행동 선택
	FPBSequenceAction* BestAction = nullptr;
	float BestActionScore = -1.0f;

	for (FPBSequenceAction& Candidate : Candidates)
	{
		// Move는 스킵 (Fallback 후 추가 이동 무의미)
		if (Candidate.ActionType == EPBActionType::Move)
		{
			continue;
		}

		float CandidateScore = 0.0f;
		if (Candidate.ActionType == EPBActionType::Attack && IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* ScoreData =
					CachedClearinghouse->GetCachedActionScores().Find(Candidate.TargetActor))
			{
				CandidateScore = ScoreData->GetActionScore();
			}
		}
		else if (Candidate.ActionType == EPBActionType::Heal && IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* HealData =
					CachedClearinghouse->GetCachedHealScores().Find(Candidate.TargetActor))
			{
				CandidateScore = HealData->GetActionScore();
			}
		}
		else if (Candidate.ActionType == EPBActionType::Buff && IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* BuffData =
					CachedClearinghouse->GetCachedBuffScores().Find(Candidate.TargetActor))
			{
				CandidateScore = BuffData->GetActionScore();
			}
		}
		else if ((Candidate.ActionType == EPBActionType::Debuff
				|| Candidate.ActionType == EPBActionType::Control)
			&& IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* DebuffData =
					CachedClearinghouse->GetCachedDebuffScores().Find(Candidate.TargetActor))
			{
				CandidateScore = DebuffData->GetActionScore();
			}
			if (const FPBTargetScore* ControlData =
					CachedClearinghouse->GetCachedControlScores().Find(Candidate.TargetActor))
			{
				CandidateScore = FMath::Max(CandidateScore, ControlData->GetActionScore());
			}
		}

		if (CandidateScore > BestActionScore)
		{
			BestActionScore = CandidateScore;
			BestAction = &Candidate;
		}
	}

	if (BestAction && BestActionScore > 0.0f)
	{
		GeneratedSequence.Actions.Add(*BestAction);

		const TCHAR* TypeStr =
			BestAction->ActionType == EPBActionType::Attack ? TEXT("Attack") :
			BestAction->ActionType == EPBActionType::Heal   ? TEXT("Heal") :
			TEXT("Other");

		UE_LOG(LogPBStateTree, Display,
			TEXT("[Fallback+Action] Fallback 후 %s 추가 (타겟: %s, Score: %.2f)"),
			TypeStr,
			IsValid(BestAction->TargetActor) ? *BestAction->TargetActor->GetName() : TEXT("N/A"),
			BestActionScore);
	}
}

bool UPBGenerateSequenceTask::ShouldSkipFallback(float RemainingMP) const
{
	// 현재 위치가 이미 전술적으로 유리한지 판정
	// 조건: (1) 사거리 내 적 존재 + (2) 잔여 이동력이 최대의 30% 미만
	// → 조금 움직여봤자 큰 이득이 없고, 이미 교전 가능한 위치
	if (!IsValid(SelfActor) || !IsValid(CachedClearinghouse))
	{
		return false;
	}

	// 잔여 이동력이 최대 이동력의 30% 이상이면 Fallback 가치 있음
	const float MaxMovement = CachedClearinghouse->GetCachedMaxMovement();
	if (MaxMovement > 0.0f && (RemainingMP / MaxMovement) >= 0.3f)
	{
		return false;
	}

	// 현재 위치에서 사거리 내 적이 있는지 확인
	const FVector CurrentPos = SelfActor->GetActorLocation();
	bool bHasTargetInRange = false;

	for (const auto& Pair : CachedClearinghouse->GetCachedActionScores())
	{
		if (IsValid(Pair.Key))
		{
			const float Dist = FVector::Dist(CurrentPos, Pair.Key->GetActorLocation());
			// 기본 근접 사거리(200cm) 이내에 적이 있으면 교전 가능
			if (Dist <= 200.0f)
			{
				bHasTargetInRange = true;
				break;
			}
		}
	}

	if (bHasTargetInRange)
	{
		UE_LOG(LogPBStateTree, Display,
			TEXT("[Fallback Skip] 현재 위치에 사거리 내 적 존재 + "
			     "잔여 MP(%.0f)가 최대(%.0f)의 30%% 미만 → Fallback 생략"),
			RemainingMP, MaxMovement);
		return true;
	}

	return false;
}

void UPBGenerateSequenceTask::TryReplaceInvalidatedActions(
	int32 InvalidatedMoveIdx, int32 InvalidatedAttackIdx)
{
	// EQS 후검증에서 Move+Attack이 무효화(None)된 경우,
	// 캐시된 스코어 맵에서 현재 위치 기준으로 대안 행동을 탐색한다.
	// 이동 없이(MP=0) 사거리 내 타겟에게 실행 가능한 행동이 있으면 교체.

	if (!IsValid(SelfActor) || !IsValid(CachedClearinghouse))
	{
		return;
	}

	if (!GeneratedSequence.Actions.IsValidIndex(InvalidatedAttackIdx))
	{
		return;
	}

	// 원래 Attack 행동의 자원 예산 보존
	const FPBSequenceAction& OriginalAttack =
		GeneratedSequence.Actions[InvalidatedAttackIdx];
	const AActor* OriginalTarget = OriginalAttack.TargetActor;

	// 현재 위치 기준 컨텍스트 (이동력 0 = 현 위치 고정)
	FPBUtilityContext Ctx;
	Ctx.RemainingAP = OriginalAttack.Cost.ActionCost;
	Ctx.RemainingBA = OriginalAttack.Cost.BonusActionCost;
	Ctx.RemainingMP = 0.0f;
	Ctx.AccumulatedMP = 0.0f;
	Ctx.LastActionLocation = SelfActor->GetActorLocation();

	// 후보 행동 생성
	TArray<FPBSequenceAction> Candidates =
		CachedClearinghouse->GetCandidateActions(Ctx);

	if (Candidates.IsEmpty())
	{
		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS Replan] 현재 위치에서 대안 행동 없음 — None 유지."));
		return;
	}

	// 최고 점수 행동 선택
	FPBSequenceAction* BestAction = nullptr;
	float BestScore = -1.0f;

	for (FPBSequenceAction& Candidate : Candidates)
	{
		// Move 스킵 (이동은 이미 무효화됨)
		if (Candidate.ActionType == EPBActionType::Move)
		{
			continue;
		}

		// AoE 스킵 (TargetLocation 비제로 = 지점 대상, 별도 EQS 필요)
		if (!Candidate.TargetLocation.IsZero())
		{
			continue;
		}

		// MultiTarget 스킵 (복수 타겟 행동은 별도 검증 필요)
		if (Candidate.MultiTargetActors.Num() > 0)
		{
			continue;
		}

		// 동일 타겟 스킵 (이미 LoS 없음이 확인된 타겟)
		if (Candidate.TargetActor == OriginalTarget)
		{
			continue;
		}

		// 캐시된 스코어 맵에서 점수 조회
		float CandidateScore = 0.0f;
		if (Candidate.ActionType == EPBActionType::Attack
			&& IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* ScoreData =
					CachedClearinghouse->GetCachedActionScores().Find(
						Candidate.TargetActor))
			{
				CandidateScore = ScoreData->GetActionScore();
			}
		}
		else if (Candidate.ActionType == EPBActionType::Heal
			&& IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* HealData =
					CachedClearinghouse->GetCachedHealScores().Find(
						Candidate.TargetActor))
			{
				CandidateScore = HealData->GetActionScore();
			}
		}
		else if (Candidate.ActionType == EPBActionType::Buff
			&& IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* BuffData =
					CachedClearinghouse->GetCachedBuffScores().Find(
						Candidate.TargetActor))
			{
				CandidateScore = BuffData->GetActionScore();
			}
		}
		else if ((Candidate.ActionType == EPBActionType::Debuff
				|| Candidate.ActionType == EPBActionType::Control)
			&& IsValid(Candidate.TargetActor))
		{
			if (const FPBTargetScore* DebuffData =
					CachedClearinghouse->GetCachedDebuffScores().Find(
						Candidate.TargetActor))
			{
				CandidateScore = DebuffData->GetActionScore();
			}
			if (const FPBTargetScore* ControlData =
					CachedClearinghouse->GetCachedControlScores().Find(
						Candidate.TargetActor))
			{
				CandidateScore =
					FMath::Max(CandidateScore, ControlData->GetActionScore());
			}
		}

		if (CandidateScore > BestScore)
		{
			BestScore = CandidateScore;
			BestAction = &Candidate;
		}
	}

	if (BestAction && BestScore > 0.0f)
	{
		// Attack 슬롯을 대안 행동으로 교체
		GeneratedSequence.Actions[InvalidatedAttackIdx] = *BestAction;

		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS Replan] 무효화된 Attack → 대안 교체: %s (타겟: %s, "
			     "Score: %.2f)"),
			*UEnum::GetValueAsString(BestAction->ActionType),
			IsValid(BestAction->TargetActor)
				? *BestAction->TargetActor->GetName()
				: TEXT("N/A"),
			BestScore);

		// Move 슬롯은 None 유지 — 현 위치에서 행동하므로 이동 불필요.
		// Move의 Cost는 AP=0, BA=0이라 None 핸들러에서 자원 낭비 없음.
	}
	else
	{
		UE_LOG(LogPBStateTree, Display,
			TEXT("[EQS Replan] 대안 행동 없음 (Score > 0 후보 없음) — None 유지."));
	}
}
