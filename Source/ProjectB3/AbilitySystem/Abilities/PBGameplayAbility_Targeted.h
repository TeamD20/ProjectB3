// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBGameplayAbility.h"
#include "PBGameplayAbility_Targeted.generated.h"

class UPBAbilityTask_WaitTargeting;

/** 타겟팅 어빌리티 종료 모드 */
UENUM(BlueprintType)
enum class EPBTargetedAbilityEndMode : uint8
{
	// ExecuteTargetLogic 반환 즉시 자동으로 EndAbility 호출
	Auto,

	// EndAbility 호출을 ExecuteTargetLogic 내부에 완전히 위임 (애니메이션, 발사체 등 비동기 로직)
	Manual
};

/**
 * 타겟팅 기반 어빌리티.
 * AI: TriggerEventData의 Payload로 즉시 실행.
 * 플레이어: TargetingMode에 따라 AbilityTask로 비동기 타겟팅 후 실행.
 */
UCLASS()
class PROJECTB3_API UPBGameplayAbility_Targeted : public UPBGameplayAbility
{
	GENERATED_BODY()

public:
	// 사거리 기반 어빌리티인지 여부 (Range > 0)
	bool IsRangedAbility() const { return Range > 0.f; }

	// 사거리 값 조회
	float GetRange() const { return Range; }
	
	// 타겟이 사거리 내에 있는지 검증
	bool IsTargetInRange(const FVector& SourceLocation, const FPBAbilityTargetData& TargetData) const;

	// 타겟이 어빌리티를 시전한 주체인지 여부
	bool IsTargetSelf(const AActor* InTargetActor) const;
	
	// AoE 반경 조회
	float GetAoERadius() const { return AoERadius; }

	// 타겟팅 모드 조회
	EPBTargetingMode GetTargetingMode() const { return TargetingMode; }

	// MultiTarget 최대 선택 수 조회
	int32 GetMaxTargetCount() const { return MaxTargetCount; }

	// 지면 타겟 허용 여부 조회
	bool IsGroundTargetAllowed() const { return bAllowGroundTarget; }

	// 현재 어빌리티 프로퍼티로 타겟팅 요청 구조체 생성. 서브클래스에서 오버라이드하여 추가 컨텍스트 세팅 가능.
	virtual FPBTargetingRequest MakeTargetingRequest() const;

	// 투사체 발사 지점의 현재 월드 위치 반환. 타겟팅 중 매 프레임 호출되어 경로 프리뷰 시작점 갱신.
	// 기본 반환값: ZeroVector (투사체 미사용 어빌리티). 서브클래스에서 오버라이드.
	virtual FVector GetProjectileLaunchLocation() const { return FVector::ZeroVector; }

protected:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	
	/*~ UPBGameplayAbility_Targeted Interface ~*/
	// 수신된 타겟 데이터 기반 어빌리티 로직 실행. BP 버전
	UFUNCTION(BlueprintNativeEvent)
	void K2_ExecuteTargetLogic(const FPBAbilityTargetData& TargetData);
	
	// 수신된 타겟 데이터 기반 어빌리티 로직 실행. 스킬 어빌리티에서 override하여 구현.
	virtual void ExecuteTargetLogic(const FPBAbilityTargetData& TargetData);

	// 타겟팅 태스크 시작 직전 호출되는 블루프린트 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Targeting")
	void OnStartTargeting();

private:
	void InternalExecuteTargetLogic(const FPBAbilityTargetData& TargetData);
	
	// AbilityTask 기반 비동기 타겟팅 진입 (플레이어 전용 경로)
	void StartTargetingTask();

	// 타겟팅 확정 콜백 — Task로부터 수신
	void OnTargetingConfirmed(const FPBAbilityTargetData& TargetData);

	// 타겟팅 취소 콜백 — Task로부터 수신
	void OnTargetingCancelled();

	// InitialTargetingDelay 대기 완료 콜백
	UFUNCTION()
	void OnTargetingDelayFinished();

	// EndMode == Auto일 때만 EndAbility를 호출하는 내부 헬퍼
	void TryAutoEndAbility(const FGameplayAbilitySpecHandle& Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo& ActivationInfo);

protected:
	// 종료 모드. Auto: 실행 후 자동 종료. Manual: ExecuteAbilityLogic 내부에서 EndAbility 직접 호출.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	EPBTargetedAbilityEndMode EndMode = EPBTargetedAbilityEndMode::Manual;

	// 타겟팅 모드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
	EPBTargetingMode TargetingMode = EPBTargetingMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
	bool bCanTargetSelf = false;
	
	// 사거리 (0이면 무제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float Range = 0.f;
	
	// AoE 반경 (AoE 모드에서만 유효)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting",
		meta = (EditCondition = "TargetingMode == EPBTargetingMode::AoE", ClampMin = "0.0"))
	float AoERadius = 0.f;

	// MultiTarget 최대 선택 수 (0이면 무제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting",
		meta = (EditCondition = "TargetingMode == EPBTargetingMode::MultiTarget", ClampMin = "0"))
	int32 MaxTargetCount = 1;

	// 액터 미지정 시 지면 위치 타겟 허용 (SingleTarget / MultiTarget 모드에서 유효)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting")
	bool bAllowGroundTarget = false;

	// 타겟팅 태스크 시작 전 지연 시간 (초). 0이면 즉시 시작.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ClampMin = "0.0"))
	float InitialTargetingDelay = 0.f;
};
