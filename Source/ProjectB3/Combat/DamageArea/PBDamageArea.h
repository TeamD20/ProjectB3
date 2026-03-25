// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "ProjectB3/Game/PBPrewarmInterface.h"
#include "PBDamageArea.generated.h"

class UAbilitySystemComponent;
class UNavModifierComponent;
class USphereComponent;
class UPBCombatManagerSubsystem;

/**
 * 라운드 기반 지속형 피해 영역 액터.
 * 초기화 시 이펙트 스펙을 전달받아, 영역 진입 액터에게 적용한다.
 */
UCLASS(Abstract)
class PROJECTB3_API APBDamageArea : public AActor, public IPBPrewarmInterface
{
	GENERATED_BODY()

public:
	APBDamageArea();

	/**
	 * 피해 영역 초기화.
	 * @param InEffectSpec   적용할 GameplayEffect 스펙 핸들
	 * @param InSourceASC    이펙트 소유자의 AbilitySystemComponent
	 * @param InDurationRounds 유지 라운드 수 (0이면 수동 제거 전까지 유지)
	 */
	UFUNCTION(BlueprintCallable)
	void InitDamageArea(const FGameplayEffectSpecHandle& InEffectSpec,
		UAbilitySystemComponent* InSourceASC,
		int32 InDurationRounds);

	// 지표면 스냅 대상 컴포넌트 등록 (BP ConstructionScript에서 호출)
	UFUNCTION(BlueprintCallable, Category = "DamageArea")
	void AddGroundSnapComponent(USceneComponent* Component);

	// 영역 반경 반환
	float GetAreaRadius() const;

	// 예상 데미지 반환 (Init 시 산출)
	float GetExpectedDamage() const { return ExpectedDamage; }

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 오버랩 진입 시 이펙트 적용 처리
	UFUNCTION()
	void OnAreaBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 대상 액터에게 이펙트 적용
	void ApplyEffectToActor(AActor* TargetActor);

	// 라운드 변경 시 영역 내 액터 이펙트 갱신 및 수명 체크
	void OnRoundChanged(int32 NewRound);

	// 영역 내 모든 유효 대상에게 이펙트 재적용
	void ReapplyEffectToOverlappingActors();

	// 등록된 컴포넌트들을 지표면 높이로 스냅
	void SnapComponentsToGround();

	// GE 스펙으로부터 예상 데미지 산출
	void CalculateExpectedDamage();

protected:
	// 오버랩 감지용 구체 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DamageArea")
	TObjectPtr<USphereComponent> CollisionComponent;

	// AI 경로 탐색 시 위험 영역 비용 부여용 NavModifier
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DamageArea")
	TObjectPtr<UNavModifierComponent> NavModifierComponent;

	// 적용할 이펙트 스펙 핸들
	UPROPERTY(BlueprintReadOnly, Category = "DamageArea|AbilitySystem")
	FGameplayEffectSpecHandle EffectSpecHandle;

	// 이펙트 소유자 ASC
	UPROPERTY(BlueprintReadOnly, Category = "DamageArea|AbilitySystem")
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 유지 라운드 수 (0이면 무한)
	UPROPERTY(BlueprintReadOnly, Category = "DamageArea")
	int32 DurationRounds = 0;

private:
	// 생성 시점 라운드 번호
	int32 SpawnRound = 0;

	// 라운드 델리게이트 핸들
	FDelegateHandle RoundChangedHandle;

	// 지표면 스냅 대상 컴포넌트 목록
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> GroundSnapComponents;

	// 산출된 예상 데미지 (Init 시 계산)
	float ExpectedDamage = 0.f;
};
