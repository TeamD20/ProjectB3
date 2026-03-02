// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "PBCharacterBase.generated.h"

// 플레이어와 AI가 공유하는 캐릭터 기반 클래스.
UCLASS()
class PROJECTB3_API APBCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	APBCharacterBase();

protected:
	virtual void BeginPlay() override;
};
