// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBPartyMemberWidget.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBPartyMemberListWidget.generated.h"

class UVerticalBox;
class UPBPartyMemberListViewModel;
class UPBPartyMemberViewModel;
class UImage;
class UTexture2D;

/**
 * 파티 리스트를 표시하며 파티원 수에 따라 위젯을 동적으로 생성하는 위젯 클래스
 */
UCLASS()
class PROJECTB3_API UPBPartyMemberListWidget : public UPBWidgetBase
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable, Category = "UI|Party")
	void InitializeBinding(UPBPartyMemberListViewModel* InViewModel);
	
protected:
	// 리스트 변경 시 호출되는 핸들러
	UFUNCTION()
	void HandlePartyListChanged();

	// 개별 파티원 위젯을 생성하고 리스트에 추가합니다.
	void CreatePartyMemberWidget(UPBPartyMemberViewModel* MemberVM);

public:
	// 위젯 리스트가 담길 박스 (BP에서 바인딩)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MemberListBox;

	// 파티원 리스트 뒤에 표시될 배경 이미지 (BP에서 바인딩, 없어도 크래시 안 나도록 BindWidgetOptional 사용)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	// 배경 이미지를 C++에서 동적으로 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Party")
	void SetBackgroundImage(UTexture2D* NewTexture);

	// 배경 이미지의 색상/투명도를 C++에서 변경하는 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Party")
	void SetBackgroundColor(FLinearColor NewColor);

	// 생성할 파티원 위젯의 클래스 (BP에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Party")
	TSubclassOf<UPBPartyMemberWidget> MemberWidgetClass;

private:
	// 바인딩된 뷰모델 참조
	UPROPERTY()
	TObjectPtr<UPBPartyMemberListViewModel> ListViewModel;
	
};
