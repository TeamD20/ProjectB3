// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBUIManagerSubsystem.h"
#include "GameFramework/PlayerController.h"

void UPBUIManagerSubsystem::Deinitialize()
{
	ResetSystem();
	Super::Deinitialize();
}

void UPBUIManagerSubsystem::ResetSystem()
{
	for (int32 Index = UIStack.Num() - 1; Index >= 0; --Index)
	{
		UPBWidgetBase* Widget = UIStack[Index];
		if (!IsValid(Widget))
		{
			continue;
		}
		
		Widget->RemoveFromParent();
	}
	UIStack.Reset();
	
	if (APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

UPBWidgetBase* UPBUIManagerSubsystem::PushUI(TSubclassOf<UPBWidgetBase> WidgetClass)
{
	if (!WidgetClass)
	{
		return nullptr;
	}

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	UPBWidgetBase* Widget = CreateWidget<UPBWidgetBase>(PC, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}

	Widget->AddToViewport();
	UIStack.Add(Widget);
	UpdateInputMode();
	return Widget;
}

void UPBUIManagerSubsystem::PopUI(UPBWidgetBase* Instance)
{
	CleanupInvalidEntries();

	if (UIStack.IsEmpty())
	{
		return;
	}

	UPBWidgetBase* Target = Instance;
	if (!Target)
	{
		Target = UIStack.Last();
	}

	if (Target)
	{
		UIStack.Remove(Target);
		Target->RemoveFromParent();
	}

	UpdateInputMode();
}

bool UPBUIManagerSubsystem::IsUIActive(TSubclassOf<UPBWidgetBase> WidgetClass) const
{
	for (const TObjectPtr<UPBWidgetBase>& Widget : UIStack)
	{
		if (IsValid(Widget) && Widget->IsA(WidgetClass))
		{
			return true;
		}
	}
	return false;
}

UPBWidgetBase* UPBUIManagerSubsystem::GetTopWidget() const
{
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]))
		{
			return UIStack[i];
		}
	}
	return nullptr;
}

void UPBUIManagerSubsystem::UpdateInputMode()
{
	CleanupInvalidEntries();

	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	// 스택 위에서부터 InputMode != None인 첫 엔트리 탐색
	EPBUIInputMode EffectiveMode = EPBUIInputMode::None;
	bool bEffectiveCursor = false;
	for (int32 i = UIStack.Num() - 1; i >= 0; --i)
	{
		if (IsValid(UIStack[i]) && UIStack[i]->InputMode != EPBUIInputMode::None)
		{
			EffectiveMode = UIStack[i]->InputMode;
			bEffectiveCursor = UIStack[i]->bShowMouseCursor;
			break;
		}
	}

	switch (EffectiveMode)
	{
	case EPBUIInputMode::UIOnly:
		{
			FInputModeUIOnly InputMode;
			if (UPBWidgetBase* Top = GetTopWidget())
			{
				InputMode.SetWidgetToFocus(Top->TakeWidget());
			}
			PC->SetInputMode(InputMode);
		}
		break;

	case EPBUIInputMode::GameAndUI:
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
		break;

	case EPBUIInputMode::None:
	default:
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
		break;
	}

	PC->bShowMouseCursor = bEffectiveCursor;
}

void UPBUIManagerSubsystem::CleanupInvalidEntries()
{
	UIStack.RemoveAll([](const TObjectPtr<UPBWidgetBase>& Widget)
	{
		return !IsValid(Widget) || !Widget->IsInViewport();
	});
}
