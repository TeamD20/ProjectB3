// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayDebuggerCategory_AI.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"
#include "GameplayDebuggerTypes.h"

#include "ProjectB3/AI/PBAITypes.h"
#include "ProjectB3/AI/PBUtilityClearinghouse.h"
#include "ProjectB3/AI/PBGenerateSequenceTask.h"
#include "ProjectB3/AI/PBExecuteSequenceTask.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Components/StateTreeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAIDebugger, Log, All);

//----------------------------------------------------------------------
// 생성자: 카테고리 이름 설정
//----------------------------------------------------------------------
FPBGameplayDebuggerCategory_AI::FPBGameplayDebuggerCategory_AI()
{
	// Tick마다 CollectData → DrawData 호출 (로컬 전용, 리플리케이션 불필요)
}

TSharedRef<FGameplayDebuggerCategory> FPBGameplayDebuggerCategory_AI::MakeInstance()
{
	return MakeShareable(new FPBGameplayDebuggerCategory_AI());
}

//----------------------------------------------------------------------
// CollectData: 디버그 대상 액터에서 AI 데이터 수집
//----------------------------------------------------------------------
void FPBGameplayDebuggerCategory_AI::CollectData(
	APlayerController* OwnerPC, AActor* DebugActor)
{
	DataPack = FRepData();

	if (!IsValid(DebugActor))
	{
		return;
	}

	DataPack.ActorName = DebugActor->GetName();

	// Clearinghouse (WorldSubsystem) 조회
	UPBUtilityClearinghouse* Clearinghouse =
		DebugActor->GetWorld()
			? DebugActor->GetWorld()->GetSubsystem<UPBUtilityClearinghouse>()
			: nullptr;

	if (Clearinghouse)
	{
		// 기본 캐시 정보
		DataPack.CachedMaxMovement = Clearinghouse->GetCachedMaxMovement();
		DataPack.NumTargets = Clearinghouse->GetCachedTargets().Num();
		DataPack.NumAllies = Clearinghouse->GetCachedAllies().Num();

		// ArchetypeWeights
		const auto& Weights = Clearinghouse->GetCachedArchetypeWeights();
		DataPack.ArchetypeWeights = FString::Printf(
			TEXT("Atk=%.1f Heal=%.1f Buff=%.1f Debuff=%.1f Ctrl=%.1f"),
			Weights.AttackWeight, Weights.HealWeight,
			Weights.BuffWeight, Weights.DebuffWeight, Weights.ControlWeight);

		// Attack 스코어링 상위 5개
		CollectScoreEntries(
			Clearinghouse->GetCachedActionScores(),
			DataPack.TopAttackScores, 5);

		// Heal 스코어링 상위 5개
		CollectScoreEntries(
			Clearinghouse->GetCachedHealScores(),
			DataPack.TopHealScores, 5);
	}

	// StateTree Task 상태 조회: AIController → StateTreeComponent → Task
	AAIController* AIC = Cast<AAIController>(
		Cast<APawn>(DebugActor) ? Cast<APawn>(DebugActor)->GetController() : nullptr);

	if (!AIC)
	{
		return;
	}

	// StateTreeComponent에서 현재 실행 중인 Task를 직접 조회할 수는 없으므로,
	// Generate/Execute Task의 public UPROPERTY를 통해 상태를 읽는다.
	// 현재 구조에서는 Task가 UObject이므로 직접 참조가 어려움.
	// 대신 Clearinghouse에서 생성한 캐시 데이터로 시퀀스 정보를 가져온다.

	// GenerateSequenceTask의 출력은 Generate→Execute 바인딩으로 넘어가므로
	// 외부에서 직접 접근이 제한적. Clearinghouse에 마지막 시퀀스를 캐싱한다.
	// (아래 DrawData에서 시퀀스 정보 표시)
}

