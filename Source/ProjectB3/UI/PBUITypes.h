#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
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
};

namespace PBUIDelegate
{
	/* ~파티 / 또는 공용 델리게이트 ~ */ 
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextChangedSignature, FText);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnImageChangedSignature, TSoftObjectPtr<UTexture2D>);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatValueChangedSignature, float);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoolValueChangedSignature, bool);
	
	// 플레이어 턴 일때 호출할 델리게이트 - 할당 예시) 플레이어 턴일때 알림 사운드 재생
	//TODO 아직 선언만 미리 해둠
	DECLARE_MULTICAST_DELEGATE_OneParam(OnPlayerTurnStarted, bool);

	
	/* ~턴오더 UI 델리게이트~(턴 오더 시스템 완성후 이전작업 해야함) */ 
	DECLARE_MULTICAST_DELEGATE(FOnTurnOrderListChangedSignature);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnAdvancedSignature, UPBTurnPortraitViewModel*);
}


namespace PBSkillBarTabIndex
{
	static constexpr int32 Action = 0;
	static constexpr int32 BonusAction = 1;
	static constexpr int32 Spell = 2;
}