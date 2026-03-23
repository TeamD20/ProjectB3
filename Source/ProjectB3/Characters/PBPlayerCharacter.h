// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "PBCharacterBase.h"
#include "PBPlayerCharacter.generated.h"

class UCameraComponent;
class APBGameplayPlayerController;
class USpringArmComponent;
class UPBCharacterPreviewComponent;
class APBPartyAIController;

// 플레이어 전용 캐릭터. SpringArm과 Camera를 소유한다.
UCLASS()
class PROJECTB3_API APBPlayerCharacter : public APBCharacterBase
{
	GENERATED_BODY()

public:
	APBPlayerCharacter();
	
	// SpringArm 컴포넌트 반환
	USpringArmComponent* GetSpringArm() const { return SpringArmComponent; }

	// Camera 컴포넌트 반환
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	
	/*~ APawn Interface ~*/
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	
	/*~ IPBCombatParticipant Interface ~*/
	virtual void OnCombatBegin() override;
	virtual void OnCombatEnd() override;
	virtual void OnTurnActivated() override;
	virtual void OnTurnEnd() override;

	void UpdatePathDisplayMovementRange(APBGameplayPlayerController* PC);
	
private:
	// 카메라 붐 (SpringArm)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	// 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// 인벤토리 UI용 캐릭터 3D 프리뷰 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPBCharacterPreviewComponent> CharacterPreviewComponent;

public:
	// 플레이어 캐릭터 전용 파티 추적 AI 컨트롤러
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<APBPartyAIController> PartyAIController;
};
