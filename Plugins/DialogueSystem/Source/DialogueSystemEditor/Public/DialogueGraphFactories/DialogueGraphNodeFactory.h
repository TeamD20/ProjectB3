// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "EdGraphUtilities.h"
#include "SGraphPin.h"

struct FDialogueGraphNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};

