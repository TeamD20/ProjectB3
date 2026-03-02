// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBViewModelBase.generated.h"

class ULocalPlayer;

// ViewModel 가시성 변경 이벤트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewModelVisibilityChanged, bool, bIsVisible);

/**
 * ViewModel 기본 클래스
 * Global 또는 Actor-Bound 모드로 초기화 가능
 */
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBViewModelBase : public UObject
{
	GENERATED_BODY()

public:
	// Global ViewModel 초기화
	virtual void InitializeForPlayer(ULocalPlayer* InLocalPlayer);

	// Actor-Bound ViewModel 초기화
	virtual void InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer);

	// ViewModel 정리
	virtual void Deinitialize();

	// ViewModel Tag 반환
	UFUNCTION(BlueprintPure, Category = "ViewModel")
	FGameplayTag GetViewModelTag() const { return ViewModelTag; }

	/*~ Visibility ~*/

	// 가시성 설정
	UFUNCTION(BlueprintCallable, Category = "ViewModel|Visibility")
	void SetDesiredVisibility(bool bNewVisible);

	// 가시성 오버라이드 설정
	UFUNCTION(BlueprintCallable, Category = "ViewModel|Visibility")
	void SetVisibilityOverride(bool bOverrideVisible);

	// 가시성 오버라이드 해제
	UFUNCTION(BlueprintCallable, Category = "ViewModel|Visibility")
	void ClearVisibilityOverride();

	// 가시성 반환
	UFUNCTION(BlueprintPure, Category = "ViewModel|Visibility")
	bool IsVisible() const { return bIsVisible; }

	// 가시성 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "ViewModel|Events")
	FOnViewModelVisibilityChanged OnVisibilityChanged;

	// 소유 LocalPlayer 반환
	UFUNCTION(BlueprintPure, Category = "ViewModel")
	ULocalPlayer* GetLocalPlayer() const { return LocalPlayer.Get(); }

	// 바인딩된 Actor 반환 (Global이면 nullptr)
	UFUNCTION(BlueprintPure, Category = "ViewModel")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

	// 소유 PlayerController 반환
	UFUNCTION(BlueprintPure, Category = "ViewModel")
	APlayerController* GetOwningPlayerController() const;

	// Actor-Bound 여부
	UFUNCTION(BlueprintPure, Category = "ViewModel")
	bool IsActorBound() const { return TargetActor.IsValid(); }

	// 바인딩된 Actor 반환 (타입 캐스팅)
	template<typename T>
	T* GetTargetActor() const
	{
		return Cast<T>(GetTargetActor());
	}

	// 소유 PlayerController 반환 (타입 캐스팅)
	template<typename T>
	T* GetOwningPlayerController() const
	{
		return Cast<T>(GetOwningPlayerController());
	}

private:
	// visibility 계산 및 브로드캐스트
	void UpdateEffectiveVisibility();

protected:
	// ViewModel Tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ViewModel")
	FGameplayTag ViewModelTag;

	// 가시성 여부
	UPROPERTY(BlueprintReadOnly, Category = "ViewModel|Visibility")
	bool bIsVisible = false;

private:

	// 가시성 기본값
	bool bDesiredVisibility = false;

	// 가시성 오버라이드 여부
	bool bVisibilityOverrideActive = false;

	// 가시성 오버라이드 값
	bool bVisibilityOverrideValue = false;

	TWeakObjectPtr<ULocalPlayer> LocalPlayer;
	TWeakObjectPtr<AActor> TargetActor;
};
