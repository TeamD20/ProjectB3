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
	FillLightComp->SetIntensity(0.6f);

	// SceneCaptureComp — PreviewRootComp에 직접 Attach (BaseMeshComp 회전에 독립)
	SceneCaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComp"));
	SceneCaptureComp->SetupAttachment(PreviewRootComp);
	SceneCaptureComp->SetRelativeLocation(FVector(-1000.0f, 0.0f, 0.0f));
	
	// SceneCapture 기본 설정 (TextureTarget은 InitializeCapture에서 RenderTarget 생성 후 할당)
	SceneCaptureComp->CaptureSource         = ESceneCaptureSource::SCS_SceneColorHDR;
	SceneCaptureComp->bCaptureEveryFrame    = false;
	SceneCaptureComp->bCaptureOnMovement    = false;
	SceneCaptureComp->PrimitiveRenderMode   = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	// 월드 환경 요소 차단 — 전용 채널 1 DirectionalLight만 사용
	SceneCaptureComp->ShowFlags.SetAtmosphere(false);
	SceneCaptureComp->ShowFlags.SetFog(false);
	SceneCaptureComp->ShowFlags.SetVolumetricFog(false);
	SceneCaptureComp->ShowFlags.SetSkyLighting(false);

	// 그림자 — 전용 조명이므로 불필요
	SceneCaptureComp->ShowFlags.SetDynamicShadows(false);
	SceneCaptureComp->ShowFlags.SetContactShadows(false);
	SceneCaptureComp->ShowFlags.SetAmbientOcclusion(false);
	SceneCaptureComp->ShowFlags.SetDistanceFieldAO(false);

	// 격리된 씬이므로 환경 반사·간접광 불필요
	SceneCaptureComp->ShowFlags.SetGlobalIllumination(false);
	SceneCaptureComp->ShowFlags.SetReflectionEnvironment(false);
	SceneCaptureComp->ShowFlags.SetScreenSpaceReflections(false);
	SceneCaptureComp->ShowFlags.SetIndirectLightingCache(false);

	// 포스트 프로세스 — 저해상도 프리뷰에 불필요한 효과 제거
	SceneCaptureComp->ShowFlags.SetEyeAdaptation(false);
	SceneCaptureComp->ShowFlags.SetBloom(false);
	SceneCaptureComp->ShowFlags.SetMotionBlur(false);
	SceneCaptureComp->ShowFlags.SetLensFlares(false);
	SceneCaptureComp->ShowFlags.SetToneCurve(false);

	// 에디터 시각화 요소 차단
	SceneCaptureComp->ShowFlags.SetCameraFrustums(false);
	SceneCaptureComp->ShowFlags.SetBillboardSprites(false);
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
	RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	RenderTarget->UpdateResourceImmediate(true);

	// RenderTarget 할당 및 ShowOnlyList 설정 (생성자에선 RenderTarget 미생성)
	SceneCaptureComp->TextureTarget = RenderTarget;
	SceneCaptureComp->ShowOnlyActors.Reset();
	SceneCaptureComp->ShowOnlyActors.Add(this);
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
