// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;
struct FPBTargetScore;
struct FPBActionSequence;

/**
 * Gameplay Debugger 카테고리: AI Utility
 * 에디터에서 ' 키로 활성화 후, AI 액터 선택 시 다음 정보를 HUD에 표시:
 *   1) 스코어링 결과 (타겟별 ActionScore / HealScore)
 *   2) DFS 생성 시퀀스 (행동 목록, 총 유틸리티 점수)
 *   3) EQS 상태 (쿼리 대기 중 여부, 타임아웃)
 *   4) 실행 상태 (현재 행동, 진행 인덱스)
 */
class FPBGameplayDebuggerCategory_AI : public FGameplayDebuggerCategory
{
public:
	FPBGameplayDebuggerCategory_AI();

	// 카테고리 팩토리 (모듈 등록용)
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	/*~ FGameplayDebuggerCategory Interface ~*/
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

protected:
	// 수집된 디버그 데이터 (Replicate 가능 구조)
	struct FRepData
	{
		// 기본 정보
		FString ActorName;
		FString ArchetypeWeights;
		float CachedMaxMovement = 0.0f;
		int32 NumTargets = 0;
		int32 NumAllies = 0;

		// 스코어링 결과 (상위 5개)
		struct FScoreEntry
		{
			FString TargetName;
			float ExpectedDamage = 0.0f;
			float ActionScore = 0.0f;
			float TotalScore = 0.0f;
			FString AbilityTag;
		};
		TArray<FScoreEntry> TopAttackScores;
		TArray<FScoreEntry> TopHealScores;

		// 생성된 시퀀스
		struct FSequenceEntry
		{
			FString ActionType;
			FString TargetName;
			FString AbilityTag;
			FVector TargetLocation = FVector::ZeroVector;
		};
		TArray<FSequenceEntry> SequenceActions;
		float SequenceTotalScore = 0.0f;
		bool bSequenceReady = false;

		// EQS 상태
		bool bWaitingForEQS = false;
		int32 PendingEQSCount = 0;

		// 실행 상태
		bool bExecuting = false;
		bool bWaitingForSequenceReady = false;
		int32 CurrentActionIndex = 0;
		int32 TotalActions = 0;
		FString CurrentActionDesc;
	};

	FRepData DataPack;

private:
	// 스코어 맵 → FScoreEntry 변환 헬퍼
	static void CollectScoreEntries(
		const TMap<AActor*, FPBTargetScore>& ScoreMap,
		TArray<FRepData::FScoreEntry>& OutEntries,
		int32 MaxCount = 5);
};

#endif // WITH_GAMEPLAY_DEBUGGER
