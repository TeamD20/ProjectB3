// PBGE_RestoreTurnResources.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PBGE_RestoreTurnResources.generated.h"

/**
 * 턴이 시작될 때마다 Action, BonusAction, Movement 자원을 최대치(Max)로
 * 회복시켜주는 유틸리티 GameplayEffect입니다.
 * C++에서 초기화되어 에디터에서 블루프린트로 활용할 수 있습니다.
 */
UCLASS(Blueprintable)
class PROJECTB3_API UPBGE_RestoreTurnResources : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPBGE_RestoreTurnResources();
};
