// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBProjectile.h"
#include "GameplayEffectTypes.h"
#include "PBArrowProjectile.generated.h"

// 화살 투사체. Bezier 곡선 비행 후 타겟 도착 시 데미지 이펙트를 적용하고 어빌리티에 알린다.
// !!!! DEPRECATED, APBProjectile Base 클래스 사용할 것. !!!!!
UCLASS()
class PROJECTB3_API APBArrowProjectile : public APBProjectile
{
	GENERATED_BODY()
};
