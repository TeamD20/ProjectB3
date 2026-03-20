// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "PBPartyAIController.generated.h"

class APBPartyAIController;
class UPBPartyFollowSubsystem;

/** 파티 추적 AIC 내부 이동 상태 */
UENUM(BlueprintType)
enum class EPBPartyMoveState : uint8
{
	// 대기 중
	Idle,
	// Trail Point로 이동 중
	MovingToTrail,
	// Scatter 위치로 이동 중
	MovingToScatter,
};

/** 이동 완료(성공/실패) 통보 델리게이트 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPartyMoveCompleted, APBPartyAIController* /*AIC*/, bool /*bSuccess*/);

/**
 * 파티 팔로워 전용 AIController.
 * 팔로워 간 실시간 회피는 UCrowdFollowingComponent가 처리한다.
 */
UCLASS()
class PROJECTB3_API APBPartyAIController : public AAIController
{
	GENERATED_BODY()

public:
	APBPartyAIController(const FObjectInitializer& OI);

	/*~ 서브시스템 명령 인터페이스 ~*/
	// Trail Point로 이동 명령
	void MoveToTrailPoint(const FVector& TrailPoint);

	// Scatter 위치로 이동 명령
	void MoveToScatterPosition(const FVector& ScatterPos);

	// 현재 이동 즉시 중단
	void StopFollowMove();

	// 현재 이동 상태 반환
	EPBPartyMoveState GetMoveState() const { return MoveState; }

	/*~ 이동 완료 델리게이트 ~*/
	// 이동 성공/실패 콜백 (서브시스템이 바인딩)
	FOnPartyMoveCompleted OnPartyMoveCompleted;

protected:
	/*~ AAIController Interface ~*/
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/*~ AAIController 이동 콜백 ~*/
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	// 공통 이동 실행 헬퍼
	void ExecuteMoveTo(const FVector& Destination, EPBPartyMoveState NewState);

	// 현재 이동 상태
	EPBPartyMoveState MoveState = EPBPartyMoveState::Idle;

	// 현재 이동 요청 ID (중복 완료 콜백 방지)
	FAIRequestID ActiveMoveRequestID;
};