//----------------------------------------------------------------------
// DrawData: HUD에 수집 데이터 렌더링
//----------------------------------------------------------------------
void FPBGameplayDebuggerCategory_AI::DrawData(
	APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	// 제목
	CanvasContext.Printf(TEXT("{yellow}=== AI Utility Debugger: %s ==="), *DataPack.ActorName);

	// --- 1) 캐시 상태 ---
	CanvasContext.Printf(TEXT("{white}[Cache] Targets: {cyan}%d{white}  Allies: {cyan}%d{white}  MaxMP: {cyan}%.0f cm"),
		DataPack.NumTargets, DataPack.NumAllies, DataPack.CachedMaxMovement);
	CanvasContext.Printf(TEXT("{white}[Archetype] %s"), *DataPack.ArchetypeWeights);

	// --- 2) Attack 스코어링 ---
	CanvasContext.Printf(TEXT(""));
	CanvasContext.Printf(TEXT("{green}--- Attack Scores (Top %d) ---"), DataPack.TopAttackScores.Num());

	if (DataPack.TopAttackScores.Num() == 0)
	{
		CanvasContext.Printf(TEXT("{white}  (no data)"));
	}
	else
	{
		for (const auto& Entry : DataPack.TopAttackScores)
		{
			CanvasContext.Printf(
				TEXT("{white}  %s: ExpDmg={cyan}%.1f{white} Action={cyan}%.1f{white} Total={yellow}%.2f {white}[%s]"),
				*Entry.TargetName,
				Entry.ExpectedDamage,
				Entry.ActionScore,
				Entry.TotalScore,
				*Entry.AbilityTag);
		}
	}

	// --- 3) Heal 스코어링 ---
	if (DataPack.TopHealScores.Num() > 0)
	{
		CanvasContext.Printf(TEXT(""));
		CanvasContext.Printf(TEXT("{green}--- Heal Scores (Top %d) ---"), DataPack.TopHealScores.Num());
		for (const auto& Entry : DataPack.TopHealScores)
		{
			CanvasContext.Printf(
				TEXT("{white}  %s: EffHeal={cyan}%.1f{white} Action={cyan}%.1f{white} Total={yellow}%.2f"),
				*Entry.TargetName,
				Entry.ExpectedDamage,
				Entry.ActionScore,
				Entry.TotalScore);
		}
	}

	// --- 4) 시퀀스 정보 ---
	CanvasContext.Printf(TEXT(""));
	CanvasContext.Printf(TEXT("{green}--- Generated Sequence ---"));
	if (DataPack.SequenceActions.Num() == 0)
	{
		CanvasContext.Printf(TEXT("{white}  (no sequence)"));
	}
	else
	{
		for (int32 i = 0; i < DataPack.SequenceActions.Num(); ++i)
		{
			const auto& Act = DataPack.SequenceActions[i];

			// 현재 실행 중인 행동 하이라이트
			const TCHAR* Prefix = (DataPack.bExecuting && i == DataPack.CurrentActionIndex)
				? TEXT("{yellow}>>") : TEXT("{white}  ");

			if (Act.TargetLocation.IsZero())
			{
				CanvasContext.Printf(TEXT("%s[%d] %s → %s %s"),
					Prefix, i, *Act.ActionType, *Act.TargetName, *Act.AbilityTag);
			}
			else
			{
				CanvasContext.Printf(TEXT("%s[%d] %s → (%.0f, %.0f, %.0f) %s"),
					Prefix, i, *Act.ActionType,
					Act.TargetLocation.X, Act.TargetLocation.Y, Act.TargetLocation.Z,
					*Act.TargetName);
			}
		}

		CanvasContext.Printf(TEXT("{white}  TotalScore: {yellow}%.2f{white}  Ready: %s"),
			DataPack.SequenceTotalScore,
			DataPack.bSequenceReady ? TEXT("{green}YES") : TEXT("{red}NO (EQS pending)"));
	}

	// --- 5) 실행 상태 ---
	CanvasContext.Printf(TEXT(""));
	CanvasContext.Printf(TEXT("{green}--- Execution State ---"));
	if (DataPack.bWaitingForSequenceReady)
	{
		CanvasContext.Printf(TEXT("{red}  Waiting for EQS ready..."));
	}
	else if (DataPack.bExecuting)
	{
		CanvasContext.Printf(TEXT("{white}  Action %d/%d: {cyan}%s"),
			DataPack.CurrentActionIndex + 1,
			DataPack.TotalActions,
			*DataPack.CurrentActionDesc);
	}
	else
	{
		CanvasContext.Printf(TEXT("{white}  Idle"));
	}
}

//----------------------------------------------------------------------
// 스코어 맵 → ScoreEntry 변환
//----------------------------------------------------------------------
void FPBGameplayDebuggerCategory_AI::CollectScoreEntries(
	const TMap<AActor*, FPBTargetScore>& ScoreMap,
	TArray<FRepData::FScoreEntry>& OutEntries,
	int32 MaxCount)
{
	// TotalScore 기준 내림차순 정렬
	TArray<TPair<AActor*, FPBTargetScore>> SortedPairs;
	for (const auto& Pair : ScoreMap)
	{
		if (IsValid(Pair.Key))
		{
			SortedPairs.Add({Pair.Key, Pair.Value});
		}
	}

	SortedPairs.Sort([](const auto& A, const auto& B)
	{
		return A.Value.GetTotalScore() > B.Value.GetTotalScore();
	});

	const int32 Count = FMath::Min(SortedPairs.Num(), MaxCount);
	for (int32 i = 0; i < Count; ++i)
	{
		const auto& [Actor, Score] = SortedPairs[i];

		FRepData::FScoreEntry Entry;
		Entry.TargetName = Actor->GetName();
		Entry.ExpectedDamage = Score.ExpectedDamage;
		Entry.ActionScore = Score.GetActionScore();
		Entry.TotalScore = Score.GetTotalScore();
		Entry.AbilityTag = Score.AbilityTag.IsValid()
			? Score.AbilityTag.ToString() : TEXT("-");

		OutEntries.Add(MoveTemp(Entry));
	}
}

#endif // WITH_GAMEPLAY_DEBUGGER
