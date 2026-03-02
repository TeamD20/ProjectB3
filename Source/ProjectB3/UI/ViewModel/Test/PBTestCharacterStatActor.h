// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBTestCharacterStatActor.generated.h"

UCLASS()
class PROJECTB3_API APBTestCharacterStatActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APBTestCharacterStatActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Test")
	void TestTakeDamage();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Test")
	void TestLevelUp();

	// 캐릭터 시트 열기
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestOpenStatSheet();

	// 캐릭터 시트 닫기
	UFUNCTION(BlueprintCallable, Category = "Test")
	void TestCloseStatSheet();

private:
	TWeakObjectPtr<class UPBWidgetBase> StatSheetWidget;

	int32 CurrentHP = 42;
	int32 MaxHP = 42;
	int32 CurrentLevel = 5;
};
