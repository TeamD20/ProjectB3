// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBFloatingTextPayload.generated.h"

/** 플로팅 텍스트 표시 타입. 어빌리티 BP에서 타입별 위젯 클래스를 선택하기 위한 키. */
UENUM(BlueprintType)
enum class EPBFloatingTextType : uint8
{
	// 일반 피해 수치
	Damage,

	// 회복 수치
	Heal,

	// 빗나감 텍스트
	Miss,

	// 내성 성공 텍스트
	SaveSuccess,

	// 내성 실패 텍스트
	SaveFailed,

	// 상태이상/버프/디버프 등 일반 텍스트
	Status
};

/**
 * 플로팅 텍스트 이벤트 페이로드.
 * FGameplayEventData.OptionalObject를 통해 ShowFloatingText 어빌리티로 전달된다.
 * UPBTargetPayload 패턴을 따른다.
 */
UCLASS(BlueprintType)
class PROJECTB3_API UPBFloatingTextPayload : public UObject
{
	GENERATED_BODY()

public:
	// 위젯 선택용 타입
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	EPBFloatingTextType FloatingTextType = EPBFloatingTextType::Damage;

	// 수치 (데미지, 힐량 등. 0이면 텍스트만 표시)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	float Magnitude = 0.f;

	// 표시 텍스트 ("Miss", "Save Success" 등. 비어있으면 수치만 표시)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	FText Text;

	// 부가 의미 전달용 메타 태그 (예: Combat.Hit.Critical, 상태 태그 등)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	FGameplayTag MetaTag;

	// 플로팅 텍스트용 아이콘 (속성 데미지 아이콘, 상태이상 아이콘 등)
	UPROPERTY(BlueprintReadWrite, Category = "FloatingText")
	TSoftObjectPtr<UTexture2D> Icon;
};
