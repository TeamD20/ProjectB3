// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DNodeFeature.h"
#include "DNodeFeature_Text.generated.h"

UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Dialogue Text"))
class DIALOGUESYSTEMRUNTIME_API UDNodeFeature_Text : public UDNodeFeature
{
	GENERATED_BODY()

public:
	/*~ UDNodeFeature_Text Interface ~*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue Text")
	FText GetDialogueText() const;
	virtual FText GetDialogueText_Implementation() const {return FText();}
	
	virtual FLinearColor GetTextColor() const {return FLinearColor::White;}
};