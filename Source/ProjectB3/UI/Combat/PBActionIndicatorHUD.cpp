// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBActionIndicatorHUD.h"
#include "PBActionIndicatorViewModel.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Animation/WidgetAnimation.h"
#include "GameFramework/PlayerController.h"

void UPBActionIndicatorHUD::SetupViewModel(UPBActionIndicatorViewModel* InViewModel)
{
	if (ActionViewModel)
	{
		ActionViewModel->OnActionChanged.RemoveAll(this);
	}

	ActionViewModel = InViewModel;

	if (ActionViewModel)
	{
		ActionViewModel->OnActionChanged.AddUObject(this, &UPBActionIndicatorHUD::HandleActionChanged);
		
		// 초기 상태 동기화
		HandleActionChanged(ActionViewModel->GetCurrentAction());
	}
}

void UPBActionIndicatorHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// 게임 시작 시(위젯이 장착될 때) 기본적으로 숨김 처리
	if (IsValid(IndicatorBorder))
	{
		IndicatorBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 뷰모델 자동 바인딩 로직 (BP에서 수동으로 안 해줬을 경우 대비)
	if (!ActionViewModel)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (ULocalPlayer* LP = PC->GetLocalPlayer())
			{
				if (UPBViewModelSubsystem* VMSubsystem = LP->GetSubsystem<UPBViewModelSubsystem>())
				{
					if (APawn* MyPawn = PC->GetPawn())
					{
						if (UPBActionIndicatorViewModel* VM = VMSubsystem->GetOrCreateActorViewModel<UPBActionIndicatorViewModel>(MyPawn))
						{
							SetupViewModel(VM);
						}
					}
				}
			}
		}
	}
}

void UPBActionIndicatorHUD::NativeDestruct()
{
	if (ActionViewModel)
	{
		ActionViewModel->OnActionChanged.RemoveAll(this);
	}
	
	Super::NativeDestruct();
}

void UPBActionIndicatorHUD::HandleActionChanged(const FPBActionIndicatorData& Data)
{
	// 텍스트 갱신
	if (IsValid(ActionText))
	{
		ActionText->SetText(Data.DisplayText);
	}

	// bIsActive 신호에 따라 등장/퇴장 애니메이션 실행
	if (Data.bIsActive)
	{
		PlayStartAnim();
	}
	else
	{
		PlayEndAnim();
	}

	// BP 디자이너에게 알림
	BP_OnActionChanged(Data);
}

void UPBActionIndicatorHUD::PlayStartAnim()
{
	if (IsValid(StartAnim))
	{
		PlayAnimation(StartAnim);
	}
}

void UPBActionIndicatorHUD::PlayEndAnim()
{
	if (IsValid(EndAnim))
	{
		PlayAnimation(EndAnim);
	}
}
