#include "PBNavLinkProxy_Jump.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

APBNavLinkProxy_Jump::APBNavLinkProxy_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSmartLinkIsRelevant = true;
	ArcParam = 0.5f;
	TargetZOffset = 150.0f; // 벽 등에 걸리지 않게 도착 지점 Z를 충분히 띄움
}

void APBNavLinkProxy_Jump::BeginPlay()
{
	Super::BeginPlay();

	// 스마트 링크에 도달했을 때의 델리게이트 바인딩
	OnSmartLinkReached.AddDynamic(this, &APBNavLinkProxy_Jump::HandleSmartLinkReached);
}

void APBNavLinkProxy_Jump::HandleSmartLinkReached(AActor* MovingActor, const FVector& DestinationPoint)
{
	ACharacter* Character = Cast<ACharacter>(MovingActor);
	if (IsValid(Character))
	{
		FVector LaunchVelocity = FVector::ZeroVector;
		FVector StartLocation = Character->GetActorLocation();

		FVector AdjustedDestination = DestinationPoint;
		AdjustedDestination.Z += TargetZOffset;

		// 포물선 궤적의 초기 속도를 계산
		bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this,
			LaunchVelocity,
			StartLocation,
			AdjustedDestination,
			GetWorld()->GetGravityZ(),
			ArcParam
		);

		if (bSuccess)
		{
			// 계산된 속도로 캐릭터를 날려보냄
			Character->LaunchCharacter(LaunchVelocity, true, true);
		}
		
		// 스마트 링크 이동이 끝났음을 알리고 기존 경로 추적을 재개
		ResumePathFollowing(MovingActor);
	}
}
