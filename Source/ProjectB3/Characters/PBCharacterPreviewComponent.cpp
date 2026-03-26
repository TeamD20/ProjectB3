// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterPreviewComponent.h"

#include "PBCharacterBase.h"
#include "PBCharacterPreviewActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UPBCharacterPreviewComponent::UPBCharacterPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PreviewActorClass = APBCharacterPreviewActor::StaticClass();
	
	static ConstructorHelpers::FClassFinder<APBCharacterPreviewActor> ClassFinder(TEXT("/Game/0_BP/Characters/BP_PreviewActor.BP_PreviewActor_C"));
	if (ClassFinder.Succeeded())
	{
		PreviewActorClass = ClassFinder.Class;
	}
}

void UPBCharacterPreviewComponent::BeginPlay()
{
	Super::BeginPlay();
	SpawnPreviewActor();
	BindEquipmentEvents();
}

void UPBCharacterPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindEquipmentEvents();

	if (IsValid(PreviewActor))
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UPBCharacterPreviewComponent::SetCaptureActive(bool bActive)
{
	if (IsValid(PreviewActor))
	{
		PreviewActor->SetCaptureActive(bActive);
	}
}

UTextureRenderTarget2D* UPBCharacterPreviewComponent::GetRenderTarget() const
{
	return IsValid(PreviewActor) ? PreviewActor->GetRenderTarget() : nullptr;
}

void UPBCharacterPreviewComponent::SpawnPreviewActor()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// 플레이 공간 하단 — 플레이어 카메라 Frustum에 진입 불가
	const FVector SpawnLocation = FVector(0.0f, 0.0f, -50000.0f);
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 기본 APBCharacterPreviewActor를 직접 스폰 — 별도 BP 파생 클래스 불필요
	PreviewActor = GetWorld()->SpawnActor<APBCharacterPreviewActor>(
		PreviewActorClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!IsValid(PreviewActor))
	{
		return;
	}

	// Component에 설정된 값을 구조체로 묶어 Actor에 주입
	FPBPreviewActorConfig ActorConfig;
	ActorConfig.RenderTargetWidth    = RenderTargetWidth;
	ActorConfig.RenderTargetHeight   = RenderTargetHeight;
	ActorConfig.PreviewIdleSequence   = PreviewIdleSequence;
	PreviewActor->ApplyConfig(ActorConfig);

	// 기본 메시 설정
	if (APBCharacterBase* OwnerChar = Cast<APBCharacterBase>(Owner))
	{
		OwnerChar->SetupVisualMesh();
		if (USkeletalMeshComponent* OwnerMesh = OwnerChar->GetVisualMesh())
		{
			PreviewActor->SetBaseMesh(OwnerMesh->GetSkeletalMeshAsset());
		}
	}

	// 현재 장착 중인 장비 메시 초기 동기화
	PreviewActor->SyncMeshesFrom(Owner);
}

void UPBCharacterPreviewComponent::BindEquipmentEvents()
{
	CachedCharacterBase = Cast<APBCharacterBase>(GetOwner());
	if (CachedCharacterBase.IsValid())
	{
		CachedCharacterBase->OnCharacterEquipmentChanged.AddDynamic(this, &ThisClass::HandleCharacterEquipmentChanged);
	}
}

void UPBCharacterPreviewComponent::UnbindEquipmentEvents()
{
	if (CachedCharacterBase.IsValid())
	{
		CachedCharacterBase->OnCharacterEquipmentChanged.RemoveDynamic(this, &ThisClass::HandleCharacterEquipmentChanged);
	}
	CachedCharacterBase = nullptr;
}

void UPBCharacterPreviewComponent::HandleCharacterEquipmentChanged(const FGameplayTag& SlotTag)
{
	// 슬롯 단위가 아닌 전체 재동기화 — 장비 시스템 내부 참조 없이 결과물 기준으로 복제
	if (IsValid(PreviewActor) && IsValid(GetOwner()))
	{
		PreviewActor->SyncMeshesFrom(GetOwner());
	}
}
