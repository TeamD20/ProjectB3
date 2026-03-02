// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBTestTurnOrderActor.generated.h"

UCLASS()
class PROJECTB3_API APBTestTurnOrderActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APBTestTurnOrderActor();

protected:
	virtual void BeginPlay() override;

public:	
	// 에디터 테스트용 함수
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Test")
	void TestAdvanceTurn();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Test")
	void TestShuffleTurnOrder();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Test")
	void TestClearViewModel();
};
