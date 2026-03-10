// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBSkillBarTypes.h"
#include "PBSkillBarViewModel.generated.h"

class APBGameplayPlayerState;
class UAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE(FOnSkillSlotsChanged);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSkillSlotUpdated, int32 /*TabIndex*/, int32 /*SlotIndex*/);

/** 선택된 파티원 기준 스킬바 데이터를 구성하는 글로벌 ViewModel */
UCLASS()
class PROJECTB3_API UPBSkillBarViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	// 플레이어 초기화 시 PlayerState를 찾아 자동 바인딩한다.
	virtual void InitializeForPlayer(ULocalPlayer* InLocalPlayer) override;

	// PlayerState 변경/해제 시 바인딩을 정리한다.
	virtual void Deinitialize() override;

	// PlayerState 선택 변경 델리게이트에 바인딩한다.
	void BindToPlayerState(APBGameplayPlayerState* InPlayerState);

	// 특정 캐릭터의 ASC 기준으로 슬롯을 재구성한다.
	void RefreshFromCharacter(AActor* InCharacter);

	// 전체 슬롯의 쿨다운/활성 가능 상태를 갱신한다.
	void RefreshAllCooldowns();

	// 탭/인덱스로 슬롯 데이터를 조회한다.
	bool GetSlotData(int32 TabIndex, int32 SlotIndex, FPBSkillSlotData& OutSlotData) const;

	// 탭 인덱스로 슬롯 배열을 조회한다.
	const TArray<FPBSkillSlotData>* GetSlotsByTab(int32 TabIndex) const;

	// 현재 바인딩된 PlayerState를 반환한다.
	APBGameplayPlayerState* GetPlayerState() const { return PlayerState.Get(); }

private:
	// 선택 파티원 변경 시 슬롯을 재구성한다.
	void HandleSelectedPartyMemberChanged(AActor* NewSelectedPartyMember);

	// 파티원 목록 변경 시 현재 선택 기준으로 슬롯을 재구성한다.
	void HandlePartyMembersChanged();

	// 필터 조건으로 슬롯 목록을 생성한다.
	void BuildSlotsFromFilter(
		UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayTagContainer& RequireTags,
		const FGameplayTagContainer& IgnoreTags,
		TArray<FPBSkillSlotData>& OutSlots) const;

	// 현재 선택된 파티원 ASC를 반환한다.
	UAbilitySystemComponent* GetSelectedAbilitySystemComponent() const;

public:
	// 주행동 탭 슬롯
	TArray<FPBSkillSlotData> ActionSlots;

	// 보조행동 탭 슬롯
	TArray<FPBSkillSlotData> BonusActionSlots;

	// 주문 탭 슬롯
	TArray<FPBSkillSlotData> SpellSlots;

	// 전체 슬롯 재구성 이벤트
	FOnSkillSlotsChanged OnSlotsChanged;

	// 개별 슬롯 갱신 이벤트
	FOnSkillSlotUpdated OnSlotUpdated;

private:
	// 바인딩된 PlayerState
	TWeakObjectPtr<APBGameplayPlayerState> PlayerState;

	// PlayerState 델리게이트 핸들
	FDelegateHandle SelectedPartyMemberChangedHandle;

	// 파티원 목록 변경 델리게이트 핸들
	FDelegateHandle PartyMembersChangedHandle;
};
