// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBCombatStatsViewModel.generated.h"

using namespace PBUIDelegate;

/**
 * 모든 전투원(플레이어/적/NPC)에 공용으로 부착되는 Actor-Bound ViewModel.
 * 턴 자원(이동력) 및 핵심 전투 어트리뷰트(AC, 명중, 주문 난이도)를 UI에 중개한다.
 */
UCLASS()
class PROJECTB3_API UPBCombatStatsViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	/*~ 이동력 ~*/

	// 현재 이동력 반환
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	float GetCurrentMovement() const { return CurrentMovement; }

	// 최대 이동력 반환
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	float GetMaxMovement() const { return MaxMovement; }

	// 이동력 비율 (0~1) 반환
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	float GetMovementPercent() const { return MovementPercent; }

	// 이동력 갱신
	void SetMovement(float InCurrent, float InMax);

	/*~ 전투 어트리뷰트 ~*/

	// 방어력(AC) 반환
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	int32 GetArmorClass() const { return ArmorClass; }

	// 명중 수정치 반환
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	int32 GetHitBonus() const { return HitBonus; }

	// 주문 내성 난이도 반환
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	int32 GetSpellSaveDC() const { return SpellSaveDC; }

	// 방어력(AC) 갱신
	void SetArmorClass(int32 InAC);

	// 명중 수정치 갱신
	void SetHitBonus(int32 InHitBonus);

	// 주문 내성 난이도 갱신
	void SetSpellSaveDC(int32 InDC);

public:
	// 이동력 변경 (Current, Max)
	FOnCuurentMaxChangedSignature OnMovementChanged;

	// 이동력 비율 변경
	FOnFloatValueChangedSignature OnMovementPercentChanged;

	// 방어력 변경
	FOnInt32ValueChangedSignature OnArmorClassChanged;

	// 명중 수정치 변경
	FOnInt32ValueChangedSignature OnHitBonusChanged;

	// 주문 내성 난이도 변경
	FOnInt32ValueChangedSignature OnSpellSaveDCChanged;

private:
	// 현재 이동력 (cm)
	float CurrentMovement = 0.f;

	// 최대 이동력 (cm)
	float MaxMovement = 0.f;

	// 이동력 비율 (0~1)
	float MovementPercent = 0.f;

	// 방어력 (AC)
	int32 ArmorClass = 0;

	// 명중 수정치
	int32 HitBonus = 0;

	// 주문 내성 난이도
	int32 SpellSaveDC = 0;
};
