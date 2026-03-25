// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterBase.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySetData.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"
#include "ProjectB3/ItemSystem/PBEquipmentActor.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
#include "ProjectB3/Interaction/PBInteractableComponent.h"
#include "ProjectB3/Interaction/Actions/PBInteraction_LootAction.h"
#include "ProjectB3/UI/PBAbilitySystemUIBridge.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectB3/Combat/PBCombatSettings.h"

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

	// 상호작용 컴포넌트 생성
	InteractableComponent = CreateDefaultSubobject<UPBInteractableComponent>(TEXT("InteractableComponent"));

	// 기본 루팅 상호작용 행동 추가
	UPBInteraction_LootAction* LootAction = CreateDefaultSubobject<UPBInteraction_LootAction>(TEXT("LootAction"));
	InteractableComponent->InteractionActions.Add(LootAction);
	
	// 진영 표시 인디케이터 VFX 설정
	FactionIndicator = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FactionIndicator"));
	FactionIndicator->SetupAttachment(RootComponent);
	FactionIndicator->bAutoActivate = false;
	FactionIndicator->SetRelativeLocation(FVector(0.f,0.f,-90.f));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FactionVFXFinder(TEXT("/Game/3_Effects/Telegraph/NS_Faction_Circle.NS_Faction_Circle"));
	if (FactionVFXFinder.Succeeded())
	{
		FactionIndicator->SetAsset(FactionVFXFinder.Object);
	}
	
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
	
	GetCapsuleComponent()->SetCanEverAffectNavigation(true);
	GetCapsuleComponent()->bDynamicObstacle = true;
	
	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;
	
	bCanAffectNavigationGeneration = true;
}

void APBCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	BindPathFollowingComponent(NewController);
	
	const bool bIsPlayerController = GetController() && GetController()->IsPlayerController();
	if (bIsPlayerController)
	{
		SetCanAffectNavigationGeneration(false);
	}
}

void APBCharacterBase::UnPossessed()
{
	UnbindPathFollowingComponent();
	
	if (!bCanAffectNavigationGeneration)
	{
		SetCanAffectNavigationGeneration(true);	
	}
	
	Super::UnPossessed();
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
	if (!IsValid(CachedVisualMesh) || !IsValid(GetWorld()))
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

	if (!SpawnedEquipment->AttachToComponent(CachedVisualMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SlotName))
	{
		SpawnedEquipment->Destroy();
		return nullptr;
	}

	SpawnedEquipment->LinkAnimLayer(GetMesh());
	
	AttachedEquipments.Add(InSlotTag, SpawnedEquipment);
	OnCharacterEquipmentChanged.Broadcast(InSlotTag);
	return SpawnedEquipment;
}

bool APBCharacterBase::DetachEquipmentInstance(APBEquipmentActor* EquipmentInstance)
{
	if (!IsValid(EquipmentInstance))
	{
		return false;
	}
	
	for (TPair<FGameplayTag, APBEquipmentActor*>& KVP :AttachedEquipments)
	{
		if (KVP.Value == EquipmentInstance)
		{
			return DetachEquipment(KVP.Key);
		}
	}
	
	return false;
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
	
	if (AttachedEquipments.IsEmpty())
	{
		GetMesh()->LinkAnimClassLayers(DefaultAnimLayerClass);
	}

	OnCharacterEquipmentChanged.Broadcast(InSlotTag);
	return true;
}

APBEquipmentActor* APBCharacterBase::GetAttachedEquipment(const FGameplayTag& SlotTag) const
{
	APBEquipmentActor* const* Found = AttachedEquipments.Find(SlotTag);
	return Found ? *Found : nullptr;
}

APBEquipmentActor* APBCharacterBase::K2_GetAttachedEquipment(const FGameplayTag SlotTag,
	TSubclassOf<APBEquipmentActor> EquipmentClass)
{
	return GetAttachedEquipment(SlotTag);
}

void APBCharacterBase::SetupVisualMesh()
{
	CachedVisualMesh = nullptr;
	if (IsValid(GetMesh()))
	{
		TArray<USceneComponent*> ChildMeshes;
		GetMesh()->GetChildrenComponents(true, ChildMeshes);
		for (USceneComponent* ChildMesh : ChildMeshes)
		{
			USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(ChildMesh);
			if (IsValid(SkelComp) && SkelComp->GetName() == TEXT("VisualMesh"))
			{
				CachedVisualMesh = SkelComp;
				break;
			}
		}

		// VisualMesh를 찾지 못한 경우 기본 메시 사용
		if (!IsValid(CachedVisualMesh))
		{
			CachedVisualMesh = GetMesh();
		}
	}
}

void APBCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// GetMesh의 자식 중 이름이 VisualMesh인 SkeletalMeshComponent를 탐색
	SetupVisualMesh();

	if (IsValid(GetMesh()))
	{
		GetMesh()->LinkAnimClassLayers(DefaultAnimLayerClass);
	}
	
	// InitAbilityActorInfo 이후 어빌리티 부여
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		// 태그 변경 이벤트 구독
		AbilitySystemComponent->OnGameplayTagUpdated.AddUObject(this, &ThisClass::HandleGameplayTagUpdated);
		
		// 기본 태그 부여
		InitTags();
		// 기본 어빌리티 부여
		GrantInitialAbilities();
	}

	// 기본 아이템/장비 지급 (GAS 초기화 이후 장비 어빌리티 부여가 올바르게 동작하도록 순서 보장)
	GrantDefaultItems();
}

void APBCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnGameplayTagUpdated.RemoveAll(this);
	}

	UnbindPathFollowingComponent();
	
	Super::EndPlay(EndPlayReason);
}

