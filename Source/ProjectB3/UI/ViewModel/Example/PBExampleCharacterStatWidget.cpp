// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBExampleCharacterStatWidget.h"
#include "PBExampleCharacterStatViewModel.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"

UPBExampleCharacterStatWidget::UPBExampleCharacterStatWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EPBUIInputMode::GameAndUI;
}

TSharedRef<SWidget> UPBExampleCharacterStatWidget::RebuildWidget()
{
	// 화면 중앙 팝업 — 반투명 배경 + 고정 크기 패널
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(360.f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.06f, 0.05f, 0.1f, 0.95f))
				.Padding(16.f)
				[
					SAssignNew(RootBox, SVerticalBox)

					// === 헤더: 캐릭터 이름 ===
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.15f, 0.1f, 0.05f, 0.9f))
						.Padding(FMargin(12.f, 8.f))
						.HAlign(HAlign_Center)
						[
							SAssignNew(NameText, STextBlock)
							.Text(FText::FromString(TEXT("---")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
							.ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.5f))
							.Justification(ETextJustify::Center)
						]
					]

					// === 레벨 / AC 행 ===
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						[
							SAssignNew(LevelText, STextBlock)
							.Text(FText::FromString(TEXT("Lv. 1  (Prof. +2)")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(ACText, STextBlock)
							.Text(FText::FromString(TEXT("AC 10")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
							.ColorAndOpacity(FLinearColor(0.5f, 0.7f, 1.f))
						]
					]

					// === HP 섹션 ===
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 12.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 2.f)
						[
							SAssignNew(HPText, STextBlock)
							.Text(FText::FromString(TEXT("HP: 10 / 10")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.HeightOverride(14.f)
							[
								SAssignNew(HPBar, SProgressBar)
								.Percent(1.f)
								.FillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
							]
						]
					]

					// === 능력치 그리드 (3x2) ===
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.04f, 0.03f, 0.07f, 0.8f))
						.Padding(8.f)
						[
							SAssignNew(AbilityScoresBox, SVerticalBox)
						]
					]
				]
			]
		];
}

void UPBExampleCharacterStatWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPBExampleCharacterStatWidget::NativeDestruct()
{
	UnbindFromViewModel();
	Super::NativeDestruct();
}

void UPBExampleCharacterStatWidget::SetTargetActor(AActor* InActor)
{
	UnbindFromViewModel();

	if (!IsValid(InActor))
	{
		return;
	}

	ULocalPlayer* LP = GetOwningLocalPlayer();
	if (!LP)
	{
		return;
	}

	UPBViewModelSubsystem* VMS = LP->GetSubsystem<UPBViewModelSubsystem>();
	if (!VMS)
	{
		return;
	}

	UPBExampleCharacterStatViewModel* VM = Cast<UPBExampleCharacterStatViewModel>(
		VMS->GetOrCreateActorViewModel(InActor, UPBExampleCharacterStatViewModel::StaticClass()));

	if (VM)
	{
		BindToViewModel(VM);
	}
}

void UPBExampleCharacterStatWidget::BindToViewModel(UPBExampleCharacterStatViewModel* InVM)
{
	if (!InVM)
	{
		return;
	}

	CachedViewModel = InVM;
	CachedViewModel->OnStatChanged.AddDynamic(this, &ThisClass::HandleStatChanged);
	CachedViewModel->OnHPChanged.AddDynamic(this, &ThisClass::HandleHPChanged);
	BindVisibilityToViewModel(CachedViewModel);

	RefreshAllStats();
}

void UPBExampleCharacterStatWidget::UnbindFromViewModel()
{
	if (!CachedViewModel)
	{
		return;
	}

	CachedViewModel->OnStatChanged.RemoveDynamic(this, &ThisClass::HandleStatChanged);
	CachedViewModel->OnHPChanged.RemoveDynamic(this, &ThisClass::HandleHPChanged);
	UnbindVisibilityFromViewModel(CachedViewModel);
	CachedViewModel = nullptr;
}

void UPBExampleCharacterStatWidget::HandleStatChanged()
{
	RefreshAllStats();
}

void UPBExampleCharacterStatWidget::HandleHPChanged(int32 NewHP, int32 NewMaxHP)
{
	RefreshHP();
}

