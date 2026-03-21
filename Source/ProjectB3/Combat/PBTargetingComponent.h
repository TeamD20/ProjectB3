// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBTargetingComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/** 타겟 확정 시 방송 — 확정된 타겟 데이터를 어빌리티 Task에 전달 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPBTargetConfirmed, const FPBAbilityTargetData&);

/** 타겟팅 취소 시 방송 */
DECLARE_MULTICAST_DELEGATE(FOnPBTargetCancelled);

/** 타겟 후보 갱신 시 방송 — UI 피드백용 (Phase 6에서 연결) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPBTargetUpdated, const FPBAbilityTargetData&, bool /*bInRange*/);

/** MultiTarget 선택 목록 변경 시 방송 — UI 선택 표시용 (Phase 6에서 연결) */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPBSelectionChanged, const FPBAbilityTargetData&);

/**
 * 타겟팅 세션 수명 관리 컴포넌트. PlayerController에 부착.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTB3_API UPBTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	APawn* GetPawn() const;
	
	bool IsHostileTarget(AActor* InTarget) const;
	
	// 타겟팅 세션 시작
	void EnterTargetingMode(const FPBTargetingRequest& Request);

	// 타겟팅 세션 종료 및 상태 초기화
	void ExitTargetingMode();

	// 호버 프리뷰 갱신 전용 (PC에서 호출)
	void UpdateTargetingFromHit(const FHitResult& HitResult);

	// MultiTarget 전용. 호버 중인 액터를 후보에 추가. MaxTargetCount 달성 시 자동 확정.
	void AddTargetSelection();

	// MultiTarget 전용. 후보 목록에서 마지막 항목 제거.
	void RemoveLastTarget();

	// 현재 호버/후보 데이터로 타겟 확정
	void ConfirmTarget();

	// 타겟팅 세션 취소
	void CancelTargeting();

	// 타겟팅 세션 활성 여부
	bool IsTargetingActive() const { return bIsTargetingActive; }

	// 현재 세션이 MultiTarget 모드인지 여부
	bool IsMultiTargetMode() const { return CurrentRequest.Mode == EPBTargetingMode::MultiTarget; }

	// AoE 텔레그래프 VFX 표시. 지정 위치에 나이아가라 이펙트를 활성화.
	void ShowAoETelegraph(const FVector& Location);
	
	// AoE 텔레그래프 VFX 숨김
	void HideAoETelegraph();
	
	// 사거리 텔레그래프 VFX 표시. 지정 위치에 나이아가라 이펙트를 활성화.
	void ShowRangeTelegraph();
	
	// 사거리 텔레그래프 VFX 숨김
	void HideRangeTelegraph();

private:
	// SelectedTargets를 FPBAbilityTargetData로 변환하는 내부 헬퍼
	FPBAbilityTargetData MakeMultiTargetData() const;

	// AoE텔레그래프 나이아가라 컴포넌트 생성 (최초 1회)
	void EnsureAoETelegraphComponent();

	// 사거리 나이아가라 컴포넌트 생성 (최초 1회)
	void EnsureRangeTelegraphComponent();

	// 대상 액터의 모든 메시에 커스텀 뎁스 스텐실 10 적용 (원래 상태 저장)
	void ApplyTargetHighlight(AActor* TargetActor);

	// 저장된 상태로 커스텀 뎁스 복원
	void ClearTargetHighlight();
	
public:
	// AoE 텔레그래프에 사용할 나이아가라 시스템 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Telegraph")
	TObjectPtr<UNiagaraSystem> AoETelegraphNiagaraSystem;

	// 범위 기반 어빌리티의 범위 표시 나이아가라 시스템 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Telegraph")
	TObjectPtr<UNiagaraSystem> RangeTelegraphNiagaraSystem;
	
	// 타겟 확정 델리게이트
	FOnPBTargetConfirmed OnTargetConfirmed;

	// 타겟팅 취소 델리게이트
	FOnPBTargetCancelled OnTargetCancelled;

	// 타겟 후보 갱신 델리게이트 — UI 호버 피드백용
	FOnPBTargetUpdated OnTargetPreviewUpdated;

	// MultiTarget 선택 목록 변경 델리게이트 — UI 선택 표시용
	FOnPBSelectionChanged OnSelectionChanged;

private:
	// 타겟팅 세션 활성 여부
	bool bIsTargetingActive = false;

	// 현재 타겟팅 요청 정보 (발동 어빌리티, 모드, 발동자 위치 등)
	FPBTargetingRequest CurrentRequest;

	// 매 틱 갱신되는 호버 프리뷰 데이터
	FPBAbilityTargetData HoverPreviewData;

	// 현재 HoverPreview가 사거리 내에 있고 유효한지 여부
	bool bIsHoverValid = false;

	// MultiTarget 모드에서 클릭으로 누적된 선택 목록
	TArray<TWeakObjectPtr<AActor>> SelectedTargets;

	// 텔레그래프 VFX 전용 액터 (월드에 독립 소환, PC 렌더링 제약 우회)
	UPROPERTY()
	TObjectPtr<AActor> TelegraphActor;

	// TelegraphActor에 부착된 텔레그래프 나이아가라 컴포넌트
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> AoETelegraphNiagaraComp;
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> RangeTelegraphNiagaraComp;
	
	bool bShowingAoETelegraph = false;

	// 현재 하이라이트 중인 호버 타겟
	TWeakObjectPtr<AActor> HighlightedTargetActor;

	// 하이라이트 진입 전 메시별 RenderCustomDepth 원래 값 (복원용)
	TMap<TObjectPtr<UMeshComponent>, bool> SavedTargetCustomDepthStates;
};
