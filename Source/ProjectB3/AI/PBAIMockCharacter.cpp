// PBAIMockCharacter.cpp

#include "PBAIMockCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "PBAIMockAttributeSet.h"


/*~ 생성자 ~*/

APBAIMockCharacter::APBAIMockCharacter() {
  PrimaryActorTick.bCanEverTick = true;

  AbilitySystemComponent =
      CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
  AttributeSet =
      CreateDefaultSubobject<UPBAIMockAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent *APBAIMockCharacter::GetAbilitySystemComponent() const {
  return AbilitySystemComponent;
}

UPBAIMockAttributeSet *APBAIMockCharacter::GetAttributeSet() const {
  return AttributeSet;
}

/*~ AActor Interface ~*/

void APBAIMockCharacter::BeginPlay() {
  Super::BeginPlay();

  // 샌드박스 환경 구동 시작을 알리는 로그 출력
  UE_LOG(
      LogTemp, Display,
      TEXT("=== PB Mock Character [%s] Spawned and Ready for AI Testing ==="),
      *GetName());

  if (IsValid(AbilitySystemComponent)) {
    // 1. ASC 초기화
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    // 2. 공격 스킬 부여
    if (IsValid(DefaultAttackAbility)) {
      AbilitySystemComponent->GiveAbility(
          FGameplayAbilitySpec(DefaultAttackAbility, 1, INDEX_NONE, this));
    }

    // 3.GameplayEffect를 생성하여 속성 초기화 
    UGameplayEffect *InitGE = NewObject<UGameplayEffect>(
        GetTransientPackage(), FName(TEXT("DynamicInitHealthGE")));
    InitGE->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo ModInfo;
    ModInfo.Attribute = UPBAIMockAttributeSet::GetHealthAttribute();
    ModInfo.ModifierOp = EGameplayModOp::Override; // 기존 값 무시하고 덮어쓰기

    FScalableFloat ScalableFloat;
    ScalableFloat.SetValue(StartingHP);
    ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableFloat);

    InitGE->Modifiers.Add(ModInfo);

    AbilitySystemComponent->ApplyGameplayEffectToSelf(
        InitGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
  }
}