void UPBExampleCharacterStatWidget::RefreshAllStats()
{
	if (!CachedViewModel)
	{
		return;
	}

	// 이름
	if (NameText.IsValid())
	{
		NameText->SetText(CachedViewModel->GetCharacterName());
	}

	// 레벨 + Proficiency
	if (LevelText.IsValid())
	{
		FString Str = FString::Printf(TEXT("Lv. %d  (Prof. +%d)"),
			CachedViewModel->GetLevel(),
			CachedViewModel->GetProficiencyBonus());
		LevelText->SetText(FText::FromString(Str));
	}

	// AC
	if (ACText.IsValid())
	{
		ACText->SetText(FText::FromString(
			FString::Printf(TEXT("AC %d"), CachedViewModel->GetArmorClass())));
	}

	// HP
	RefreshHP();

	// 능력치 그리드 (3열 x 2행)
	if (AbilityScoresBox.IsValid())
	{
		AbilityScoresBox->ClearChildren();

		struct FAbilityInfo { FString Label; int32 Score; };
		const FAbilityInfo Abilities[] = {
			{ TEXT("STR"), CachedViewModel->GetStrength() },
			{ TEXT("DEX"), CachedViewModel->GetDexterity() },
			{ TEXT("CON"), CachedViewModel->GetConstitution() },
			{ TEXT("INT"), CachedViewModel->GetIntelligence() },
			{ TEXT("WIS"), CachedViewModel->GetWisdom() },
			{ TEXT("CHA"), CachedViewModel->GetCharisma() },
		};

		// 3열씩 2행으로 배치
		for (int32 Row = 0; Row < 2; ++Row)
		{
			TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

			for (int32 Col = 0; Col < 3; ++Col)
			{
				int32 Idx = Row * 3 + Col;
				const FAbilityInfo& Ability = Abilities[Idx];
				int32 Mod = UPBExampleCharacterStatViewModel::GetAbilityModifier(Ability.Score);

				RowBox->AddSlot()
					.FillWidth(1.f)
					.Padding(2.f)
					[
						MakeAbilityRow(Ability.Label, Ability.Score, Mod)
					];
			}

			AbilityScoresBox->AddSlot()
				.AutoHeight()
				.Padding(0.f, 2.f)
				[
					RowBox
				];
		}
	}
}

void UPBExampleCharacterStatWidget::RefreshHP()
{
	if (!CachedViewModel)
	{
		return;
	}

	if (HPText.IsValid())
	{
		HPText->SetText(CachedViewModel->GetHPText());

		FLinearColor Color = CachedViewModel->IsBloodied()
			? FLinearColor(0.9f, 0.2f, 0.2f)
			: FLinearColor::White;
		HPText->SetColorAndOpacity(Color);
	}

	if (HPBar.IsValid())
	{
		float Ratio = CachedViewModel->GetHPRatio();
		HPBar->SetPercent(Ratio);

		FLinearColor BarColor;
		if (Ratio > 0.5f)
		{
			BarColor = FLinearColor::LerpUsingHSV(
				FLinearColor(1.f, 1.f, 0.f),
				FLinearColor(0.2f, 0.8f, 0.2f),
				(Ratio - 0.5f) * 2.f);
		}
		else
		{
			BarColor = FLinearColor::LerpUsingHSV(
				FLinearColor(0.9f, 0.1f, 0.1f),
				FLinearColor(1.f, 1.f, 0.f),
				Ratio * 2.f);
		}
		HPBar->SetFillColorAndOpacity(BarColor);
	}
}

TSharedRef<SWidget> UPBExampleCharacterStatWidget::MakeAbilityRow(const FString& Label, int32 Score, int32 Modifier)
{
	FString ModStr = (Modifier >= 0)
		? FString::Printf(TEXT("+%d"), Modifier)
		: FString::Printf(TEXT("%d"), Modifier);

	// 세로 카드: 라벨 / 수치 / modifier
	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.08f, 0.07f, 0.12f, 0.9f))
		.Padding(FMargin(8.f, 6.f))
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)

			// 능력치 라벨 (STR, DEX...)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
				.Justification(ETextJustify::Center)
			]

			// 수치
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), Score)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.ColorAndOpacity(FLinearColor::White)
				.Justification(ETextJustify::Center)
			]

			// Modifier
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ModStr))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
				.ColorAndOpacity(FLinearColor(0.5f, 0.8f, 1.f))
				.Justification(ETextJustify::Center)
			]
		];
}
