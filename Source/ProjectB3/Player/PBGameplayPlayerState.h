// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerState.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "PBGameplayPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectedPartyMemberChanged, AActor*);
DECLARE_MULTICAST_DELEGATE(FOnPartyMembersChanged);

// 플레이어 스테이트 기반 클래스.
UCLASS()
class PROJECTB3_API APBGameplayPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/*~ APBGameplayPlayerState Interface ~*/
	// 파티원 목록에 액터를 추가한다.
	void AddPartyMember(AActor* PartyMember);

	// 파티원 목록에서 액터를 제거한다.
	void RemovePartyMember(AActor* PartyMember);

	// 현재 선택된 파티원을 변경하고 델리게이트를 브로드캐스트한다.
	void SelectPartyMember(AActor* PartyMember);

	// 파티원 목록을 반환한다.
	TArray<AActor*> GetPartyMembers() const;

	// 현재 선택된 파티원을 반환한다.
	AActor* GetSelectedPartyMember() const;

	// 현재 선택된 파티원의 ASC에 어빌리티 발동을 요청한다.
	bool RequestAbilityActivation(FGameplayAbilitySpecHandle AbilityHandle) const;

public:
	// 선택된 파티원 변경 델리게이트
	FOnSelectedPartyMemberChanged OnSelectedPartyMemberChanged;

	// 파티원 목록 변경 델리게이트
	FOnPartyMembersChanged OnPartyMembersChanged;

	// 파티원 전환 시 카메라 블렌딩 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Party", meta = (ClampMin = 0.0f))
	float PartyMemberCameraBlendTime = 0.5f;

private:
	// 현재 파티원 목록
	TArray<TWeakObjectPtr<AActor>> PartyMembers;

	// 현재 선택된 파티원
	TWeakObjectPtr<AActor> SelectedPartyMember;
};
