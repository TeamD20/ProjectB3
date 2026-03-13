// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBProfileWidget.generated.h"

class UPBPartyMemberViewModel;
class UImage;
class UProgressBar;
class UTextBlock;
class UButton;

/** 좌측 프로필 및 인벤토리 버튼 위젯 */
UCLASS(Abstract)
class PROJECTB3_API UPBProfileWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 특정 캐릭터의 ViewModel을 주입받아 바인딩 수행
	UFUNCTION(BlueprintCallable, Category = "UI|Profile")
	void InitializeProfile(UPBPartyMemberViewModel* InViewModel);

protected:
	virtual void NativeDestruct() override;

	// ViewModel 이벤트 핸들러
	UFUNCTION()
	void OnHPPercentChanged(float NewPercent);
	
	UFUNCTION()
	void OnHPTextChanged(FText HPText);
	
	UFUNCTION()
	void OnPortraitImageChanged(TSoftObjectPtr<UTexture2D> NewPortrait);

protected: 
	// 초상화 이미지
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PortraitImage;

	// 체력 프로그레스 바 (원형 혹은 가로바)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HPProgressBar;

	// 체력 숫자 텍스트 (예: 62/62)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HPText;

private:
	// 현재 구독 중인 뷰모델 포인터
	UPROPERTY(Transient)
	TObjectPtr<UPBPartyMemberViewModel> CurrentViewModel;

	// 델리게이트 핸들 보관
	FDelegateHandle HPPercentChangedHandle;
	FDelegateHandle HPTextChangedHandle;
	FDelegateHandle PortraitChangedHandle;
};
