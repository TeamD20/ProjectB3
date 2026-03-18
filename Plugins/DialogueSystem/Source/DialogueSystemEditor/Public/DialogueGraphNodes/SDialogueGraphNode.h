// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UEdGraphNode;
class SGraphPanel;
class SNodeTitle;

class SDialogueGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SDialogueGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphNode* InNode);
	virtual TSharedRef<SWidget> CreateNodeContentArea() override;

private:
	TSharedPtr<SLevelOfDetailBranchNode> TitleLODBranchNode;
};
