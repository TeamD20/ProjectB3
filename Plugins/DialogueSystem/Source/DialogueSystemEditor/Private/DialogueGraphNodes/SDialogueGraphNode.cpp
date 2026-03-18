// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueGraphNodes/SDialogueGraphNode.h"

#include "DialogueData.h"
#include "DialogueNode.h"
#include "EdGraph/EdGraphNode.h"
#include "SGraphPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "EdGraphSchema_K2.h"
#include "DialogueFeatures/DNodeFeature_Text.h"
#include "DialogueGraphNodes/DialogueGraphNode_Base.h"
#include "Slate/DeferredCleanupSlateBrush.h"

void SDialogueGraphNode::Construct(const FArguments& InArgs, UEdGraphNode* InNode)
{
    GraphNode = InNode;
    TitleBorderMargin = FMargin(8.f, 6.f, 8.f, 6.f);
    UpdateGraphNode();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SDialogueGraphNode::CreateNodeContentArea()
{
   UDialogueGraphNode_Base* DialogueGraphNode = Cast<UDialogueGraphNode_Base>(GraphNode);
	if (!DialogueGraphNode)
	{
		return SDialogueGraphNode::CreateNodeContentArea();
	}
	
	FText CustomText;
	FLinearColor TextColor = FColor::White;

	// TextColor를 미리보기에 적용
	if (auto DialogueNode = DialogueGraphNode->DialogueNode)
	{
		if (auto TextFeature =  DialogueNode->FindFeatureByClass<UDNodeFeature_Text>())
		{
			FString TextString = TextFeature->GetDialogueText().ToString();
			if (TextString.Len() > 50) {
				TextString = TextString.Left(50) + TEXT("...");
			}
			CustomText = FText::FromString(TextString);
			TextColor = TextFeature->GetTextColor();
		}
	}

	TSharedRef<SHorizontalBox> AdditionalContent = SNew(SHorizontalBox);
	{
		// 텍스트가 비어있지 않으면 표시
		if (!CustomText.IsEmpty())
		{
			FSlateFontInfo Font = FAppStyle::GetFontStyle("Graph.Node.NodeTitleFont");
			Font.Size = 10;
			
			AdditionalContent->AddSlot()
			.MaxWidth(220.f)
			.Padding(FMargin(8.f, 5.f))
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(CustomText)
				.WrapTextAt(200.f)
				.Justification(ETextJustify::Left)
				.Font(Font)
				.ColorAndOpacity(FSlateColor(TextColor))
			];
		}
	}
	
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SGraphNode::CreateNodeContentArea()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			AdditionalContent
		];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

