// PBAIMockCharacter.h
#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "PBAIMockCharacter.generated.h"


class UPBTurnResourceAttributeSet;
class UGameplayAbility;

// AI 샌드박스 구동 전용 더미 캐릭터 클래스
UCLASS()
class APBAIMockCharacter : public APBCharacterBase
{
	GENERATED_BODY()

	/*~ 생성자 ~*/
public:
	APBAIMockCharacter();

	/*~ IAbilitySystemInterface ~*/


	/*~ AActor Interface ~*/
protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float StartingHP = 100.0f;

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EnemyAbility")
	TSubclassOf<class UGameplayAbility> DefaultAttackAbility;

	/*~ IPBCombatParticipant Interface ~*/
public:
	virtual int32 GetInitiativeModifier() const override;
	virtual bool HasInitiativeAdvantage() const override;
	virtual void OnCombatBegin() override;
	virtual void OnCombatEnd() override;
	virtual void OnRoundBegin() override;
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void OnTurnBegin() override;
	virtual void OnTurnEnd() override;
	virtual bool CanReact() const override;
	virtual void
	OnReactionOpportunity(const FPBReactionContext& Context) override;
	virtual void OnActionInterrupted() override;
	virtual bool IsIncapacitated() const override;
	virtual FGameplayTag GetFactionTag() const override;
	virtual float GetBaseMovementSpeed() const override;
	virtual FText GetCombatDisplayName() const override;
	virtual TSoftObjectPtr<UTexture2D> GetCombatPortrait() const override;
};
