// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterPreviewActor.h"

#include "PBCharacterBase.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"

APBCharacterPreviewActor::APBCharacterPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 이 Actor는 프리뷰 전용 — 모든 콜리전 비활성화
	SetActorEnableCollision(false);

	// PreviewRootComp를 액터 루트로 설정
	// BaseMeshComp와 SceneCaptureComp를 분리 Attach하여
	// 메시 회전(-90도 등)이 SceneCapture 방향에 영향을 주지 않도록 함
	PreviewRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRootComp"));
	SetRootComponent(PreviewRootComp);

	// BaseMeshComp — PreviewRootComp에 Attach
	BaseMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMeshComp"));
	BaseMeshComp->SetupAttachment(PreviewRootComp);
	BaseMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseMeshComp->SetRelativeRotation(FRotator(0.f,-90.f,0.f));

	// 전용 주 조명 (채널 1) — BaseMeshComp에 Attach하여 메시와 함께 조명 방향 유지
	PreviewLightComp = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PreviewLightComp"));
	PreviewLightComp->SetupAttachment(PreviewRootComp);
	PreviewLightComp->LightingChannels.bChannel0 = false;
	PreviewLightComp->LightingChannels.bChannel1 = true;

	// 보조 Fill Light (채널 1)
	FillLightComp = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("FillLightComp"));
	FillLightComp->SetupAttachment(PreviewRootComp);
	FillLightComp->LightingChannels.bChannel0 = false;
	FillLightComp->LightingChannels.bChannel1 = true;
	FillLightComp->SetIntensity(1.0f);

	// SceneCaptureComp — PreviewRootComp에 직접 Attach (BaseMeshComp 회전에 독립)
	SceneCaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComp"));
	SceneCaptureComp->SetupAttachment(PreviewRootComp);
	SceneCaptureComp->SetRelativeLocation(FVector(-1000.0f, 0.0f, 0.0f));
}

void APBCharacterPreviewActor::ApplyConfig(const FPBPreviewActorConfig& InConfig)
{
	Config = InConfig;
	InitializeCapture();
	ApplyPreviewLightingChannel(BaseMeshComp);
}

void APBCharacterPreviewActor::SetBaseMesh(USkeletalMesh* Mesh)
{
	if (!IsValid(Mesh))
	{
		return;
	}

	BaseMeshComp->SetSkeletalMeshAsset(Mesh);

	// ABP 없이 단일 AnimSequence 재생 — 없으면 T-Pose 유지
	BaseMeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	if (IsValid(Config.PreviewIdleSequence))
	{
		BaseMeshComp->SetAnimation(Config.PreviewIdleSequence);
		BaseMeshComp->Play(true);
	}
}

void APBCharacterPreviewActor::SyncMeshesFrom(AActor* SourceActor)
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	// 기존 복제 컴포넌트 전부 제거
	for (UMeshComponent* OldMesh : ReplicatedMeshes)
	{
		if (IsValid(OldMesh))
		{
			OldMesh->DestroyComponent();
		}
	}
	ReplicatedMeshes.Reset();

	// SourceActor 자신 + Attach된 Child Actor(APBEquipmentActor 등)의 메시까지 포함
	TArray<UMeshComponent*> SourceMeshes;
	SourceActor->GetComponents<UMeshComponent>(SourceMeshes, /*bIncludeChildActors=*/true);

	// Character의 주 SkeletalMesh는 SetBaseMesh()로 별도 처리하므로 제외
	USkeletalMeshComponent* OwnerMainMesh = nullptr;
	USkeletalMeshComponent* OwnerVisualMesh = nullptr;
	if (APBCharacterBase* OwnerChar = Cast<APBCharacterBase>(SourceActor))
	{
		OwnerMainMesh = OwnerChar->GetMesh();
		OwnerVisualMesh = OwnerChar->GetVisualMesh();
	}

	for (UMeshComponent* SrcMesh : SourceMeshes)
	{
		// 주 캐릭터 메시는 SetBaseMesh()로 별도 처리
		// bHiddenInGame인 컴포넌트는 게임에서 보이지 않아야 하는 것 (카메라 시각화, 디버그 메시 등)이므로 제외
		if (!IsValid(SrcMesh) || !SrcMesh->IsVisible() || SrcMesh == OwnerMainMesh || SrcMesh == OwnerVisualMesh)
		{
			continue;
		}

		USkeletalMeshComponent* SrcSkeletal = Cast<USkeletalMeshComponent>(SrcMesh);
		UStaticMeshComponent*   SrcStatic   = Cast<UStaticMeshComponent>(SrcMesh);

		UMeshComponent* NewMesh = nullptr;

		if (IsValid(SrcSkeletal) && IsValid(SrcSkeletal->GetSkeletalMeshAsset()))
		{
			USkeletalMeshComponent* NewSkeletal = NewObject<USkeletalMeshComponent>(this);
			NewSkeletal->SetSkeletalMeshAsset(SrcSkeletal->GetSkeletalMeshAsset());
			NewMesh = NewSkeletal;
		}
		else if (IsValid(SrcStatic) && IsValid(SrcStatic->GetStaticMesh()))
		{
			UStaticMeshComponent* NewStatic = NewObject<UStaticMeshComponent>(this);
			NewStatic->SetStaticMesh(SrcStatic->GetStaticMesh());
			NewMesh = NewStatic;
		}

		if (!IsValid(NewMesh))
		{
			continue;
		}

		// 머티리얼 복사
		for (int32 MatIdx = 0; MatIdx < SrcMesh->GetNumMaterials(); ++MatIdx)
		{
			NewMesh->SetMaterial(MatIdx, SrcMesh->GetMaterial(MatIdx));
		}

		NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewMesh->RegisterComponent();

		// 소켓 기반 Attach 재현
		const FName SocketName = SrcMesh->GetAttachSocketName();
		if (SocketName != NAME_None && BaseMeshComp->DoesSocketExist(SocketName))
		{
			NewMesh->AttachToComponent(BaseMeshComp, FAttachmentTransformRules::KeepRelativeTransform, SocketName);
		}
		else
		{
			NewMesh->AttachToComponent(BaseMeshComp, FAttachmentTransformRules::KeepRelativeTransform);
		}

		NewMesh->SetRelativeTransform(SrcMesh->GetRelativeTransform());
		ApplyPreviewLightingChannel(NewMesh);

		ReplicatedMeshes.Add(NewMesh);
	}
}

