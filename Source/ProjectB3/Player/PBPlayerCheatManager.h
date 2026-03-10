// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "PBPlayerCheatManager.generated.h"

/**
 * 플레이어 치트 매니저.
 * 디버그/테스트용 컨트롤 모드 전환 및 어빌리티 발동 치트 제공.
 */
UCLASS()
class PROJECTB3_API UPBPlayerCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// Movement 모드 진입 및 이동 어빌리티 발동
	UFUNCTION(Exec)
	void EnterMovementMode();

	// FreeMovement 모드 진입 (거리 제한 없음)
	UFUNCTION(Exec)
	void EnterFreeMovementMode();

	// 현재 모드를 None으로 종료
	UFUNCTION(Exec)
	void ExitMode();

	// 이동력 설정 (cm 단위)
	UFUNCTION(Exec)
	void SetMovement(float Value);

	// 인덱스로 파티원을 선택한다.
	UFUNCTION(Exec)
	void SelectPartyMember(int32 Index);
};
