// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAIController.h"
#include "Components/StateTreeComponent.h"

/*~ 생성자 ~*/

APBAIController::APBAIController()
{
	StateTreeComponent =
		CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComponent"));
}

/*~ AController Interface ~*/

void APBAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsValid(InPawn))
	{
		UE_LOG(LogTemp, Display,
		       TEXT("=== PBAIController가 성공적으로 폰 [%s]에 빙의했습니다. "
			       "StateTree 구동 대기 완료 ==="),
		       *InPawn->GetName());
	}
}