void APBCharacterPreviewActor::SetCaptureActive(bool bActive)
{
	if (!IsValid(SceneCaptureComp))
	{
		return;
	}

	SceneCaptureComp->SetVisibility(bActive);
	SceneCaptureComp->bCaptureEveryFrame = bActive;

	// 활성화 시 즉시 1회 캡처 — 첫 프레임 검은 화면 방지
	if (bActive)
	{
		SceneCaptureComp->CaptureScene();
	}
}

void APBCharacterPreviewActor::InitializeCapture()
{
	// 렌더 타겟 동적 생성
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitCustomFormat(Config.RenderTargetWidth, Config.RenderTargetHeight, PF_B8G8R8A8, false);
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->UpdateResourceImmediate(true);

	// 조명 파라미터 적용
	if (IsValid(PreviewLightComp))
	{
		PreviewLightComp->SetIntensity(Config.PreviewLightIntensity);
		PreviewLightComp->SetRelativeRotation(Config.PreviewLightRotation);
	}
	if (IsValid(FillLightComp))
	{
		// Fill Light는 주 조명 반대 방향에서 약하게
		FillLightComp->SetRelativeRotation(FRotator(
			Config.PreviewLightRotation.Pitch * -0.5f,
			Config.PreviewLightRotation.Yaw + 180.0f,
			0.0f
		));
	}

	// SceneCapture 설정
	SceneCaptureComp->TextureTarget         = RenderTarget;
	SceneCaptureComp->CaptureSource         = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureComp->bCaptureEveryFrame    = false;
	SceneCaptureComp->bCaptureOnMovement    = false;
	SceneCaptureComp->PrimitiveRenderMode   = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCaptureComp->ShowOnlyActors.Add(this);
	// SceneCaptureComp 자체가 ShowOnly 목록에 포함되면 내부 카메라 메시가 캡처됨 — 명시적 제외
	// SceneCaptureComp->HiddenComponents.Add(SceneCaptureComp); <- 컴파일 에러

	// 월드 환경 요소 차단, 전용 채널 1 조명만 사용
	SceneCaptureComp->ShowFlags.SetAtmosphere(false);
	SceneCaptureComp->ShowFlags.SetFog(false);
	SceneCaptureComp->ShowFlags.SetVolumetricFog(false);
	SceneCaptureComp->ShowFlags.SetDynamicShadows(false);
	SceneCaptureComp->ShowFlags.SetSkyLighting(false);
	SceneCaptureComp->ShowFlags.SetAmbientOcclusion(false);
	// UCameraComponent 내부의 UDrawFrustumComponent(카메라 절두체 시각화)가
	// ShowOnlyActors 목록에 포함된 Actor에 붙어있으면 캡처됨 — 명시적으로 차단
	SceneCaptureComp->ShowFlags.SetCameraFrustums(false);
	SceneCaptureComp->ShowFlags.SetBillboardSprites(false);

	SceneCaptureComp->SetRelativeLocation(Config.CaptureOffset);
	SceneCaptureComp->SetRelativeRotation(Config.CaptureRotation);
	SceneCaptureComp->SetVisibility(false);
}

void APBCharacterPreviewActor::ApplyPreviewLightingChannel(UMeshComponent* MeshComp)
{
	if (!IsValid(MeshComp))
	{
		return;
	}

	// 채널 0(월드 조명) 차단, 채널 1(전용 조명)만 수신
	MeshComp->LightingChannels.bChannel0 = false;
	MeshComp->LightingChannels.bChannel1 = true;
}
