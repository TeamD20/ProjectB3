// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PBWidgetBase.h"
#include "PBUIManagerSubsystem.generated.h"

/**
 * UI 스택 매니저 서브시스템
 * Push/Pop 기반으로 위젯을 관리하고, 스택 최상위 위젯의 InputMode를 자동 적용한다.
 */
UCLASS()
class PROJECTB3_API UPBUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// 위젯을 스택에 Push
	UFUNCTION(BlueprintCallable, Category = "UI", meta = (DeterminesOutputType = "WidgetClass"))
	UPBWidgetBase* PushUI(TSubclassOf<UPBWidgetBase> WidgetClass);

	// 위젯을 스택에서 Pop (nullptr이면 최상위)
	UFUNCTION(BlueprintCallable, Category = "UI")
	void PopUI(UPBWidgetBase* Instance = nullptr);

	// 스택에 특정 클래스 위젯이 있는지
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsUIActive(TSubclassOf<UPBWidgetBase> WidgetClass) const;

	// 스택 최상위 위젯 조회
	UFUNCTION(BlueprintPure, Category = "UI")
	UPBWidgetBase* GetTopWidget() const;

private:
	// 스택 최상위 InputMode를 PlayerController에 적용
	void UpdateInputMode();

	// 외부에서 제거된 위젯 정리
	void CleanupInvalidEntries();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPBWidgetBase>> UIStack;
};

/** TODO [고도화] - GameplayTag 기반 레이어 시스템:
 *  PBWidgetBase의 LayerTag를 읽어 ZOrder를 자동 결정한다.
 *  레이어 간 정책(블러, 입력 차단 등)은 서브시스템이 중앙에서 집행한다.
 *
 * TODO [고도화] - 기타:
 *  - 위젯 캐싱 (자주 열리는 패널 재사용)
 *  - 비동기 로딩 지원
 */