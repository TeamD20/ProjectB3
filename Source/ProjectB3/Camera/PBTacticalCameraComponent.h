// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "ProjectB3/Combat/PBCombatTypes.h"
#include "PBTacticalCameraComponent.generated.h"

class APBTacticalCameraActor;
class UAbilitySystemComponent;
class UGameplayAbility;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTB3_API UPBTacticalCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBTacticalCameraComponent();

	// 전술 카메라가 현재 ViewTarget인지 여부 반환
	bool IsTacticalCameraActive() const;

	// Orbit 모델 기반 카메라 목표 위치 계산
	// FocusPoint 기준으로 Yaw/Pitch/Distance에 따른 카메라 위치 반환
	FVector CalculateOrbitLocation(const FVector& FocusPoint, float Yaw, float Pitch, float Distance) const;

	// LookAt 기반 카메라 목표 회전 계산
	// CurrentRotation 기준으로 최소 회전각을 가지는 등가 회전을 반환
	FRotator CalculateOrbitRotation(const FVector& CameraLocation, const FVector& FocusPoint, const FRotator& CurrentRotation) const;

	// 피사체 거리 → 카메라 거리 매핑 (FramingNearThreshold~FramingFarThreshold 사이 보간)
	float CalculateDesiredDistance(float SubjectDistance) const;

	// 피사체 거리 → Pitch 매핑 (FramingNearThreshold~FramingFarThreshold 사이 보간)
	float CalculateDesiredPitch(float SubjectDistance) const;

protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 전투 상태 변경 처리 — CombatStarting 시 카메라 액터 스폰, CombatEnding 시 정리
	void HandleCombatStateChanged(EPBCombatState NewState);

	// 활성 턴 변경 처리 — 적 턴: CameraActor 즉시 배치 후 ViewTarget 전환
	void HandleActiveTurnChanged(AActor* NewCombatant, int32 TurnIndex);

	// 타겟 기반 스킬 발동 처리 — CameraActor TargetTransform 갱신
	void HandleAbilityExecutionStarted(const UGameplayAbility* Ability, const FPBAbilityTargetData& TargetData);

	// 피사체들이 HUD 패딩을 제외한 유효 뷰포트 안에 모두 들어오는 최소 카메라 높이 계산
	float CalculateSkillFramingHeight(const FVector& FocusPoint, float InYaw, const TArray<FVector>& SubjectLocations) const;

	// 카메라 액터 스폰
	void SpawnCameraActor();

	// 카메라 액터 파괴
	void DestroyCameraActor();

	// 전투 참가자들의 ASC 구독
	void SubscribeToAbilitySystems();

	// ASC 구독 해제
	void UnsubscribeFromAbilitySystems();

	// FactionTag 기반 적군 판별
	bool IsEnemyFaction(AActor* Actor) const;

public:
	// ViewTarget 전환 Blend 시간 (초)
	UPROPERTY(EditAnywhere, Category = "TacticalCamera")
	float BlendTime = 0.3f;

	// 적 턴 기본 카메라 거리
	UPROPERTY(EditAnywhere, Category = "TacticalCamera")
	float DefaultDistance = 1000.0f;

	// 적 턴 기본 Pitch
	UPROPERTY(EditAnywhere, Category = "TacticalCamera")
	float DefaultPitch = -50.0f;

	// 스킬 프레이밍 Pitch
	UPROPERTY(EditAnywhere, Category = "TacticalCamera")
	float SkillFramingPitch = -90.0f;

	// 적 진영 태그
	UPROPERTY(EditAnywhere, Category = "TacticalCamera")
	FGameplayTag EnemyFactionTag;

	// 스킬 프레이밍 시 HUD 위아래 합산 차지 비율 (예: 0.25 = 상하 각 12.5%)
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float HUDVerticalPaddingFraction = 0.4f;

	// 피사체 주변 추가 여백 (월드 단위)
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float SubjectWorldPadding = 150.0f;

	// 스킬 프레이밍 최소 거리 임계값
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float FramingNearThreshold = 300.0f;

	// 스킬 프레이밍 최대 거리 임계값
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float FramingFarThreshold = 2000.0f;

	// 스킬 프레이밍 최소 카메라 거리
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float FramingMinDistance = 800.0f;

	// 스킬 프레이밍 최대 카메라 거리
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float FramingMaxDistance = 3000.0f;

	// 가까운 피사체 Pitch
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float FramingNearPitch = -40.0f;

	// 먼 피사체 Pitch
	UPROPERTY(EditAnywhere, Category = "TacticalCamera|Framing")
	float FramingFarPitch = -65.0f;

private:
	// 전술 카메라 액터
	UPROPERTY()
	TObjectPtr<APBTacticalCameraActor> CameraActor;

	// 전투 상태 변경 구독 핸들
	FDelegateHandle CombatStateChangedHandle;

	// 활성 턴 변경 구독 핸들
	FDelegateHandle ActiveTurnChangedHandle;

	// ASC 구독 목록
	struct FASCEntry
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		FDelegateHandle ExecutionStartedHandle;
	};
	TArray<FASCEntry> ASCEntries;
};
