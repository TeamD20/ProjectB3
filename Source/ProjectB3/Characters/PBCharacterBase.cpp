// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterBase.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySetData.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/ItemSystem/PBEquipmentActor.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
#include "ProjectB3/UI/PBAbilitySystemUIBridge.h"

APBCharacterBase::APBCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// ASC 생성
	AbilitySystemComponent = CreateDefaultSubobject<UPBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// AttributeSet 생성
	CharacterAttributeSet = CreateDefaultSubobject<UPBCharacterAttributeSet>(TEXT("CharacterAttributeSet"));
	TurnResourceAttributeSet = CreateDefaultSubobject<UPBTurnResourceAttributeSet>(TEXT("TurnResourceAttributeSet"));

	// ASC to UI 브리지
	AbilitySystemUIBridge = CreateDefaultSubobject<UPBAbilitySystemUIBridge>(TEXT("AbilitySystemUIBridge"));

	// 인벤토리 컴포넌트 생성
	InventoryComponent = CreateDefaultSubobject<UPBInventoryComponent>(TEXT("InventoryComponent"));

	// 장비 컴포넌트 생성
	EquipmentComponent = CreateDefaultSubobject<UPBEquipmentComponent>(TEXT("EquipmentComponent"));
	
	static ConstructorHelpers::FClassFinder<UAnimInstance> ABPFinder(TEXT("/Game/2_Characters/Manny/ABP_CharacterBase.ABP_CharacterBase_C"));
	if (ABPFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(ABPFinder.Class);
	}
	
	static ConstructorHelpers::FClassFinder<UAnimInstance> DefaultAnimLayerFinder(TEXT("/Game/2_Characters/Manny/ABP_WeaponLayerBase.ABP_WeaponLayerBase_C"));
	if (DefaultAnimLayerFinder.Succeeded())
	{
		DefaultAnimLayerClass = DefaultAnimLayerFinder.Class;
	}
}

UAbilitySystemComponent* APBCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

APBEquipmentActor* APBCharacterBase::AttachEquipment(const FGameplayTag& InSlotTag,
	TSubclassOf<APBEquipmentActor> EquipmentClass)
{
	if (!InSlotTag.IsValid())
	{
		return nullptr;
	}
	if (!IsValid(EquipmentClass))
	{
		return nullptr;
	}
	if (!EquipmentSlotTagNameMap.Contains(InSlotTag) || EquipmentSlotTagNameMap[InSlotTag] == NAME_None)
	{
		return nullptr;
	}
	
	// 이미 해당 클래스 액터를 해당 슬롯에 장착중인 경우
	if (AttachedEquipments.Contains(InSlotTag))
	{
		APBEquipmentActor* ExistingEquipment = AttachedEquipments[InSlotTag];
		if (IsValid(ExistingEquipment) && ExistingEquipment->GetClass() == EquipmentClass.Get())
		{
			return ExistingEquipment;
		}

		if (IsValid(ExistingEquipment))
		{
			ExistingEquipment->UnlinkAnimLayer(GetMesh());
			ExistingEquipment->Destroy();
		}

		AttachedEquipments.Remove(InSlotTag);
	}
	
	FName SlotName = EquipmentSlotTagNameMap[InSlotTag];
	if (!IsValid(GetMesh()) || !IsValid(GetWorld()))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = GetInstigator();

	APBEquipmentActor* SpawnedEquipment = GetWorld()->SpawnActor<APBEquipmentActor>(EquipmentClass, FTransform::Identity, SpawnParameters);
	if (!IsValid(SpawnedEquipment))
	{
		return nullptr;
	}

	if (!SpawnedEquipment->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SlotName))
	{
		SpawnedEquipment->Destroy();
		return nullptr;
	}
	
	SpawnedEquipment->LinkAnimLayer(GetMesh());
	AttachedEquipments.Add(InSlotTag, SpawnedEquipment);
	OnCharacterEquipmentChanged.Broadcast(InSlotTag);
	return SpawnedEquipment;
}

bool APBCharacterBase::DetachEquipment(const FGameplayTag& InSlotTag)
{
	if (!InSlotTag.IsValid())
	{
		return false;
	}

	if (!AttachedEquipments.Contains(InSlotTag))
	{
		return false;
	}

	APBEquipmentActor* ExistingEquipment = AttachedEquipments[InSlotTag];
	AttachedEquipments.Remove(InSlotTag);

	if (IsValid(ExistingEquipment))
	{
		ExistingEquipment->UnlinkAnimLayer(GetMesh());
		ExistingEquipment->Destroy();
	}

	OnCharacterEquipmentChanged.Broadcast(InSlotTag);
	return true;
}

void APBCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetMesh()))
	{
		GetMesh()->LinkAnimClassLayers(DefaultAnimLayerClass);
	}
	
	// InitAbilityActorInfo 이후 어빌리티 부여
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		InitTags();
		GrantInitialAbilities();
	}

	// 기본 아이템/장비 지급 (GAS 초기화 이후 장비 어빌리티 부여가 올바르게 동작하도록 순서 보장)
	GrantDefaultItems();
}

