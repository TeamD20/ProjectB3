// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "ProjectB3/Game/PBPrewarmInterface.h"
#include "PBTargetingComponent.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;

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
class PROJECTB3_API UPBTargetingComponent : public UActorComponent, public IPBPrewarmInterface
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

	// 투사체 경로 프리뷰 표시. TargetActor: 타겟 액터 (visibility 체크 시 무시).
	void ShowProjectilePath(const FVector& TargetLocation, AActor* TargetActor = nullptr);

	// 투사체 경로 프리뷰 숨기기
	void HideProjectilePath();

	// 투사체 경로에 장애물이 있는지 여부
	bool IsProjectilePathBlocked() const { return bProjectilePathBlocked; }
	
	// 선택된 타겟 개수
	int32 NumSelectedTargets() const {return SelectedTargets.Num();}

	/*~ IPBPrewarmInterface ~*/
	virtual void NativeCollectPrewarmTargets(FPBPrewarmTargets& InOutTargets) override;

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

	// 투사체 경로 스플라인 메시 세그먼트 풀 관리
	void EnsureProjectilePathPool();
	USplineMeshComponent* GetOrCreatePathSegment(int32 Index);

	// 장애물 Visibility 체크 (발사 지점 → 타겟 직선 Line Trace). IgnoreActor: 타겟 액터 등 무시 대상.
	bool CheckProjectileVisibility(const FVector& Start, const FVector& End, AActor* IgnoreActor = nullptr) const;
	
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

	// 투사체 경로 프리뷰 에셋
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath")
	TObjectPtr<UMaterialInterface> ClearPathMaterial;		// 흰색 — 장애물 없음

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath")
	TObjectPtr<UMaterialInterface> BlockedPathMaterial;		// 빨간색 — 장애물 있음 / 사거리 밖 공용

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath")
	TObjectPtr<UStaticMesh> PathSegmentMesh;				// 세그먼트용 메시

	// 곡선 세분화 수
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath", meta = (ClampMin = "4", ClampMax = "32"))
	int32 ProjectilePathSegmentCount = 12;

	// Visibility 트레이스 시작점 오프셋 (발사 지점에서 타겟 방향으로, cm)
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath")
	float VisibilityTraceSourceOffset = 50.f;

	// Visibility 트레이스 끝점 수평 오프셋 (타겟에서 발사 지점 방향으로, cm)
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath")
	float VisibilityTraceTargetHorizontalOffset = 30.f;

	// Visibility 트레이스 끝점 Z 오프셋 (타겟에서 위쪽으로, cm)
	UPROPERTY(EditDefaultsOnly, Category = "Targeting|ProjectilePath")
	float VisibilityTraceTargetZOffset = 50.f;

	// 스플라인 메시 세그먼트 풀 (TelegraphActor에 부착)
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> ProjectilePathPool;

	// 현재 경로에 장애물이 있는지 여부
	bool bProjectilePathBlocked = false;

	// 현재 투사체 경로 프리뷰 표시 중 여부
	bool bShowingProjectilePath = false;

	// 현재 하이라이트 중인 호버 타겟
	TWeakObjectPtr<AActor> HighlightedTargetActor;

	// 하이라이트 진입 전 메시별 RenderCustomDepth 원래 값 (복원용)
	TMap<TObjectPtr<UMeshComponent>, bool> SavedTargetCustomDepthStates;
};
