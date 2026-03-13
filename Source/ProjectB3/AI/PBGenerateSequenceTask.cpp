// PBGenerateSequenceTask.cpp

#include "PBGenerateSequenceTask.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "PBUtilityClearinghouse.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "StateTreeExecutionContext.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTree, Log, All);

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

	UPBUtilityClearinghouse* Clearinghouse =
		World->GetSubsystem<UPBUtilityClearinghouse>();
	if (!IsValid(Clearinghouse))
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
	const TArray<FPBTargetScore> TopTargets = Clearinghouse->GetTopKTargets(3);

	// 5-1. CachedHealScoreMap 채우기 (아군 Heal 평가)
	// DFS의 GetCandidateActions가 Heal 후보 생성 시 참조.
	for (const TWeakObjectPtr<AActor>& WeakAlly : Clearinghouse->GetCachedAllies())
	{
		if (WeakAlly.IsValid())
		{
			Clearinghouse->EvaluateHealScore(WeakAlly.Get());
		}
	}

	// Heal 후보 존재 여부 확인 (적이 없어도 힐할 아군이 있으면 DFS 실행)
	bool bHasHealCandidates = false;
	for (const auto& HealPair : Clearinghouse->GetCachedHealScores())
	{
		if (HealPair.Value.GetActionScore() > 0.0f)
		{
			bHasHealCandidates = true;
			break;
		}
	}

	if (TopTargets.IsEmpty() && !bHasHealCandidates)
	{
		UE_LOG(LogPBStateTree, Warning,
		       TEXT("GenerateSequenceTask: 유효한 타겟도 힐 대상도 없습니다."));

		// 타겟 없지만 이동력 남아있으면 방어적 후퇴
		if (CurrentMovement > 10.0f)
		{
			const FVector FallbackPos =
				Clearinghouse->CalculateFallbackPosition(
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

	Clearinghouse->SearchBestSequence(
		InitialContext, CurrentPath, 0.0f,
		BestScore, BestPath, 0);

	UE_LOG(LogPBStateTree, Display,
	       TEXT("[DFS] 탐색 완료: BestScore=%.2f, 행동 수=%d"),
	       BestScore, BestPath.Num());

	// 8. 결과 반영
	if (BestPath.Num() > 0)
	{
		GeneratedSequence.Actions = BestPath;
		GeneratedSequence.TotalUtilityScore = BestScore;

		// Fallback 후퇴 검토: 시퀀스 실행 후 잔여 이동력으로 방어적 후퇴
		float ConsumedMP = 0.0f;
		for (const FPBSequenceAction& Action : BestPath)
		{
			ConsumedMP += Action.Cost.MovementCost;
		}
		const float RemainingMPAfterSequence = CurrentMovement - ConsumedMP;

		if (RemainingMPAfterSequence > 10.0f)
		{
			const FVector FallbackPos =
				Clearinghouse->CalculateFallbackPosition(
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
		if (CurrentMovement > 10.0f)
		{
			const FVector FallbackPos =
				Clearinghouse->CalculateFallbackPosition(
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

	// 9. 최종 결과 로깅
	UE_LOG(LogPBStateTree, Display,
	       TEXT("\n============================================="));
	UE_LOG(LogPBStateTree, Display,
	       TEXT("AI [%s] 생성된 시퀀스: %d개 행동, TotalScore: %.2f"),
	       *SelfActor->GetName(), GeneratedSequence.Actions.Num(),
	       GeneratedSequence.TotalUtilityScore);

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

	// StateTree 하위 State(Execute)가 유지되도록 Running 반환
	// (Succeeded를 반환하면 하위 State가 즉시 강제 종료됨)
	return EStateTreeRunStatus::Running;
}
