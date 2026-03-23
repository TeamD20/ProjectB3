// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "ProjectB3/PBGameplayTags.h"
#include "PBAbilityTypes.generated.h"

/** 어빌리티 타겟팅 모드 */
UENUM(BlueprintType)
enum class EPBTargetingMode : uint8
{
	// 타겟 지정 불필요 (Dash 등)
	None,

	// 자기 자신 대상 (자가 버프, 치유 등)
	Self,

	// 액터 1명 지정 (근접/원거리 공격, 단일 대상 주문)
	SingleTarget,

    // 액터 다수 지정
	MultiTarget,

	// 지면 위치 지정, 범위 표시 없음 (텔레포트, 소환 등)
	Location,

	// 위치 지정 + 범위 표시 (파이어볼, 폭발 화살 등)
	AoE
};

/** 어빌리티 자원 소모 유형 */
UENUM(BlueprintType)
enum class EPBAbilityType : uint8
{
	None,
	
	// 주 행동
	Action,

	// 보조 행동
	BonusAction,

	// 반응 행동 (상대 턴에 사용)
	Reaction,
	
	// 자원 소모 없음
	Free,
	
	// 이동
	Movement
};

/** 태그 컨테이너에서 Ability.Type.* 태그를 EPBAbilityType으로 변환. 미매칭 시 None 반환. */
inline EPBAbilityType GetAbilityTypeFromTags(const FGameplayTagContainer& Tags)
{
	if (Tags.HasTag(PBGameplayTags::Ability_Type_Action))      { return EPBAbilityType::Action; }
	if (Tags.HasTag(PBGameplayTags::Ability_Type_BonusAction)) { return EPBAbilityType::BonusAction; }
	if (Tags.HasTag(PBGameplayTags::Ability_Type_Reaction))    { return EPBAbilityType::Reaction; }
	if (Tags.HasTag(PBGameplayTags::Ability_Type_Free))        { return EPBAbilityType::Free; }
	if (Tags.HasTag(PBGameplayTags::Ability_Active_Move))        { return EPBAbilityType::Movement; }
	return EPBAbilityType::None;
}

/** 어빌리티 실행에 필요한 타겟 정보. AI/플레이어 양쪽 경로에서 공통 사용. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBAbilityTargetData
{
	GENERATED_BODY()

	// 타겟팅 모드 (수신 측에서 데이터 해석 기준)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	EPBTargetingMode TargetingMode = EPBTargetingMode::None;

	// 사전 지정 타겟 액터 목록 (Self / SingleTarget / MultiTarget 용)
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> TargetActors;

	// 타겟 위치 목록 (Location / AoE 용, 다수 지점 지정 가능)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	TArray<FVector> TargetLocations;

	// AoE 반경
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	float AoERadius = 0.f;

	// AoE 히트 결과 — 실행 단계에서 채워짐 (TargetActors와 의미가 다르므로 분리 유지)
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	// 단일 타겟 액터 반환 (SingleTarget, Self 용). 유효하지 않으면 nullptr 반환.
	AActor* GetSingleTargetActor() const
	{
		return (TargetActors.Num() > 0) ? TargetActors[0].Get() : nullptr;
	}

	// Self 타겟 액터 반환 (GetSingleTargetActor alias)
	AActor* GetSelfTargetActor() const
	{
		return GetSingleTargetActor();
	}

	// 전체 타겟 액터 배열 반환 (MultiTarget 용)
	TArray<AActor*> GetAllTargetActors() const
	{
		TArray<AActor*> Result;
		for (const TWeakObjectPtr<AActor>& Weak : TargetActors)
		{
			if (Weak.IsValid())
			{
				Result.Add(Weak.Get());
			}
		}
		return Result;
	}

	// 단일 타겟 위치 반환. 없으면 ZeroVector 반환.
	FVector GetSingleTargetLocation() const
	{
		return (TargetLocations.Num() > 0) ? TargetLocations[0] : FVector::ZeroVector;
	}

	// 전체 타겟 위치 배열 반환 (Location / AoE / bAllowGroundTarget 용)
	const TArray<FVector>& GetAllTargetLocations() const
	{
		return TargetLocations;
	}

	// 액터 또는 위치 타겟이 하나라도 존재하는지 확인
	bool HasTarget() const
	{
		return TargetActors.Num() > 0 || TargetLocations.Num() > 0;
	}

	// 타겟팅 모드 기반 유효성 검증
	bool IsValid() const
	{
		switch (TargetingMode)
		{
		case EPBTargetingMode::None:         return true;
		case EPBTargetingMode::Self:         return TargetActors.Num() > 0 && TargetActors[0].IsValid();
		case EPBTargetingMode::SingleTarget: return (TargetActors.Num() > 0 && TargetActors[0].IsValid()) || TargetLocations.Num() > 0;
		case EPBTargetingMode::MultiTarget:  return TargetActors.Num() > 0;
		case EPBTargetingMode::Location:     return TargetLocations.Num() > 0;
		case EPBTargetingMode::AoE:          return TargetLocations.Num() > 0 && AoERadius > 0.f;
		default:                             return false;
		}
	}
};


/** 주사위 굴림 판정 유형. 데미지/회복 양쪽에서 공용. 회복은 항상 None. */
UENUM(BlueprintType)
enum class EPBDiceRollType : uint8
{
	// 명중 굴림 (공격자가 d20 굴림): Roll + AttackBonus >= 대상 AC 이면 명중 (Natural 20 = 치명타)
	HitRoll,

	// 내성 굴림 (대상이 d20 굴림): Roll + SaveBonus >= 주문 난이도 이면 절반 적용
	SavingThrow,

