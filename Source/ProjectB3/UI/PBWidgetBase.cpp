// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (UI 위젯 공통 활성화 처리)

#include "PBWidgetBase.h"
#include "ViewModel/PBViewModelBase.h"

void UPBWidgetBase::BindVisibilityToViewModel(UPBViewModelBase* ViewModel)
{
	if (!ViewModel)
	{
		return;
	}

	// 중복 바인딩 방지 후 등록
	ViewModel->OnVisibilityChanged.RemoveDynamic(this, &UPBWidgetBase::HandleViewModelVisibilityChanged);
	ViewModel->OnVisibilityChanged.AddUniqueDynamic(this, &UPBWidgetBase::HandleViewModelVisibilityChanged);

	// 현재 상태를 즉시 반영
	HandleViewModelVisibilityChanged(ViewModel->IsVisible());
}

void UPBWidgetBase::UnbindVisibilityFromViewModel(UPBViewModelBase* ViewModel)
{
	if (!ViewModel)
	{
		return;
	}

	ViewModel->OnVisibilityChanged.RemoveDynamic(this, &UPBWidgetBase::HandleViewModelVisibilityChanged);
}

void UPBWidgetBase::HandleViewModelVisibilityChanged(bool bIsVisible)
{
	SetVisibility(bIsVisible ? VMVisibleState : VMHiddenState);
}
