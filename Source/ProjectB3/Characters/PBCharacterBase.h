// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include <ProjectB3/Combat/IPBCombatTarget.h>

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBCharacterBase.generated.h"

class UNavModifierComponent;
class APBEquipmentActor;
class UPBCharacterAttributeSet;
class UPBAbilitySystemComponent;
class UPBAbilitySystemUIBridge;
class UPBTurnResourceAttributeSet;
class UPBAbilitySetData;
class UPBInventoryComponent;
class UPBEquipmentComponent;

// 장비 부착/제거 시 브로드캐스트되는 델리게이트 (슬롯 태그)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterEquipmentChanged, const FGameplayTag&, SlotTag);

// 플레이어와 AI가 공유하는 캐릭터 기반 클래스.
UCLASS()
class PROJECTB3_API APBCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPBCombatParticipant, public IPBCombatTarget
{
	GENERATED_BODY()

public:
	APBCharacterBase();

	/*~ IAbilitySystemInterface ~*/
	// AbilitySystemComponent 반환
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*~ IPBCombatParticipant Interface ~*/
	// 이니셔티브 수정치 반환 (기본 0, 하위 클래스에서 DEX 기반 override)
	virtual int32 GetInitiativeModifier() const override;

	// 이니셔티브 이점 보유 여부
	virtual bool HasInitiativeAdvantage() const override;

	// 전투 시작 시 호출
	virtual void OnCombatBegin() override;

	// 전투 종료 시 호출
	virtual void OnCombatEnd() override;

	// 라운드 시작 시 호출 (Reaction 리필)
	virtual void OnRoundBegin() override;

	// 턴 시작 시 호출 (Action/BonusAction/Movement 리셋)
	virtual void OnTurnBegin() override;

	// 실제 행동 차례가 됐을 때 호출
	virtual void OnTurnActivated() override;

	// 턴 종료 시 호출
	virtual void OnTurnEnd() override;

	// 반응 행동 가능 여부
	virtual bool CanReact() const override;

	// 반응 기회 통지
	virtual void OnReactionOpportunity(const FPBReactionContext& Context) override;

	// 행동 중단 통지
	virtual void OnActionInterrupted() override;

	// 무력화 여부
	virtual bool IsIncapacitated() const override;

	// 진영 태그
	virtual FGameplayTag GetFactionTag() const override;

	// 기본 이동 속도 (cm 단위)
	virtual float GetBaseMovementSpeed() const override;

	// 표시 이름
	virtual FText GetCombatDisplayName() const override;

	// 초상화
	virtual TSoftObjectPtr<UTexture2D> GetCombatPortrait() const override;

	/*~ ICombatTarget Interface ~*/
	virtual FVector GetCombatTargetLocation() const override;
	
	/*~ APBCharacterBase Interface ~*/
	// 전투 식별 정보 설정
	void SetCombatIdentity(const FPBCombatIdentity& InIdentity);

	// 전투 식별 정보 반환
	const FPBCombatIdentity& GetCombatIdentity() const { return CombatIdentity; }

	// AbilitySystemComponent 반환 (타입 지정)
	UPBAbilitySystemComponent* GetPBAbilitySystemComponent() const { return AbilitySystemComponent; }
	
	// 캐릭터 AttributeSet 반환
	UPBCharacterAttributeSet* GetCharacterAttributeSet() const { return CharacterAttributeSet; }
	
	// 턴 자원 AttributeSet 반환
	UPBTurnResourceAttributeSet* GetTurnResourceAttributeSet() const { return TurnResourceAttributeSet; }

	// 장비 부착후 스폰된 액터 반환
	UFUNCTION(BlueprintCallable, Category = "Combat")
	APBEquipmentActor* AttachEquipment(const FGameplayTag& InSlotTag, TSubclassOf<APBEquipmentActor> EquipmentClass);

	// 슬롯 기준으로 장비 제거 성공 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool DetachEquipment(const FGameplayTag& InSlotTag);

	// 슬롯 태그에 부착된 장비 액터 반환. 없으면 nullptr.
	APBEquipmentActor* GetAttachedEquipment(const FGameplayTag& SlotTag) const;

	// 인벤토리 컴포넌트 반환
	UPBInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// 장비 컴포넌트 반환
	UPBEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

public:
	// 장비 부착/제거 후 브로드캐스트되는 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnCharacterEquipmentChanged OnCharacterEquipmentChanged;
	
protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ APBCharacterBase Interface ~*/
	// 어빌리티 초기 부여 (InitAbilityActorInfo 이후 호출)
	virtual void GrantInitialAbilities();
	// 캐릭터 태그 부여
	virtual void InitTags();
	// 기본 지급 아이템/장비를 인벤토리/장비 컴포넌트에 추가
	virtual void GrantDefaultItems();
	// ASC의 OwnedTags 스택 변경 이벤트 핸들러, TagExists가 true 면 태그 추가, false면 태그 제거
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists);
	
protected:
	// 기본 애니메이션 레이어
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;

	// BeginPlay 시 인벤토리에 자동 추가되는 기본 아이템 목록 (테스트용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items|Default")
	TArray<FPBDefaultItemEntry> DefaultItems;

	// BeginPlay 시 자동 장착되는 기본 장비 목록 (테스트용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Items|Default")
	TArray<FPBDefaultEquipmentEntry> DefaultEquipments;
	
	// 장비 부착 슬롯 (메시 슬롯) 태그 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TMap<FGameplayTag, FName> EquipmentSlotTagNameMap;
	
	// 부착된 장비 액터 목록, SlotTag : AttachedActor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TMap<FGameplayTag, APBEquipmentActor*> AttachedEquipments;
	
	// AbilitySystemComponent
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UPBAbilitySystemComponent> AbilitySystemComponent;

	// 캐릭터 AttributeSet
	UPROPERTY()
	TObjectPtr<UPBCharacterAttributeSet> CharacterAttributeSet;
	
	// 턴 자원 AttributeSet
	UPROPERTY()
	TObjectPtr<UPBTurnResourceAttributeSet> TurnResourceAttributeSet;

	// ASC → UI 브리지 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UPBAbilitySystemUIBridge> AbilitySystemUIBridge;

	// 인벤토리 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UPBInventoryComponent> InventoryComponent;

	// 장비 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UPBEquipmentComponent> EquipmentComponent;
	
	// NavModifier (캐릭터 영역을 경로에서 제외)
	/*
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UNavModifierComponent> NavModifierComponent;
	*/

	// 전투 식별 정보 (진영, 표시 이름, 초상화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FPBCombatIdentity CombatIdentity;
	
	// 전투 중 여부
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInCombat = false;
};
