// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBGameplayAbility.generated.h"

class UPBAbilityTask_WaitTargeting;

/** 어빌리티 종료 모드 */
UENUM(BlueprintType)
enum class EPBAbilityEndMode : uint8
{
	// ExecuteAbilityLogic 반환 즉시 자동으로 EndAbility 호출
	Auto,

	// EndAbility 호출을 ExecuteAbilityLogic 내부에 완전히 위임 (애니메이션, 발사체 등 비동기 로직)
	Manual
};

/** 프로젝트 전용 GameplayAbility 기반 클래스. 모든 어빌리티는 이 클래스를 상속해서 구현. */
UCLASS()
class PROJECTB3_API UPBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/*~ UPBGameplayAbility Interface ~*/
	// 사거리 기반 어빌리티인지 여부 (Range > 0)
	bool IsRangedAbility() const;

	// 타겟이 사거리 내에 있는지 검증. PlayerController, AI(StateTree) 양쪽에서 호출.
	bool IsTargetInRange(const FVector& SourceLocation, const FPBAbilityTargetData& TargetData) const;

	// 사거리 값 조회
	float GetRange() const { return Range; }

	// AoE 반경 조회
	float GetAoERadius() const { return AoERadius; }

	// 타겟팅 모드 조회
	EPBTargetingMode GetTargetingMode() const { return TargetingMode; }

protected:
	// 어빌리티 로직 실행. 스킬 어빌리티에서 override하여 구현.
	virtual void ExecuteAbilityLogic(const FPBAbilityTargetData& TargetData) {}

	// Payload에서 타겟 데이터 추출 유틸리티
	FPBAbilityTargetData ExtractTargetDataFromEvent(const FGameplayEventData& EventData) const;

	// AbilityTask 기반 비동기 타겟팅 진입 (플레이어 전용 경로)
	void StartTargetingTask();

	// 타겟팅 확정 콜백 — Task로부터 수신
	void OnTargetingConfirmed(const FPBAbilityTargetData& TargetData);

	// 타겟팅 취소 콜백 — Task로부터 수신
	void OnTargetingCancelled();

private:
	// EndMode == Auto일 때만 EndAbility를 호출하는 내부 헬퍼
	void TryAutoEndAbility(const FGameplayAbilitySpecHandle& Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo);

protected:
	// 타겟팅 모드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
	EPBTargetingMode TargetingMode = EPBTargetingMode::None;

	// 자원 소모 유형 (CostGameplayEffectClass와 대응)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cost")
	EPBAbilityCostType CostType = EPBAbilityCostType::Action;

	// 종료 모드. Auto: 실행 후 자동 종료. Manual: ExecuteAbilityLogic 내부에서 EndAbility 직접 호출.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EPBAbilityEndMode EndMode = EPBAbilityEndMode::Auto;

	// 사거리 (0이면 무제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ClampMin = "0.0"))
	float Range = 0.f;

	// AoE 반경 (AoE 모드에서만 유효)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting",
		meta = (EditCondition = "TargetingMode == EPBTargetingMode::AoE", ClampMin = "0.0"))
	float AoERadius = 0.f;

	// MultiTarget 최대 선택 수 (0이면 무제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting",
		meta = (EditCondition = "TargetingMode == EPBTargetingMode::MultiTarget", ClampMin = "0"))
	int32 MaxTargetCount = 1;
};
