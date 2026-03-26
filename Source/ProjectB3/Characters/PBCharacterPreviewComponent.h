// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PBCharacterPreviewActor.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBCharacterPreviewComponent.generated.h"

class APBCharacterPreviewActor;
class APBCharacterBase;
class UTextureRenderTarget2D;

// 인벤토리 패널에 캐릭터 3D 프리뷰를 제공하는 컴포넌트.
// APBCharacterPreviewActor를 월드 숨김 위치에 스폰하고
// 장비 변경 시 메시를 동기화하는 관리자 역할을 담당한다.
// 모든 설정은 이 컴포넌트에서 관리하므로 별도 Actor BP 파생 클래스가 불필요하다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTB3_API UPBCharacterPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBCharacterPreviewComponent();

	// 캡처 활성화 여부를 토글 (인벤토리 열림/닫힘 시 호출)
	void SetCaptureActive(bool bActive);

	// 캐릭터 프리뷰 렌더 타겟을 반환 (초기화 전이면 nullptr)
	UTextureRenderTarget2D* GetRenderTarget() const;

protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 프리뷰 Actor를 스폰하고 초기 메시·장비를 동기화
	void SpawnPreviewActor();

	// Owner의 장비 슬롯 변경 이벤트 구독
	void BindEquipmentEvents();

	// Owner의 장비 슬롯 변경 이벤트 구독 해제
	void UnbindEquipmentEvents();

	// 장비 부착/제거 시 프리뷰 메시를 재동기화
	UFUNCTION()
	void HandleCharacterEquipmentChanged(const FGameplayTag& SlotTag);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	TSubclassOf<APBCharacterPreviewActor> PreviewActorClass;
	
	// 렌더 타겟 가로 해상도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview", meta = (AllowPrivateAccess = "true", ClampMin = "64"))
	int32 RenderTargetWidth = 270;

	// 렌더 타겟 세로 해상도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview", meta = (AllowPrivateAccess = "true", ClampMin = "64"))
	int32 RenderTargetHeight = 360;
	
	// 프리뷰 전용 Idle 애니메이션 시퀀스 (nullptr 시 T-Pose)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> PreviewIdleSequence;

private:
	// 스폰된 프리뷰 Actor
	UPROPERTY(Transient)
	TObjectPtr<APBCharacterPreviewActor> PreviewActor;

	// 이벤트 구독을 위해 캐시된 CharacterBase (약참조)
	TWeakObjectPtr<APBCharacterBase> CachedCharacterBase;
};
