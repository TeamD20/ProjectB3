#pragma once

#include "CoreMinimal.h"
#include "Navigation/NavLinkProxy.h"
#include "PBNavLinkProxy_Jump.generated.h"

/**
 * 캐릭터가 도달했을 때 포물선 궤적으로 점프하게 만드는 커스텀 NavLinkProxy
 */
UCLASS()
class PROJECTB3_API APBNavLinkProxy_Jump : public ANavLinkProxy
{
	GENERATED_BODY()

public:
	APBNavLinkProxy_Jump(const FObjectInitializer& ObjectInitializer);

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;

private:
	// 스마트 링크에 도달했을 때 호출되는 함수 
	UFUNCTION()
	void HandleSmartLinkReached(AActor* MovingActor, const FVector& DestinationPoint);

public:
	// 점프 궤적 계산을 위한 Arc 비율 (0.0: 일직선, 1.0: 높은 포물선)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	float ArcParam;

	// 목표 지점에 추가할 Z축 오프셋 (벽이나 언덕에 걸리지 않도록 여유 높이 제공)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump")
	float TargetZOffset = 250.f;
};
