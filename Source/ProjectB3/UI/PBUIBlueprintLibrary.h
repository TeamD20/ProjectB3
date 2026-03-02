// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectB3/Utils/PBBlueprintTypes.h"
#include "PBUIBlueprintLibrary.generated.h"

class UPBWidgetBase;
class UPBUIManagerSubsystem;
class UPBViewModelBase;
class UPBViewModelSubsystem;

/**
 * UI / ViewModel 헬퍼 함수 라이브러리
 */
UCLASS()
class PROJECTB3_API UPBUIBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*~ UI Manager ~*/

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (ExpandEnumAsExecs = "Result"))
	static UPBUIManagerSubsystem* GetUIManager(APlayerController* OwningPlayer, EPBValidResult& Result);
	static UPBUIManagerSubsystem* GetUIManager(APlayerController* OwningPlayer);
	static UPBUIManagerSubsystem* GetUIManager(const ULocalPlayer* LocalPlayer);

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (DeterminesOutputType = "WidgetClass", ExpandEnumAsExecs = "Result"))
	static UPBWidgetBase* PushUI(APlayerController* OwningPlayer, TSubclassOf<UPBWidgetBase> WidgetClass, EPBValidResult& Result);
	static UPBWidgetBase* PushUI(APlayerController* OwningPlayer, TSubclassOf<UPBWidgetBase> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "UI")
	static void PopUI(APlayerController* OwningPlayer, UPBWidgetBase* WidgetInstance = nullptr);

	/*~ ViewModel Subsystem ~*/

	UFUNCTION(BlueprintCallable, Category = "ViewModel", meta = (ExpandEnumAsExecs = "Result"))
	static UPBViewModelSubsystem* GetViewModelSubsystem(APlayerController* OwningPlayer, EPBValidResult& Result);
	static UPBViewModelSubsystem* GetViewModelSubsystem(APlayerController* OwningPlayer);
	static UPBViewModelSubsystem* GetViewModelSubsystem(const ULocalPlayer* LocalPlayer);

	/*~ Global ViewModel ~*/

	UFUNCTION(BlueprintCallable, Category = "ViewModel|Global", meta = (DeterminesOutputType = "ViewModelClass", ExpandEnumAsExecs = "Result"))
	static UPBViewModelBase* GetOrCreateGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBValidResult& Result);
	static UPBViewModelBase* GetOrCreateGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass);
	static UPBViewModelBase* GetOrCreateGlobalViewModel(const ULocalPlayer* LocalPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass);

	UFUNCTION(BlueprintCallable, Category = "ViewModel|Global", meta = (DeterminesOutputType = "ViewModelClass", ExpandEnumAsExecs = "Result"))
	static UPBViewModelBase* GetGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBFoundResult& Result);
	static UPBViewModelBase* GetGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass);
	static UPBViewModelBase* GetGlobalViewModel(const ULocalPlayer* LocalPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass);

	/*~ Actor-Bound ViewModel ~*/

	UFUNCTION(BlueprintCallable, Category = "ViewModel|Actor", meta = (DeterminesOutputType = "ViewModelClass", ExpandEnumAsExecs = "Result"))
	static UPBViewModelBase* GetOrCreateActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBValidResult& Result);
	static UPBViewModelBase* GetOrCreateActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass);
	static UPBViewModelBase* GetOrCreateActorViewModel(const ULocalPlayer* LocalPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass);

	UFUNCTION(BlueprintCallable, Category = "ViewModel|Actor", meta = (DeterminesOutputType = "ViewModelClass", ExpandEnumAsExecs = "Result"))
	static UPBViewModelBase* FindActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBFoundResult& Result);
	static UPBViewModelBase* FindActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass);
	static UPBViewModelBase* FindActorViewModel(const ULocalPlayer* LocalPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass);

	/*~ C++ Template Helpers (by PlayerController) ~*/

	template<typename T>
	static T* GetOrCreateGlobalViewModel(APlayerController* OwningPlayer)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(GetOrCreateGlobalViewModel(OwningPlayer, T::StaticClass()));
	}

	template<typename T>
	static T* GetGlobalViewModel(APlayerController* OwningPlayer)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(GetGlobalViewModel(OwningPlayer, T::StaticClass()));
	}

	template<typename T>
	static T* GetOrCreateActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(GetOrCreateActorViewModel(OwningPlayer, TargetActor, T::StaticClass()));
	}

	template<typename T>
	static T* FindActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(FindActorViewModel(OwningPlayer, TargetActor, T::StaticClass()));
	}

	/*~ C++ Template Helpers (by LocalPlayer) ~*/

	template<typename T>
	static T* GetOrCreateGlobalViewModel(const ULocalPlayer* LocalPlayer)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(GetOrCreateGlobalViewModel(LocalPlayer, T::StaticClass()));
	}

	template<typename T>
	static T* GetGlobalViewModel(const ULocalPlayer* LocalPlayer)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(GetGlobalViewModel(LocalPlayer, T::StaticClass()));
	}

	template<typename T>
	static T* GetOrCreateActorViewModel(const ULocalPlayer* LocalPlayer, AActor* TargetActor)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(GetOrCreateActorViewModel(LocalPlayer, TargetActor, T::StaticClass()));
	}

	template<typename T>
	static T* FindActorViewModel(const ULocalPlayer* LocalPlayer, AActor* TargetActor)
	{
		static_assert(TIsDerivedFrom<T, UPBViewModelBase>::Value, "T must derive from UPBViewModelBase");
		return Cast<T>(FindActorViewModel(LocalPlayer, TargetActor, T::StaticClass()));
	}

private:
	static ULocalPlayer* GetLocalPlayerFromController(APlayerController* OwningPlayer);
};
