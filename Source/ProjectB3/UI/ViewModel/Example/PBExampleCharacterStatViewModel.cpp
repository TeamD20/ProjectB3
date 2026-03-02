// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBExampleCharacterStatViewModel.h"
#include "ProjectB3/UI/PBUITags.h"

UPBExampleCharacterStatViewModel::UPBExampleCharacterStatViewModel()
{
	ViewModelTag = PBUITags::UI_ViewModel_CharacterStat;
}

void UPBExampleCharacterStatViewModel::InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer)
{
	Super::InitializeForActor(InTargetActor, InLocalPlayer);

	if (InTargetActor)
	{
		InTargetActor->OnDestroyed.AddDynamic(this, &UPBExampleCharacterStatViewModel::HandleActorDestroyed);
	}
}

void UPBExampleCharacterStatViewModel::Deinitialize()
{
	if (AActor* Target = GetTargetActor())
	{
		Target->OnDestroyed.RemoveDynamic(this, &UPBExampleCharacterStatViewModel::HandleActorDestroyed);
	}

	Super::Deinitialize();
}

void UPBExampleCharacterStatViewModel::SetCharacterName(const FText& InName)
{
	CharacterName = InName;
	OnStatChanged.Broadcast();
}

void UPBExampleCharacterStatViewModel::SetLevel(int32 InLevel)
{
	Level = InLevel;
	OnStatChanged.Broadcast();
}

void UPBExampleCharacterStatViewModel::SetHP(int32 InHP, int32 InMaxHP)
{
	HP = InHP;
	MaxHP = InMaxHP;
	OnHPChanged.Broadcast(HP, MaxHP);
}

void UPBExampleCharacterStatViewModel::SetArmorClass(int32 InAC)
{
	ArmorClass = InAC;
	OnStatChanged.Broadcast();
}

void UPBExampleCharacterStatViewModel::SetAbilityScores(int32 InSTR, int32 InDEX, int32 InCON, int32 InINT, int32 InWIS, int32 InCHA)
{
	Strength = InSTR;
	Dexterity = InDEX;
	Constitution = InCON;
	Intelligence = InINT;
	Wisdom = InWIS;
	Charisma = InCHA;
	OnStatChanged.Broadcast();
}

int32 UPBExampleCharacterStatViewModel::GetAbilityModifier(int32 Score)
{
	return FMath::FloorToInt((Score - 10) / 2.0f);
}

int32 UPBExampleCharacterStatViewModel::GetStrengthModifier() const
{
	return GetAbilityModifier(Strength);
}

FText UPBExampleCharacterStatViewModel::GetFormattedModifier(int32 Score)
{
	int32 Modifier = GetAbilityModifier(Score);
	if (Modifier > 0)
	{
		return FText::Format(FText::FromString(TEXT("+{0}")), FText::AsNumber(Modifier));
	}
	else if (Modifier < 0)
	{
		return FText::AsNumber(Modifier); // 음수는 자체적으로 부호 포함
	}
	else
	{
		return FText::FromString(TEXT("+0"));
	}
}

int32 UPBExampleCharacterStatViewModel::GetProficiencyBonus() const
{
	return FMath::FloorToInt((Level - 1) / 4.0f) + 2;
}

float UPBExampleCharacterStatViewModel::GetHPRatio() const
{
	if (MaxHP <= 0)
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>(HP) / static_cast<float>(MaxHP), 0.0f, 1.0f);
}

FText UPBExampleCharacterStatViewModel::GetHPText() const
{
	return FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(HP), FText::AsNumber(MaxHP));
}

bool UPBExampleCharacterStatViewModel::IsBloodied() const
{
	return HP <= (MaxHP / 2);
}

void UPBExampleCharacterStatViewModel::HandleActorDestroyed(AActor* DestroyedActor)
{
	SetDesiredVisibility(false);
}
