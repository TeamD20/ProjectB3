// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBBlueprintTypes.generated.h"

/**
 * 오브젝트 유효성 분기용 Exec Pin Enum
 * UFUNCTION의 ExpandEnumAsExecs 메타와 함께 사용한다.
 *
 * 사용 예:
 *   UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "Result"))
 *   static UObject* FindSomething(EPBValidResult& Result);
 */
UENUM(BlueprintType)
enum class EPBValidResult : uint8
{
	Valid,
	Invalid,
};

/**
 * 검색 결과 분기용 Exec Pin Enum
 *
 * 사용 예:
 *   UFUNCTION(BlueprintCallable, meta = (ExpandEnumAsExecs = "Result"))
 *   static UObject* FindSomething(EPBFoundResult& Result);
 */
UENUM(BlueprintType)
enum class EPBFoundResult : uint8
{
	Found,
	NotFound,
};