	// 판정 없이 무조건 전체 적용 (회복, 무조건 데미지 등)
	None
};

/** 내성 굴림 결과. 내성 굴림에는 Natural 1/20 자동 실패/성공 규칙 없음. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBSavingThrowResult
{
	GENERATED_BODY()

	// d20 주사위 결과 (1-20)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	int32 Roll = 0;

	// 내성 성공 여부 (Roll + SaveBonus >= SpellSaveDC)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	bool bSucceeded = false;
};

/** 어빌리티 고유 주사위 설정. 데미지·회복 양쪽에서 공용. CDO에서 BP 디자이너가 설정. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBDiceSpec
{
	GENERATED_BODY()

	// 판정 유형 (회복 어빌리티는 None으로 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice")
	EPBDiceRollType RollType = EPBDiceRollType::HitRoll;

	// 주사위 수 (예: 2d6 에서 2)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice", meta = (ClampMin = "1"))
	int32 DiceCount = 1;

	// 주사위 면 수 (예: 2d6 에서 6)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice", meta = (ClampMin = "1"))
	int32 DiceFaces = 6;

	// UI 표시용: 주사위 설명 (예: "1d10 역장")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice|UI")
	FText DiceRollDesc;

	// UI 표시용: 주사위 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice|UI")
	TSoftObjectPtr<UTexture2D> DiceIcon;

	// UI 표시용: 주사위 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice|UI")
	FSlateColor DiceColor = FSlateColor(FLinearColor::White);

	// 공격자 데미지 수정치 핵심 능력치 재정의.
	// 미지정(IsValid() == false) 시 ASC의 AttackModifier 폴백 어트리뷰트 사용.
	// HitRoll / None / SavingThrow(주문 데미지 수정치 계산 경로)에서 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice")
	FGameplayAttribute AttackModifierAttributeOverride;

	// 공격자 명중 보너스/주문 난이도(DC) 핵심 능력치 재정의.
	// 미지정(IsValid() == false) 시 ASC의 HitBonus/SpellSaveDCModifier 폴백 어트리뷰트 사용.
	// HitRoll / SavingThrow(SpellSaveDC 계산) 전용. (예: Strength, Dexterity, Intelligence)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice", meta = (EditCondition = "RollType != EPBDiceRollType::None"))
	FGameplayAttribute BonusAttributeOverride;

	// 피주문자가 내성 굴림에 사용할 능력치 어트리뷰트. 이 어트리뷰트에 의해 피주문자가 내성 보너스를 획득.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dice", meta = (EditCondition = "RollType == EPBDiceRollType::SavingThrow"))
	FGameplayAttribute TargetSaveAttribute;
};

/** d20 명중 굴림 결과 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBHitRollResult
{
	GENERATED_BODY()

	// d20 주사위 결과 (1-20)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	int32 Roll = 0;

	// 명중 여부 (Natural 1 자동 실패, Natural 20 자동 성공, 그 외 Roll + HitBonus >= TargetAC)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	bool bHit = false;

	// 치명타 여부 (Natural 20)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	bool bCritical = false;
};

/** 무기 데미지 주사위 굴림 결과. ExecCalc에 SetByCaller로 전달되는 두 값을 묶어 반환. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBDamageRollResult
{
	GENERATED_BODY()

	// 무기 주사위 합산 결과 (치명타 시 주사위 수 2배)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	float DiceRoll = 0.f;

	// 공격 수정치 (Str 또는 Dex modifier)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Roll")
	float AttackModifier = 0.f;
};

class UPBGameplayAbility_Targeted;

/** 투사체 경로 프리뷰 컨텍스트. FPBTargetingRequest에 내장. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBProjectilePathContext
{
	GENERATED_BODY()

	// 투사체 경로 프리뷰 표시 여부
	UPROPERTY(BlueprintReadWrite, Category = "Targeting|Projectile")
	bool bShowPath = false;

	// 투사체 발사 지점 (무기 소켓 위치)
	UPROPERTY(BlueprintReadWrite, Category = "Targeting|Projectile")
	FVector LaunchLocation = FVector::ZeroVector;

	// Bezier 호 높이 파라미터 (APBProjectile CDO에서 복사)
	UPROPERTY(BlueprintReadWrite, Category = "Targeting|Projectile")
	float ArcHeightRatio = 0.3f;

	UPROPERTY(BlueprintReadWrite, Category = "Targeting|Projectile")
	float MinArcHeight = 50.f;

	UPROPERTY(BlueprintReadWrite, Category = "Targeting|Projectile")
	float MaxArcHeight = 500.f;
};

/** UPBTargetingComponent에 타겟팅 세션을 요청할 때 전달하는 구조체. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBTargetingRequest
{
	GENERATED_BODY()

	// 요청한 어빌리티 (사거리 검증 위임용)
	TWeakObjectPtr<UPBGameplayAbility_Targeted> RequestingAbility;

	// 타겟팅 모드
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	EPBTargetingMode Mode = EPBTargetingMode::None;

	// 발동자 위치 (사거리 계산 기준)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	FVector OriginLocation = FVector::ZeroVector;

	// 사거리
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	float Range = 0.f;

	// AoE 반경 (AoE 모드 미리보기용)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	float AoERadius = 0.f;

	// MultiTarget 최대 선택 수 (0이면 무제한)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	int32 MaxTargetCount = 1;

	// 액터 미지정 시 지면 위치 타겟 허용 (SingleTarget / MultiTarget 모드에서 유효)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	bool bAllowGroundTarget = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	bool bCanTargetSelf = false;

	// 투사체 경로 프리뷰 컨텍스트 (투사체 어빌리티에서만 설정)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	FPBProjectilePathContext ProjectileContext;
};

