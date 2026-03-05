// PBAIMockCharacter.h
#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PBAIMockCharacter.generated.h"

class UAbilitySystemComponent;
class UPBAIMockAttributeSet;
class UGameplayAbility;

// AI 샌드박스 구동 전용 더미 캐릭터 클래스
UCLASS()
class APBAIMockCharacter : public ACharacter, public IAbilitySystemInterface {
  GENERATED_BODY()

  /*~ 생성자 ~*/
public:
  APBAIMockCharacter();

  /*~ IAbilitySystemInterface ~*/
public:
  virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
  UPBAIMockAttributeSet *GetAttributeSet() const;

  /*~ AActor Interface ~*/
protected:
  virtual void BeginPlay() override;

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
  float StartingHP = 100.0f;

protected:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EnemyAbility")
  TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EnemyAbility")
  TObjectPtr<UPBAIMockAttributeSet> AttributeSet;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EnemyAbility")
  TSubclassOf<class UGameplayAbility> DefaultAttackAbility;
};
