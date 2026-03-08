// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBPartyMemberViewmodel.h"


FText UPBPartyMemberViewModel::GetCharacterName() const
{
	return CharacterName;
}

FText UPBPartyMemberViewModel::GetCharacterLevel() const
{
	
	return FText::AsNumber(Level);
}

FText UPBPartyMemberViewModel::GetCharacterClass() const
{
	return CharacterClass;
}

FText UPBPartyMemberViewModel::GetCharacterHPText() const
{
	FText Current = FText::AsNumber(CurrentHP);
	FText Max = FText::AsNumber(MaxHP);
	
	FText CombinedText = FText::Format(
		NSLOCTEXT("UI", "HPText", "{0} / {1}"), 
		Current, 
		Max
	);
	
	return CombinedText;
}

TSoftObjectPtr<UTexture2D> UPBPartyMemberViewModel::GetPortrait() const
{
	return TSoftObjectPtr<UTexture2D>(Portrait);
}

float UPBPartyMemberViewModel::GetHealthPercent() const
{
	return HealthPercent;
}

bool UPBPartyMemberViewModel::IsMyTurn() const
{
	return bIsMyTurn;
}

void UPBPartyMemberViewModel::SetCharacterName(FText InCharacterName)
{
	if (!CharacterName.EqualTo(InCharacterName))
	{
		CharacterName = InCharacterName;
	
		if (OnNameChanged.IsBound())
		{
			OnNameChanged.Broadcast(GetCharacterName());
		}
	}
}

void UPBPartyMemberViewModel::SetLevel(int32 InCharacterLevel)
{
	if (Level != InCharacterLevel)
	{
		Level = InCharacterLevel;
		
		if (OnLevelChanged.IsBound())
		{
			OnLevelChanged.Broadcast(GetCharacterLevel());
		}
	}
}

void UPBPartyMemberViewModel::SetCharacterClass(FText InCharacterClass)
{
	if (!CharacterClass.EqualTo(InCharacterClass))
	{
		CharacterClass = InCharacterClass;
	
		if (OnClassChanged.IsBound())
		{
			OnClassChanged.Broadcast(GetCharacterClass());
		}
	}
}

void UPBPartyMemberViewModel::SetHP(int32 InCurrentHP, int32 InMaxHP)
{
	if (CurrentHP != InCurrentHP || MaxHP != InMaxHP)
	{
		CurrentHP = InCurrentHP;
		MaxHP = InMaxHP;
		
		if (OnHPChanged.IsBound())
		{
			OnHPChanged.Broadcast(GetCharacterHPText());
		}
		
		float Percent = 0.f;
		if (!FMath::IsNearlyEqual(MaxHP,0.f))
		{
			Percent = CurrentHP / MaxHP; 
		}
		
		SetHealthPercent(Percent);
	}
}

void UPBPartyMemberViewModel::SetHealthPercent(float InHealthPercent)
{
	if (HealthPercent != InHealthPercent)
	{
		HealthPercent = InHealthPercent;
		
		if (OnHPPercentValueChanged.IsBound())
		{
			OnHPPercentValueChanged.Broadcast(InHealthPercent);
		}
	}
}

void UPBPartyMemberViewModel::SetPortrait(TSoftObjectPtr<UTexture2D> InPortrait)
{
	if (Portrait != InPortrait)
	{
		Portrait = InPortrait;
		
		if (OnPortraitChanged.IsBound())
		{
			OnPortraitChanged.Broadcast(InPortrait);
		}
	}
}

void UPBPartyMemberViewModel::SetIsMyTurn(bool InMyTurn)
{
	if (bIsMyTurn != InMyTurn)
	{
		bIsMyTurn = InMyTurn;
		
		if (OnIsMyTurnChanged.IsBound())
		{
			OnIsMyTurnChanged.Broadcast(InMyTurn);
		}
	}
}

