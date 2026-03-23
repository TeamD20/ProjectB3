#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBUITypes.generated.h"

class UPBTurnOrderViewModel;
class UPBTurnPortraitViewModel;

USTRUCT(BlueprintType)
struct FPBTurnOrderEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn")
	bool bIsAlly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn")
	AActor* TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn")
	float InitialHealthPercent = 0.0f;
};

namespace PBUIDelegate
{
	/* ~파티 / 또는 공용 델리게이트 ~ */ 
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextChangedSignature, FText);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnImageChangedSignature, TSoftObjectPtr<UTexture2D>);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatValueChangedSignature, float);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoolValueChangedSignature, bool);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInt32ValueChangedSignature, int32);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCuurentMaxChangedSignature, float /*Current*/, float /*Max*/);

	// 플레이어 턴 일때 호출할 델리게이트 - 할당 예시) 플레이어 턴일때 알림 사운드 재생
	//TODO 아직 선언만 미리 해둠
	DECLARE_MULTICAST_DELEGATE_OneParam(OnPlayerTurnStarted, bool);

	
	/* ~턴오더 UI 델리게이트~(턴 오더 시스템 완성후 이전작업 해야함) */ 
	DECLARE_MULTICAST_DELEGATE(FOnTurnOrderListChangedSignature);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnAdvancedSignature, UPBTurnPortraitViewModel*);
}

// 스킬 시전 이벤트 델리게이트
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSkillActivatedSignature, AActor* /*Caster*/, const FText& /*SkillName*/, EPBAbilityType /*AbilityType*/);


namespace PBSkillBarTabIndex
{
	static constexpr int32 Action = 0;
	static constexpr int32 BonusAction = 1;
	static constexpr int32 Spell = 2;
}

/** 자원(행동, 보조, 주문 등)의 종류를 구분하는 열거형 */
UENUM(BlueprintType)
enum class EPBResourceType : uint8
{
	None UMETA(DisplayName = "None"),
	Action UMETA(DisplayName = "Action"),
	BonusAction UMETA(DisplayName = "Bonus Action"),
	SpellSlot UMETA(DisplayName = "Spell Slot")
};

/** 단일 자원의 현재 상태 스냅샷 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBResourceState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	EPBResourceType ResourceType = EPBResourceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 CurrentValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	int32 MaxValue = 0;
};

/** 자원 툴팁 표시용 정적 데이터 구조체 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBResourceTooltipData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Tooltip")
	FText ResourceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Tooltip")
	FText ResourceSubtitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Tooltip")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Tooltip")
	FText RechargeText;

	// 자원 상징 색상 아이콘 (행동: 초록, 보조: 주황 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Tooltip")
	TSoftObjectPtr<UTexture2D> ResourceIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Tooltip")
	TSoftObjectPtr<UTexture2D> RechargeIcon;
};