// PBAIMockCharacter.h
#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "PBAIMockCharacter.generated.h"


class UPBAIArchetypeData;

// AI 샌드박스 구동 전용 더미 캐릭터 클래스.
// CombatIdentity(ClassTag, FactionTag)를 에디터에서 설정하면
// 부모 APBCharacterBase의 GrantInitialAbilities()가 Registry 기반으로 어빌리티를 부여한다.
UCLASS()
class APBAIMockCharacter : public APBCharacterBase
{
	GENERATED_BODY()

	/*~ 생성자 ~*/
public:
	APBAIMockCharacter();

	/*~ AActor Interface ~*/
protected:
	virtual void BeginPlay() override;

public:
	// AI 아키타입 데이터 (공격형/방어형/지원형 등 행동 가중치)
	// 미설정 시 모든 ArchetypeWeight = 1.0 (균등 가중)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UPBAIArchetypeData> ArchetypeData;

	/*~ APBCharacterBase Interface ~*/
protected:
	virtual void HandleGameplayTagUpdated(const FGameplayTag& ChangedTag, bool TagExists) override;

	/*~ IPBCombatParticipant Interface ~*/
public:
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnTurnBegin() override;
	virtual void OnTurnActivated() override;
	virtual void OnActionInterrupted() override;
	virtual float GetBaseMovementSpeed() const override;
};
