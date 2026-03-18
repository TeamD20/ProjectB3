// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBCombatLogTypes.h"
#include "PBCombatLogEntryWidget.generated.h"

class UTextBlock;

/** 컴뱃 로그 단일 항목을 표시하는 위젯 */
UCLASS(Abstract)
class PROJECTB3_API UPBCombatLogEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 엔트리 데이터로 텍스트와 색상을 설정한다.
	void SetEntryData(const FPBCombatLogEntry& Entry);

protected:
	// 로그 텍스트 표시 블록
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EntryText;
};