void APBCharacterBase::GrantInitialAbilities()
{
	// TODO: 레벨 전달
	FPBAbilityGrantedHandles Handles;
	UPBAbilitySystemLibrary::ApplyStatsInitialization(AbilitySystemComponent,Handles,CombatIdentity.ClassTag);
	UPBAbilitySystemLibrary::ApplyCommonAbilitySet(AbilitySystemComponent);
	UPBAbilitySystemLibrary::ApplyClassAbilitySet(AbilitySystemComponent,CombatIdentity.ClassTag);
}

void APBCharacterBase::GrantDefaultItems()
{
	// 인벤토리 기본 아이템 추가
	if (IsValid(InventoryComponent))
	{
		for (const FPBDefaultItemEntry& Entry : DefaultItems)
		{
			if (IsValid(Entry.ItemData))
			{
				InventoryComponent->AddItem(Entry.ItemData, Entry.Amount);
			}
		}
	}

	// 기본 장비 장착: 인벤토리에 추가 후 지정 슬롯에 장착
	if (IsValid(InventoryComponent) && IsValid(EquipmentComponent))
	{
		for (const FPBDefaultEquipmentEntry& Entry : DefaultEquipments)
		{
			if (!IsValid(Entry.ItemData))
			{
				continue;
			}

			// InstanceID를 보존해 장착 시 정확한 인스턴스를 참조할 수 있도록 직접 생성
			const FPBItemInstance NewInstance = FPBItemInstance::Create(Entry.ItemData, 1);
			if (InventoryComponent->AddItemInstance(NewInstance))
			{
				EquipmentComponent->EquipItem(NewInstance.InstanceID, Entry.Slot, InventoryComponent);
			}
		}
	}
}

void APBCharacterBase::InitTags()
{
	if (AbilitySystemComponent)
	{
		if (CombatIdentity.ClassTag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(CombatIdentity.ClassTag);	
		}
	}
}

int32 APBCharacterBase::GetInitiativeModifier() const
{
	// TODO: Attribute 수정치 기반으로 반환
	return 0;
}

bool APBCharacterBase::HasInitiativeAdvantage() const
{
	return false;
}

void APBCharacterBase::OnCombatBegin()
{
	bIsInCombat = true;
}

void APBCharacterBase::OnCombatEnd()
{
	bIsInCombat = false;
}

void APBCharacterBase::OnRoundBegin()
{
	// Reaction 리필
	if (IsValid(AbilitySystemComponent) && IsValid(TurnResourceAttributeSet))
	{
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetReactionAttribute(), 1.0f);
	}
}

void APBCharacterBase::OnTurnBegin()
{
	// Action, BonusAction, Movement 리셋
	if (IsValid(AbilitySystemComponent) && IsValid(TurnResourceAttributeSet))
	{
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetActionAttribute(), 1.0f);
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetBonusActionAttribute(), 1.0f);
		AbilitySystemComponent->ResetMovementResource();
	}
}

void APBCharacterBase::OnTurnActivated()
{
	// 기본 구현: 별도 처리 없음 (하위 클래스에서 override)
}

void APBCharacterBase::OnTurnEnd()
{
	// 기본 구현: 별도 처리 없음
}

bool APBCharacterBase::CanReact() const
{
	if (IsIncapacitated())
	{
		return false;
	}

	if (IsValid(AbilitySystemComponent) && IsValid(TurnResourceAttributeSet))
	{
		return TurnResourceAttributeSet->GetReaction() > 0.0f;
	}

	return false;
}

void APBCharacterBase::OnReactionOpportunity(const FPBReactionContext& Context)
{
	// 기본 구현: 아무 것도 하지 않음 (하위 클래스에서 override)
}

void APBCharacterBase::OnActionInterrupted()
{
	// 기본 구현: 아무 것도 하지 않음 (하위 클래스에서 override)
}

bool APBCharacterBase::IsIncapacitated() const
{
	// 기본 구현: 하위 클래스에서 상태이상 시스템 기반으로 override
	return false;
}

void APBCharacterBase::SetCombatIdentity(const FPBCombatIdentity& InIdentity)
{
	CombatIdentity = InIdentity;
}

FGameplayTag APBCharacterBase::GetFactionTag() const
{
	if (CombatIdentity.FactionTag.IsValid())
	{
		return CombatIdentity.FactionTag;
	}
	return PBGameplayTags::Combat_Faction_Neutral;
}

float APBCharacterBase::GetBaseMovementSpeed() const
{
	return 30.0f;
}

FText APBCharacterBase::GetCombatDisplayName() const
{
	if (!CombatIdentity.DisplayName.IsEmpty())
	{
		return CombatIdentity.DisplayName;
	}
	return FText::FromString(GetName());
}

TSoftObjectPtr<UTexture2D> APBCharacterBase::GetCombatPortrait() const
{
	return CombatIdentity.Portrait;
}

FVector APBCharacterBase::GetCombatTargetLocation() const
{
	// TODO: 허리 위치나 흉부 위치 반환.
	return GetActorLocation();
}
