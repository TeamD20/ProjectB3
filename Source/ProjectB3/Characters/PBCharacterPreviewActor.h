// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBCharacterPreviewActor.generated.h"

class UDirectionalLightComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UTextureRenderTarget2D;

// APBCharacterPreviewActor 초기화 시 외부(UPBCharacterPreviewComponent)에서 주입하는 설정값 묶음
USTRUCT(BlueprintType)
struct FPBPreviewActorConfig
{
	GENERATED_BODY()

	// 렌더 타겟 가로 해상도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (ClampMin = "64"))
	int32 RenderTargetWidth = 210;

	// 렌더 타겟 세로 해상도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (ClampMin = "64"))
	int32 RenderTargetHeight = 280;

	// 프리뷰 전용 Idle 애니메이션 시퀀스 (nullptr 시 T-Pose)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	TObjectPtr<UAnimSequence> PreviewIdleSequence = nullptr;
};

// 인벤토리 프리뷰 전용 Actor.
// 월드 숨김 위치(Z=-50000)에 스폰되어 캐릭터 메시를 고정 포즈로 촬영한다.
// 모든 설정은 UPBCharacterPreviewComponent가 ApplyConfig()로 주입하므로 별도 BP 파생 클래스가 불필요하다.
UCLASS()
class PROJECTB3_API APBCharacterPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	APBCharacterPreviewActor();

	// 외부에서 설정값을 주입하고 캡처를 초기화 — 스폰 직후 반드시 1회 호출
	void ApplyConfig(const FPBPreviewActorConfig& InConfig);

	// 기본 캐릭터 메시를 설정하고 프리뷰 애니메이션을 재생
	void SetBaseMesh(USkeletalMesh* Mesh);

	// SourceActor의 모든 MeshComponent를 읽어 프리뷰에 복제
	// 장비 변경 시 기존 복제본을 제거하고 재생성한다
	void SyncMeshesFrom(AActor* SourceActor);

	// 캡처 활성화 여부를 설정
	void SetCaptureActive(bool bActive);

	// 렌더 타겟을 반환 (ApplyConfig 호출 전이면 nullptr)
	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

private:
	// RenderTarget 및 SceneCapture를 Config 기반으로 초기화
	void InitializeCapture();

	// 메시 컴포넌트에 라이팅 채널 1 전용 설정을 적용
	void ApplyPreviewLightingChannel(UMeshComponent* MeshComp);

private:
	// 액터 루트 — BaseMeshComp 회전과 SceneCaptureComp 위치를 독립적으로 관리하기 위한 분리점
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneComponent> PreviewRootComp;

	// 캐릭터 기본 메시 (PreviewRootComp에 Attach)
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USkeletalMeshComponent> BaseMeshComp;

	// 프리뷰 전용 주 조명 (라이팅 채널 1, BaseMeshComp에 Attach)
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UDirectionalLightComponent> PreviewLightComp;

	// 프리뷰 전용 보조 조명 — 역광 방지용 Fill Light (라이팅 채널 1, BaseMeshComp에 Attach)
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UDirectionalLightComponent> FillLightComp;

	// 고정 위치에서 캐릭터를 촬영하는 SceneCaptureComponent2D (PreviewRootComp에 Attach)
	// BaseMeshComp와 분리하여 메시 회전에 무관하게 방향을 유지
	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComp;

	// 캡처 결과가 저장되는 렌더 타겟
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	// 장비 동기화 시 동적 생성된 복제 메시 컴포넌트 목록
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> ReplicatedMeshes;

	// 주입된 설정값 캐시 (SetBaseMesh 등에서 재사용)
	FPBPreviewActorConfig Config;
};
