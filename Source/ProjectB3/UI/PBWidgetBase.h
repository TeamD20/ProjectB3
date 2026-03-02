// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBWidgetBase.generated.h"

class UPBViewModelBase;

/**
 * 위젯이 요구하는 입력 모드
 */
UENUM(BlueprintType)
enum class EPBUIInputMode : uint8
{
	None,       // 입력 모드 변경 안함 (HUD, 데미지 넘버)
	GameAndUI,  // 카메라 이동 가능 (인벤토리, 캐릭터 시트)
	UIOnly,     // 게임 입력 차단 (대화, 레벨업)
};

/**
 * UI 위젯 기본 클래스
 * Push/Pop 스택에서 관리되는 모든 위젯의 부모 클래스
 */
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// ViewModel의 OnVisibilityChanged에 이 위젯의 Visibility를 바인딩.
	// 호출 시 현재 ViewModel의 IsVisible() 상태를 즉시 반영한다.
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	void BindVisibilityToViewModel(UPBViewModelBase* ViewModel);

	// ViewModel의 OnVisibilityChanged 바인딩 해제.
	UFUNCTION(BlueprintCallable, Category = "UI|ViewModel")
	void UnbindVisibilityFromViewModel(UPBViewModelBase* ViewModel);

private:
	// ViewModel의 OnVisibilityChanged 델리게이트 콜백
	UFUNCTION()
	void HandleViewModelVisibilityChanged(bool bIsVisible);
	
public:
	// 이 위젯이 활성화될 때 적용할 입력 모드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPBUIInputMode InputMode = EPBUIInputMode::GameAndUI;

	// 이 위젯이 활성화될 때 마우스 커서 표시 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bShowMouseCursor = true;
	
	// ViewModel이 Visible일 때 적용할 Slate Visibility (기본: SelfHitTestInvisible)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI|ViewModel")
	ESlateVisibility VMVisibleState = ESlateVisibility::SelfHitTestInvisible;

	// ViewModel이 Hidden일 때 적용할 Slate Visibility (기본: Collapsed)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI|ViewModel")
	ESlateVisibility VMHiddenState = ESlateVisibility::Collapsed;
};

/** TODO [고도화] - GameplayTag 기반 레이어 시스템:
 *  PBWidgetBase에 FGameplayTag LayerTag 프로퍼티를 추가한다.
 *  UI.Layer.Base, UI.Layer.Panel, UI.Layer.Modal 등을 설정하면
 *  UIManagerSubsystem이 ZOrder 매핑 테이블을 참조하여 AddToViewport(ZOrder)를 자동 적용한다.
 *
 *  레이어 간 정책 예시:
 *    - UI.Layer.Modal 활성화 시: UI.Layer.Base 위젯에 블러 파라미터 주입
 *    - UI.Layer.Modal 활성화 시: UI.Layer.Panel 위젯의 입력 차단
 *    - ESC 입력 시: 활성 LayerTag 스택 최상위 Pop
 *  이 정책은 UIManagerSubsystem 내 Tag 매칭 규칙으로 선언적 정의한다.
 *
 * TODO [고도화] - 기타:
 *  - Push/Pop 진입/퇴장 애니메이션
 *  - ESC 키 닫기 정책 (bClosableByEsc)
 *  - 배경 블러/딤 설정
 */