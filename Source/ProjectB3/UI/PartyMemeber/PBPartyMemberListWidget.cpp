// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyMemberListWidget.h"
#include "PBPartyMemberListViewModel.h"
#include "PBPartyMemberViewModel.h"
#include "PBPartyMemberWidget.h"
#include "Components/VerticalBox.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"



void UPBPartyMemberListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (UPBPartyMemberListViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBPartyMemberListViewModel>(GetOwningLocalPlayer()))
	{
		InitializeBinding(VM);
	}
}

void UPBPartyMemberListWidget::InitializeBinding(UPBPartyMemberListViewModel* InViewModel)
{
	if (!IsValid(InViewModel))
	{
		UE_LOG(LogTemp, Error, TEXT("InitializeBinding: Invalid ViewModel!"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("UPBPartyMemberListWidget::InitializeBinding Started"));
		
	ListViewModel = InViewModel;
	
	ListViewModel->OnPartyListChanged.AddDynamic(this, &UPBPartyMemberListWidget::HandlePartyListChanged);
	
	HandlePartyListChanged();
}

void UPBPartyMemberListWidget::HandlePartyListChanged()
{
	if (MemberListBox)
	{
		MemberListBox->ClearChildren();
		UE_LOG(LogTemp, Warning, TEXT("HandlePartyListChanged: MemberListBox cleared."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HandlePartyListChanged: MemberListBox is NULL! Check BindWidget in Blueprint."));
	}
	
	if (!IsValid(ListViewModel))
	{
		UE_LOG(LogTemp, Error, TEXT("HandlePartyListChanged: ListViewModel is Invalid!"));
		return;
	}
	
	const TArray<UPBPartyMemberViewModel*>& Members = ListViewModel->GetMemberViewModels();
	UE_LOG(LogTemp, Warning, TEXT("HandlePartyListChanged: Found %d Members in ViewModel"), Members.Num());
	
	for (UPBPartyMemberViewModel* MemberVM : Members)
	{
		if (MemberVM)
		{
			CreatePartyMemberWidget(MemberVM);
		}
	}

}

void UPBPartyMemberListWidget::CreatePartyMemberWidget(UPBPartyMemberViewModel* MemberVM)
{
	if (MemberWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePartyMemberWidget: MemberWidgetClass is NOT SET!"));
		return;
	}

	if (MemberVM == nullptr) return;
	
	UPBPartyMemberWidget* NewMemberWidget = CreateWidget<UPBPartyMemberWidget>(GetWorld(), MemberWidgetClass);
	
	if (NewMemberWidget)
	{
		NewMemberWidget->InitializeWithViewModel(MemberVM);
		
		if (MemberListBox)
		{
			MemberListBox->AddChild(NewMemberWidget);
			UE_LOG(LogTemp, Warning, TEXT("CreatePartyMemberWidget: Added Child Widget for VM"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePartyMemberWidget: Failed to CreateWidget!"));
	}
}