void APBCharacterBase::BindPathFollowingComponent(AController* InController)
{
	UnbindPathFollowingComponent();

	if (!IsValid(InController))
	{
		return;
	}

	UPathFollowingComponent* PathFollowingComponent = InController->FindComponentByClass<UPathFollowingComponent>();
	if (!IsValid(PathFollowingComponent))
	{
		return;
	}

	BoundPathFollowingComponent = PathFollowingComponent;
	PathFollowingComponent->OnRequestFinished.AddUObject(this, &ThisClass::HandlePathFollowingRequestFinished);
}

void APBCharacterBase::UnbindPathFollowingComponent()
{
	if (IsValid(BoundPathFollowingComponent))
	{
		BoundPathFollowingComponent->OnRequestFinished.RemoveAll(this);
		BoundPathFollowingComponent = nullptr;
	}

	bWasPathFollowingMoving = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PathFollowingPollTimerHandle);
	}
}

void APBCharacterBase::HandlePathFollowingRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	const bool bIsPlayerController = GetController() && GetController()->IsPlayerController();
	if (!bIsPlayerController)
	{
		SetCanAffectNavigationGeneration(true);	
	}
}

void APBCharacterBase::GrantInitialAbilities()
{
	// TODO: 레벨 전달
	if (IsValid(InnateAbilitySet))
	{
		AbilitySystemComponent->GrantAbilitiesFromData(PBGameplayTags::Ability_Source_Innate, InnateAbilitySet);	
	}
	
	FPBAbilityGrantedHandles Handles;
	UPBAbilitySystemLibrary::ApplyStatsInitialization(AbilitySystemComponent,Handles,CharacterIdentity.ClassTag);
	UPBAbilitySystemLibrary::ApplyCommonAbilitySet(AbilitySystemComponent);
	UPBAbilitySystemLibrary::ApplyClassAbilitySet(AbilitySystemComponent,CharacterIdentity.ClassTag);
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

void APBCharacterBase::HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists)
{
	if (ChangedTag == PBGameplayTags::Character_State_Incapacitated && TagExists)
	{
		if (UPBCombatManagerSubsystem* CombatManager = UPBCombatSystemLibrary::GetCombatManager(this))
		{
			if (CombatManager->IsInCombat())
			{
				CombatManager->NotifyCombatantIncapacitated(this);
			}
		}
	}
	if (ChangedTag == PBGameplayTags::Character_State_Dead && TagExists)
	{
		HandleDeath();
	}
}

void APBCharacterBase::HandleDeath()
{
	// 캐릭터 사망 영역은 NavMesh 활성화
	SetCanAffectNavigationGeneration(false, true);
	FactionIndicator->Deactivate();
}

void APBCharacterBase::InitTags()
{
	if (AbilitySystemComponent)
	{
		if (CharacterIdentity.ClassTag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(CharacterIdentity.ClassTag);	
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
	
	if (IsValid(FactionIndicator))
	{
		FColor FactionColor = FColor::White;
		if (const UPBCombatSettings* const CombatSettings = GetDefault<UPBCombatSettings>())
		{
			if (CharacterIdentity.FactionTag.MatchesTag(PBGameplayTags::Combat_Faction_Player))
			{
				FactionColor = CombatSettings->FriendlyColor;
			}
			if (CharacterIdentity.FactionTag.MatchesTag(PBGameplayTags::Combat_Faction_Enemy))
			{
				FactionColor = CombatSettings->HostileColor;
			}
			if (CharacterIdentity.FactionTag.MatchesTag(PBGameplayTags::Combat_Faction_Neutral))
			{
				FactionColor = CombatSettings->NeutralColor;
			}
		}
		FactionIndicator->SetColorParameter(TEXT("User.Color"), FactionColor);
		FactionIndicator->Activate();
	}
}

void APBCharacterBase::OnCombatEnd()
{
	bIsInCombat = false;
	
	if (IsValid(FactionIndicator))
	{
		FactionIndicator->Deactivate();
	}
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
		// 이펙트 스택 차감
		AbilitySystemComponent->OnProgressTurn();
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

bool APBCharacterBase::IsDead() const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(PBGameplayTags::Character_State_Dead);
	}
	return false;
}

bool APBCharacterBase::IsIncapacitated() const
{
	FGameplayTagContainer IncapacitatedTags;
	IncapacitatedTags.AddTag(PBGameplayTags::Character_State_Dead);
	IncapacitatedTags.AddTag(PBGameplayTags::Character_State_Incapacitated);
	IncapacitatedTags.AddTag(PBGameplayTags::Character_State_Debuff_Stunned);
	
	if (AbilitySystemComponent->HasAnyMatchingGameplayTags(IncapacitatedTags))
	{
		return true;
	}
	return false;
}

void APBCharacterBase::SetCombatIdentity(const FPBCharacterIdentity& InIdentity)
{
	CharacterIdentity = InIdentity;
}

FGameplayTag APBCharacterBase::GetFactionTag() const
{
	if (CharacterIdentity.FactionTag.IsValid())
	{
		return CharacterIdentity.FactionTag;
	}
	return PBGameplayTags::Combat_Faction_Neutral;
}

float APBCharacterBase::GetBaseMovementSpeed() const
{
	return 30.0f;
}

FText APBCharacterBase::GetCombatDisplayName() const
{
	if (!CharacterIdentity.DisplayName.IsEmpty())
	{
		return CharacterIdentity.DisplayName;
	}
	return FText::FromString(GetName());
}

TSoftObjectPtr<UTexture2D> APBCharacterBase::GetCombatPortrait() const
{
	return CharacterIdentity.Portrait;
}

FVector APBCharacterBase::GetCombatTargetLocation() const
{
	// TODO: 허리 위치나 흉부 위치 반환.
	return GetActorLocation();
}
