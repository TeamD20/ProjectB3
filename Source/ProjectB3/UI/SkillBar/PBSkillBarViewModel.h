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
	UPBSkillBarViewModel();
	
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

	// 탭/인덱스로 슬롯 데이터를 조회한다. (0: Primary, 1: Secondary, 2: Spell)
	bool GetSlotData(int32 CategoryIndex, int32 SlotIndex, FPBSkillSlotData& OutSlotData) const;

	// 카테고리 인덱스로 슬롯 배열을 조회한다. (0: Primary, 1: Secondary, 2: Spell, 3: Response)
	const TArray<FPBSkillSlotData>* GetSlotsByCategory(int32 CategoryIndex) const;

	// 현재 바인딩된 PlayerState를 반환한다.
	APBGameplayPlayerState* GetPlayerState() const { return PlayerState.Get(); }

private:
	// 선택 파티원 변경 시 슬롯을 재구성한다.
	void HandleSelectedPartyMemberChanged(AActor* NewSelectedPartyMember);

	// 소비 아이템 목록만 별도로 갱신한다.
	void RefreshConsumables();

	// 인벤토리 아이템 변경 이벤트 핸들러
	UFUNCTION()
	void HandleInventoryItemChanged(int32 SlotIndex);

	// 인벤토리 전체 갱신 이벤트 핸들러
	UFUNCTION()
	void HandleInventoryFullRefresh();

	// 파티원 목록 변경 시 현재 선택 기준으로 슬롯을 재구성한다.
	void HandlePartyMembersChanged();

	// 필터 조건으로 슬롯 목록을 생성한다.
	void BuildSlotsFromFilter(
		UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayTagContainer& RequireTags,
		const FGameplayTagContainer& IgnoreTags,
		const FText& InSkillType,
		TArray<FPBSkillSlotData>& OutSlots) const;

	// 현재 선택된 파티원 ASC를 반환한다.
	UAbilitySystemComponent* GetSelectedAbilitySystemComponent() const;

public:
	// 주행동 슬롯 (Primary)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ViewModel|SkillBar")
	TArray<FPBSkillSlotData> PrimaryActions;

	// 보조행동 슬롯 (Secondary)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ViewModel|SkillBar")
	TArray<FPBSkillSlotData> SecondaryActions;

	// 소비 아이템 슬롯 (Consumable)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ViewModel|SkillBar")
	TArray<FPBSkillSlotData> ConsumableActions;

	// [초록 영역] 대응 스킬 슬롯 (Response/Reaction)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ViewModel|SkillBar")
	TArray<FPBSkillSlotData> ResponseActions;

	// [노랑 영역] 좌측 주무기 슬롯 데이터
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ViewModel|Equipment")
	TArray<FPBEquipmentSlotData> WeaponSlots;

	// [노랑 영역] 우측 유틸리티/소모품 슬롯 데이터
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ViewModel|Equipment")
	TArray<FPBEquipmentSlotData> UtilitySlots;

	// 전체 슬롯 재구성 이벤트
	FOnSkillSlotsChanged OnSlotsChanged;

	// 개별 슬롯 갱신 이벤트
	FOnSkillSlotUpdated OnSlotUpdated;

private:
	// 바인딩된 PlayerState
	TWeakObjectPtr<APBGameplayPlayerState> PlayerState;

	// 바인딩된 인벤토리 컴포넌트
	TWeakObjectPtr<class UPBInventoryComponent> CachedInventory;

	// PlayerState 델리게이트 핸들
	FDelegateHandle SelectedPartyMemberChangedHandle;

	// 파티원 목록 변경 델리게이트 핸들
	FDelegateHandle PartyMembersChangedHandle;
};
