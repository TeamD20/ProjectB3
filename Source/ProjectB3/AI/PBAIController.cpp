// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAIController.h"
#include "Components/StateTreeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAIController, Log, All);

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
		UE_LOG(LogPBAIController, Display,
		       TEXT("PBAIController가 폰 [%s]에 빙의. StateTree 구동 대기 완료."),
		       *InPawn->GetName());
	}
}
