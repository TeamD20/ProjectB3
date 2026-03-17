// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteraction_LootAction.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/Interaction/PBInteractionAction.h"
#include "ProjectB3/Interaction/PBInteractorComponent.h"
#include "ProjectB3/UI/Loot/PBLootPanelWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"

UPBInteraction_LootAction::UPBInteraction_LootAction()
{
	InteractionType = EPBInteractionType::Sustained;
	bRequiresRange = true;
	
	static ConstructorHelpers::FClassFinder<UPBLootPanelWidget> LootPanelFinder(TEXT("/Game/0_BP/UI/Loot/W_LootPanel.W_LootPanel_C"));
	if (LootPanelFinder.Succeeded())
	{
		LootWidgetClass = LootPanelFinder.Class;
	}
}

bool UPBInteraction_LootAction::CanInteract_Implementation(AActor* Interactor) const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return false;
	}

	// Owner가 ASC를 보유하면 루팅 가능 상태 태그 확인
	if (LootableTag.IsValid())
	{
		if (const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner))
		{
			UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
			if (IsValid(ASC))
			{
				return ASC->HasMatchingGameplayTag(LootableTag);
			}
		}
	}

	// ASC가 없는 대상(보물상자 등)은 항상 루팅 가능
	return true;
}

void UPBInteraction_LootAction::Execute_Implementation(AActor* Interactor)
{
	if (!IsValid(Interactor) || !LootWidgetClass)
	{
		return;
	}

	// Interactor의 PlayerController 획득
	APlayerController* PC = Cast<APlayerController>(GetController(Interactor));
	if (!IsValid(PC))
	{
		return;
	}

	// 이미 루팅 UI가 열려있으면 중복 Push 방지
	UPBUIManagerSubsystem* UIManager = UPBUIBlueprintLibrary::GetUIManager(PC);
	if (IsValid(UIManager) && UIManager->IsUIActive(LootWidgetClass))
	{
		return;
	}

	// 루팅 UI Push 후 대상 액터로 초기화
	UPBWidgetBase* Widget = UPBUIBlueprintLibrary::PushUI(PC, LootWidgetClass);
	UPBLootPanelWidget* LootWidget = Cast<UPBLootPanelWidget>(Widget);
	if (!IsValid(LootWidget))
	{
		return;
	}

	LootWidget->InitializeLoot(GetOwner(), PC);

	// 위젯의 OnLootClosed 델리게이트에 바인딩
	LootWidget->OnLootClosed.AddDynamic(this, &ThisClass::HandleLootWidgetClosed);
	CachedLootWidget = LootWidget;

	// InteractorComponent 캐시 (종료 요청용)
	CachedInteractorComponent = PC->FindComponentByClass<UPBInteractorComponent>();

	// 베이스 클래스에서 bIsActive = true 설정
	Super::Execute_Implementation(Interactor);
}

void UPBInteraction_LootAction::EndInteraction_Implementation()
{
	// 위젯이 아직 열려있으면 Pop (거리 초과 등 외부 종료 경로)
	if (CachedLootWidget.IsValid())
	{
		UPBLootPanelWidget* LootWidget = CachedLootWidget.Get();
		LootWidget->OnLootClosed.RemoveDynamic(this, &ThisClass::HandleLootWidgetClosed);

		if (APlayerController* PC = LootWidget->GetOwningPlayer())
		{
			UPBUIBlueprintLibrary::PopUI(PC, LootWidget);
		}
	}

	CachedLootWidget.Reset();
	CachedInteractorComponent.Reset();

	// 베이스 클래스에서 bIsActive = false 설정
	Super::EndInteraction_Implementation();
}

void UPBInteraction_LootAction::HandleLootWidgetClosed()
{
	// 위젯이 닫혔으므로 델리게이트 해제
	if (CachedLootWidget.IsValid())
	{
		CachedLootWidget->OnLootClosed.RemoveDynamic(this, &ThisClass::HandleLootWidgetClosed);
		CachedLootWidget.Reset();
	}

	// InteractorComponent에 종료 요청
	if (CachedInteractorComponent.IsValid())
	{
		CachedInteractorComponent->EndActiveInteraction();
	}
}
