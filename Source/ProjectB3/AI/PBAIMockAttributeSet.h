// PBAIMockAttributeSet.h
#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "PBAIMockAttributeSet.generated.h"

// 매크로를 이용해 속성의 Get, Set, Init 헬퍼 함수를 자동 생성합니다.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)                           \
  GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)                   \
  GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)                                 \
  GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)                                 \
  GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTB3_API UPBAIMockAttributeSet : public UAttributeSet {
  GENERATED_BODY()

  /*~ 생성자 ~*/
public:
  UPBAIMockAttributeSet();

  /*~ Attribute Accessors ~*/
public:
  ATTRIBUTE_ACCESSORS(UPBAIMockAttributeSet, Health)
  ATTRIBUTE_ACCESSORS(UPBAIMockAttributeSet, ActionPoints)

  /*~ Attributes ~*/
public:
  UPROPERTY(BlueprintReadOnly, Category = "Mock Attributes")
  FGameplayAttributeData Health;

  UPROPERTY(BlueprintReadOnly, Category = "Mock Attributes")
  FGameplayAttributeData ActionPoints;
};
