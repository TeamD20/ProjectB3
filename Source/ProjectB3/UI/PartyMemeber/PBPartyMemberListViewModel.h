


#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/UI/PBUITags.h"
#include "PBPartyMemberListViewModel.generated.h"


class UPBPartyMemberViewModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartyListChangedSignature);

/**
 * 파티 전체 리스트를 관리하는 ViewModel
 */
UCLASS()
class PROJECTB3_API UPBPartyMemberListViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	// 파티 멤버 데이터를 바탕으로 뷰모델 리스트를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "ViewModel|Party")
	void SetPartyMembers(const TArray<AActor*>& InPartyMembers);

	// 현재 생성된 파티 멤버 뷰모델 리스트를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "ViewModel|Party")
	const TArray<UPBPartyMemberViewModel*>& GetMemberViewModels() const { return reinterpret_cast<const TArray<UPBPartyMemberViewModel*>&>(MemberViewModels); }

public:
	// 파티 리스트 구성이 변경되었을 때 위젯에 알리는 프로퍼티
	UPROPERTY(BlueprintAssignable, Category = "ViewModel|Events")
	FOnPartyListChangedSignature OnPartyListChanged;

private:
	// 자식 파티원 개별 뷰모델들을 저장하는 내부 배열
	UPROPERTY()
	TArray<TObjectPtr<UPBPartyMemberViewModel>> MemberViewModels;
};
