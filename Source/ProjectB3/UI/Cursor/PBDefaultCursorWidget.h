// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBDefaultCursorWidget.generated.h"

class UTextBlock;

// 기본 커서 위젯. TurnMovement 모드에서 이동 거리(m)를 표시한다.
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBDefaultCursorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이동 거리 표시 갱신 (0 이하 시 숨김)
	void SetDistance(float Meters);

	// 거리 표시 초기화 및 숨김
	void ClearDistance();

protected:
	// 거리 텍스트블록 (BP에서 바인딩)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> DistanceText;
};
