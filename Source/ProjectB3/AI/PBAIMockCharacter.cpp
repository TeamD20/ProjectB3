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

    // HP 초기화
    FGameplayModifierInfo HPModInfo;
    HPModInfo.Attribute = UPBAIMockAttributeSet::GetHealthAttribute();
    HPModInfo.ModifierOp =
        EGameplayModOp::Override; // 기존 값 무시하고 덮어쓰기

    FScalableFloat HPScalableFloat;
    HPScalableFloat.SetValue(StartingHP);
    HPModInfo.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(HPScalableFloat);

    InitGE->Modifiers.Add(HPModInfo);

    // Action (AP) 초기화: 1.0f
    FGameplayModifierInfo ActionModInfo;
    ActionModInfo.Attribute = UPBAIMockAttributeSet::GetActionAttribute();
    ActionModInfo.ModifierOp = EGameplayModOp::Override;
    FScalableFloat ActionScalableFloat;
    ActionScalableFloat.SetValue(1.0f);
    ActionModInfo.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(ActionScalableFloat);
    InitGE->Modifiers.Add(ActionModInfo);

    // MaxAction 초기화: 1.0f
    FGameplayModifierInfo MaxActionModInfo;
    MaxActionModInfo.Attribute = UPBAIMockAttributeSet::GetMaxActionAttribute();
    MaxActionModInfo.ModifierOp = EGameplayModOp::Override;
    FScalableFloat MaxActionScalableFloat;
    MaxActionScalableFloat.SetValue(1.0f);
    MaxActionModInfo.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(MaxActionScalableFloat);
    InitGE->Modifiers.Add(MaxActionModInfo);

    // BonusAction 초기화: 1.0f
    FGameplayModifierInfo BonusActionModInfo;
    BonusActionModInfo.Attribute =
        UPBAIMockAttributeSet::GetBonusActionAttribute();
    BonusActionModInfo.ModifierOp = EGameplayModOp::Override;
    FScalableFloat BonusActionScalableFloat;
    BonusActionScalableFloat.SetValue(1.0f);
    BonusActionModInfo.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(BonusActionScalableFloat);
    InitGE->Modifiers.Add(BonusActionModInfo);

    // MaxBonusAction 초기화: 1.0f
    FGameplayModifierInfo MaxBonusActionModInfo;
    MaxBonusActionModInfo.Attribute =
        UPBAIMockAttributeSet::GetMaxBonusActionAttribute();
    MaxBonusActionModInfo.ModifierOp = EGameplayModOp::Override;
    FScalableFloat MaxBonusActionScalableFloat;
    MaxBonusActionScalableFloat.SetValue(1.0f);
    MaxBonusActionModInfo.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(MaxBonusActionScalableFloat);
    InitGE->Modifiers.Add(MaxBonusActionModInfo);

    // Movement 초기화: 900.0f (9m)
    FGameplayModifierInfo MovementModInfo;
    MovementModInfo.Attribute = UPBAIMockAttributeSet::GetMovementAttribute();
    MovementModInfo.ModifierOp = EGameplayModOp::Override;
    FScalableFloat MovementScalableFloat;
    MovementScalableFloat.SetValue(900.0f);
    MovementModInfo.ModifierMagnitude =
        FGameplayEffectModifierMagnitude(MovementScalableFloat);
    InitGE->Modifiers.Add(MovementModInfo);

    AbilitySystemComponent->ApplyGameplayEffectToSelf(
        InitGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
  }
}
