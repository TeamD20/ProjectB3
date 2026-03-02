// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBExampleTurnOrderWidget.h"
#include "PBExampleTurnOrderViewModel.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

UPBExampleTurnOrderWidget::UPBExampleTurnOrderWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputMode = EPBUIInputMode::GameAndUI;
}

TSharedRef<SWidget> UPBExampleTurnOrderWidget::RebuildWidget()
{
	// 화면 상단 중앙에 가로 배치
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.f, 16.f))
		[
			SNew(SVerticalBox)

			// 라운드 표시 (가운데 정렬)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.08f, 0.06f, 0.12f, 0.85f))
				.Padding(FMargin(12.f, 4.f))
				[
					SAssignNew(RoundText, STextBlock)
					.Text(FText::FromString(TEXT("Round 1")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.6f))
					.Justification(ETextJustify::Center)
				]
			]

			// 턴 순서 카드 (가로)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.05f, 0.04f, 0.08f, 0.7f))
				.Padding(4.f)
				[
					SAssignNew(TurnListBox, SHorizontalBox)
				]
			]
		];
}

void UPBExampleTurnOrderWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UPBViewModelSubsystem* VMS = LP->GetSubsystem<UPBViewModelSubsystem>())
		{
			CachedViewModel = Cast<UPBExampleTurnOrderViewModel>(
				VMS->GetOrCreateGlobalViewModel(UPBExampleTurnOrderViewModel::StaticClass()));
		}
	}

	if (CachedViewModel)
	{
		CachedViewModel->OnTurnOrderChanged.AddDynamic(this, &ThisClass::HandleTurnOrderChanged);
		CachedViewModel->OnTurnAdvanced.AddDynamic(this, &ThisClass::HandleTurnAdvanced);
		BindVisibilityToViewModel(CachedViewModel);

		RebuildTurnList();
	}
}

void UPBExampleTurnOrderWidget::NativeDestruct()
{
	if (CachedViewModel)
	{
		CachedViewModel->OnTurnOrderChanged.RemoveDynamic(this, &ThisClass::HandleTurnOrderChanged);
		CachedViewModel->OnTurnAdvanced.RemoveDynamic(this, &ThisClass::HandleTurnAdvanced);
		UnbindVisibilityFromViewModel(CachedViewModel);
		CachedViewModel = nullptr;
	}

	Super::NativeDestruct();
}

void UPBExampleTurnOrderWidget::HandleTurnOrderChanged()
{
	RebuildTurnList();
}

void UPBExampleTurnOrderWidget::HandleTurnAdvanced(int32 NewIndex)
{
	RebuildTurnList();
}

void UPBExampleTurnOrderWidget::RebuildTurnList()
{
	if (!CachedViewModel || !TurnListBox.IsValid())
	{
		return;
	}

	// 라운드 텍스트
	if (RoundText.IsValid())
	{
		RoundText->SetText(CachedViewModel->GetRoundText());
	}

	// 카드 재구성
	TurnListBox->ClearChildren();

	const TArray<FPBTurnOrderEntry>& Entries = CachedViewModel->GetTurnOrder();
	const int32 CurrentIndex = CachedViewModel->GetCurrentTurnIndex();

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		TurnListBox->AddSlot()
			.AutoWidth()
			.Padding(2.f)
			[
				MakeTurnCard(Entries[i], i == CurrentIndex)
			];
	}
}

TSharedRef<SWidget> UPBExampleTurnOrderWidget::MakeTurnCard(const FPBTurnOrderEntry& Entry, bool bIsCurrent)
{
	// 아군/적 테두리 색상
	FLinearColor BorderColor = Entry.bIsAlly
		? FLinearColor(0.2f, 0.6f, 0.3f, 0.9f)
		: FLinearColor(0.7f, 0.2f, 0.2f, 0.9f);

	// 현재 턴 하이라이트
	FLinearColor BgColor = bIsCurrent
		? FLinearColor(0.25f, 0.25f, 0.4f, 0.9f)
		: FLinearColor(0.08f, 0.08f, 0.12f, 0.8f);

	FLinearColor NameColor = bIsCurrent
		? FLinearColor::White
		: FLinearColor(0.6f, 0.6f, 0.6f);

	return SNew(SBox)
		.WidthOverride(90.f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(BorderColor)
			.Padding(2.f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(BgColor)
				.Padding(FMargin(6.f, 8.f))
				[
					SNew(SVerticalBox)

					// 캐릭터 이름
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(Entry.DisplayName)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(NameColor)
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]

					// Initiative 숫자
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d"), Entry.Initiative)))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						.ColorAndOpacity(FLinearColor(0.9f, 0.85f, 0.5f))
						.Justification(ETextJustify::Center)
					]
				]
			]
		];
}
