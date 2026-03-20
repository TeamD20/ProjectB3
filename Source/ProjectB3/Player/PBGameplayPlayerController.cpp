// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "PBPlayerCheatManager.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Camera/PBCameraControlComponent.h"
#include "ProjectB3/Characters/PBPlayerCharacter.h"
#include "ProjectB3/Combat/PBTargetingComponent.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "ProjectB3/Interaction/PBInteractorComponent.h"
#include "ProjectB3/Camera/PBTacticalCameraComponent.h"
#include "ProjectB3/Dialogue/PBDialogueManagerComponent.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/Player/PBPartyFollowSubsystem.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Utils/PBGameplayStatics.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Navigation/CrowdFollowingComponent.h"

#include "ProjectB3/Combat/IPBCombatTarget.h"
#include "ProjectB3/Environment/PBPathDisplayComponent.h"

APBGameplayPlayerController::APBGameplayPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	CheatClass = UPBPlayerCheatManager::StaticClass();

	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("PathFollowingComponent"));
	CameraControlComponent = CreateDefaultSubobject<UPBCameraControlComponent>(TEXT("CameraControlComponent"));
	PathDisplayComponent = CreateDefaultSubobject<UPBPathDisplayComponent>(TEXT("PathDisplayComponent"));
	TargetingComponent = CreateDefaultSubobject<UPBTargetingComponent>(TEXT("TargetingComponent"));
	InteractorComponent = CreateDefaultSubobject<UPBInteractorComponent>(TEXT("InteractorComponent"));
	TacticalCameraComponent = CreateDefaultSubobject<UPBTacticalCameraComponent>(TEXT("TacticalCameraComponent"));
	DialogueManagerComponent = CreateDefaultSubobject<UPBDialogueManagerComponent>(TEXT("DialogueManagerComponent"));
}

void APBGameplayPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (PathFollowingComponent)
	{
		PathFollowingComponent->Initialize();
	}
	
	// Enhanced Input 매핑 컨텍스트 등록
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (IsValid(DefaultMappingContext))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		if (IsValid(CameraMappingContext))
		{
			Subsystem->AddMappingContext(CameraMappingContext, 1);
		}
	}
	
	// 기본 마우스 표시
	bShowMouseCursor = true;
	
	// 초기 ControlMode 설정
	SetControllerMode(EPBPlayerControllerMode::FreeMovement);
	
	PathDisplayComponent->SetPathDisplayEnabled(true);

	BindCameraToCharacter();

	GetWorldTimerManager().SetTimer(CutoutTimerHandle, this, &APBGameplayPlayerController::UpdateCameraCutout, CutoutTraceInterval, true);
}

void APBGameplayPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 전술 카메라가 비활성 상태일 때만 기본 카메라 갱신
	if (!TacticalCameraComponent->IsTacticalCameraActive())
	{
		CameraControlComponent->UpdateCamera(DeltaTime);
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	if (bIsMouseRotationHeld || bIsFreeLookHeld)
	{
		GetInputMouseDelta(MouseX, MouseY);
	}

	if (bIsMouseRotationHeld && FMath::Abs(MouseX) > 0.0f)
	{
		CameraControlComponent->AddMouseRotationInput(MouseX);
	}

	if (bIsFreeLookHeld && (FMath::Abs(MouseX) > 0.0f || FMath::Abs(MouseY) > 0.0f))
	{
		CameraControlComponent->AddFreeLookInput(FVector2D(MouseX, MouseY));
	}
	// 단일 라인트레이스 결과를 경로 표시와 타겟팅이 공유
	FHitResult CursorHit;
	const bool bGotHit = GetCursorHitWithIgnoredActors(ECC_Visibility, false, CursorHit);

	if (bGotHit)
	{
		if (CurrentMode == EPBPlayerControllerMode::TurnMovement || CurrentMode == EPBPlayerControllerMode::FreeMovement)
		{
			UpdateHoverPathDisplay(CursorHit);
			InteractorComponent->TryFocus(CursorHit.GetActor());
		}
		if (CurrentMode == EPBPlayerControllerMode::Targeting)
		{
			TargetingComponent->UpdateTargetingFromHit(CursorHit);
		}
	}
	else if (CurrentMode == EPBPlayerControllerMode::TurnMovement || CurrentMode == EPBPlayerControllerMode::FreeMovement)
	{
		// 커서 히트 없으면 포커스 해제
		InteractorComponent->ClearFocus();
	}

	if (CurrentMode == EPBPlayerControllerMode::Moving)
	{
		if (APawn* MyPawn = GetPawn())
		{
			PathDisplayComponent->UpdateTrackedPath(MyPawn->GetActorLocation());
		}
	}

	// FreeMovement 모드: 리더 속도 감시 — 정지 감지 시 0.1초 디바운스 후 서브시스템에 통보
	if (CurrentMode == EPBPlayerControllerMode::FreeMovement)
	{
		if (APawn* MyPawn = GetPawn())
		{
			const bool bMovingNow = MyPawn->GetVelocity().SizeSquared() > 100.f;

			if (bWasLeaderMoving && !bMovingNow)
			{
				// 이동 중이었다가 속도가 떨어짐 — 0.1초 뒤에 정지 확정
				GetWorldTimerManager().SetTimer(
					LeaderStopDebounceTimer,
					[this]()
					{
						if (UPBPartyFollowSubsystem* FollowSys = GetWorld()->GetSubsystem<UPBPartyFollowSubsystem>())
						{
							FollowSys->NotifyLeaderMoveStopped();
						}
					},
					0.1f,
					false
				);
			}
			else if (!bWasLeaderMoving && bMovingNow)
			{
				// 정지 중이었다가 다시 이동 시작 — 디바운스 타이머 취소
				GetWorldTimerManager().ClearTimer(LeaderStopDebounceTimer);
			}

			bWasLeaderMoving = bMovingNow;
		}
	}
}

void APBGameplayPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInput))
	{
		return;
	}

	if (IsValid(SelectCommandAction))
	{
		EnhancedInput->BindAction(SelectCommandAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnSelectCommand);
	}
	if (IsValid(RightClickAction))
	{
		EnhancedInput->BindAction(RightClickAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnRightClick);
	}
	if (IsValid(CameraZoomAction))
	{
		EnhancedInput->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &APBGameplayPlayerController::OnCameraZoom);
	}
	if (IsValid(CameraRotateAction))
	{
		EnhancedInput->BindAction(CameraRotateAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraRotate);
	}
	if (IsValid(CameraMouseRotateAction))
	{
		EnhancedInput->BindAction(CameraMouseRotateAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraMouseRotateStart);
		EnhancedInput->BindAction(CameraMouseRotateAction, ETriggerEvent::Completed, this, &APBGameplayPlayerController::OnCameraMouseRotateStop);
		EnhancedInput->BindAction(CameraMouseRotateAction, ETriggerEvent::Canceled, this, &APBGameplayPlayerController::OnCameraMouseRotateStop);
	}
	if (IsValid(CameraFreeLookAction))
	{
		EnhancedInput->BindAction(CameraFreeLookAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraFreeLookStart);
		EnhancedInput->BindAction(CameraFreeLookAction, ETriggerEvent::Completed, this, &APBGameplayPlayerController::OnCameraFreeLookStop);
		EnhancedInput->BindAction(CameraFreeLookAction, ETriggerEvent::Canceled, this, &APBGameplayPlayerController::OnCameraFreeLookStop);
	}
	if (IsValid(CameraResetAction))
	{
		EnhancedInput->BindAction(CameraResetAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraReset);
	}
	if (IsValid(ToggleInventoryAction))
	{
		EnhancedInput->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnToggleInventory);
	}
}

void APBGameplayPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	BindCameraToCharacter();

	// 캐릭터 전환 시 카메라 오프셋을 새 캐릭터 중심으로 리셋
	CameraControlComponent->ResetOffset();

	// 빙의 폰은 포커스 대상에서 제외
	if (IsValid(InPawn))
	{
		InteractorComponent->SetIgnoreActors({ InPawn });
	}
	
	// 다른 캐릭터가 타겟팅 중이었던 경우 타겟팅 취소
	if (TargetingComponent->IsTargetingActive())
	{
		TargetingComponent->CancelTargeting();
	}
}

void APBGameplayPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	if (bIsInventoryOpen)
	{
		return;
	}
	
	CameraControlComponent->AddZoomInput(Value.Get<float>());
}

void APBGameplayPlayerController::OnCameraRotate(const FInputActionValue& Value)
{
	CameraControlComponent->AddRotationStepInput(Value.Get<float>());
}

void APBGameplayPlayerController::OnCameraMouseRotateStart(const FInputActionValue& Value)
{
	bIsMouseRotationHeld = true;
}

void APBGameplayPlayerController::OnCameraMouseRotateStop(const FInputActionValue& Value)
{
	bIsMouseRotationHeld = false;
}

void APBGameplayPlayerController::OnCameraFreeLookStart(const FInputActionValue& Value)
{
	bIsFreeLookHeld = true;
}

void APBGameplayPlayerController::OnCameraFreeLookStop(const FInputActionValue& Value)
{
	bIsFreeLookHeld = false;
}

void APBGameplayPlayerController::OnCameraReset(const FInputActionValue& Value)
{
	CameraControlComponent->ResetOffset();
}

void APBGameplayPlayerController::BindCameraToCharacter()
{
	APBPlayerCharacter* PlayerChar = Cast<APBPlayerCharacter>(GetPawn());
	if (IsValid(PlayerChar))
	{
		CameraControlComponent->SetSpringArm(PlayerChar->GetSpringArm());
	}
}

void APBGameplayPlayerController::SetControllerMode(EPBPlayerControllerMode NewMode)
{
	if (CurrentMode == NewMode)
	{
		return;
	}

	// 이전 모드 종료 처리
	if (CurrentMode == EPBPlayerControllerMode::Targeting)
	{
		if (TargetingComponent->IsTargetingActive())
		{
			TargetingComponent->ExitTargetingMode();
		}
	}
	else if (CurrentMode == EPBPlayerControllerMode::Moving)
	{
		PathDisplayComponent->EndPathTracking();
	}

	CurrentMode = NewMode;

	PathDisplayComponent->ClearPath();
	
	// FreeMovement 진입 시 PathDisplay 거리 제한 해제
	if (NewMode == EPBPlayerControllerMode::FreeMovement)
	{
		PathDisplayComponent->SetMaxMoveDistance(-1.0f);
	}
}

void APBGameplayPlayerController::SetPathDisplayMovementRange(float Range)
{
	PathDisplayComponent->SetMaxMoveDistance(Range);
}

void APBGameplayPlayerController::ClearPathDisplay()
{
	PathDisplayComponent->ClearPath();
}

void APBGameplayPlayerController::BeginMoving(const TArray<FVector>& PathPoints)
{
	PathDisplayComponent->BeginPathTracking(PathPoints);
	SetControllerMode(EPBPlayerControllerMode::Moving);
}

void APBGameplayPlayerController::EndMoving()
{
	PathDisplayComponent->EndPathTracking();
}

void APBGameplayPlayerController::EnterTargetingMode(const FPBTargetingRequest& Request)
{
	// TargetingComponent에 세션 진입 요청 후 모드 전환
	TargetingComponent->EnterTargetingMode(Request);
	SetControllerMode(EPBPlayerControllerMode::Targeting);
}

void APBGameplayPlayerController::ExitCurrentMode()
{
	// TODO: 전투 진행 중 -> None, 비전투 -> FreeMovement
	SetControllerMode(EPBPlayerControllerMode::None);
}

void APBGameplayPlayerController::ToggleInventory()
{
	UPBUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UPBUIManagerSubsystem>(GetLocalPlayer());
	if (!IsValid(UIManager) || !IsValid(InventoryWidgetClass))
	{
		return;
	}

	// TODO: Instance 캐싱?
	if (UIManager->IsUIActive(InventoryWidgetClass))
	{
		bIsInventoryOpen = false;
		UIManager->PopUI(nullptr);
	}
	else
	{
		bIsInventoryOpen = true;
		UIManager->PushUI(InventoryWidgetClass);
	}
}

void APBGameplayPlayerController::OnToggleInventory(const FInputActionValue& Value)
{
	ToggleInventory();
}

void APBGameplayPlayerController::OnSelectCommand(const FInputActionValue& Value)
{
	switch (CurrentMode)
	{
	case EPBPlayerControllerMode::Targeting:
		// 타겟팅 모드: MultiTarget는 후보 추가, 그 외는 즉시 확정
		if (TargetingComponent->IsMultiTargetMode())
		{
			TargetingComponent->AddTargetSelection();
		}
		else
		{
			TargetingComponent->ConfirmTarget();
		}
		return;

	case EPBPlayerControllerMode::TurnMovement:
	{
		// 포커스 대상이 있으면 상호작용 위임 후 이동하지 않음
		if (InteractorComponent->HasFocus())
		{
			InteractorComponent->Interact();
			return;
		}

		FHitResult HitResult;
		if (!GetCursorHitWithIgnoredActors(ECC_Visibility, false, HitResult))
		{
			return;
		}

		APawn* MyPawn = GetPawn();
		if (!IsValid(MyPawn))
		{
			return;
		}

		// 이동 Payload 구성 후 이동 어빌리티에 이벤트 전송
		UPBTargetPayload* MovePayload = NewObject<UPBTargetPayload>(this);
		MovePayload->TargetData.TargetingMode = EPBTargetingMode::Location;
		MovePayload->TargetData.TargetLocations = { HitResult.Location };

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyPawn);
		if (IsValid(ASC))
		{
			FGameplayEventData EventData;
			EventData.OptionalObject = MovePayload;
				ASC->HandleGameplayEvent(PBGameplayTags::Event_Movement_MoveCommand, &EventData);
		}

		PathDisplayComponent->ClearPath();
		if (IsValid(CursorVFX))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CursorVFX, HitResult.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
		}
		return;
	}
	case EPBPlayerControllerMode::FreeMovement:
	{
		// 포커스 대상이 있으면 상호작용 위임 후 이동하지 않음
		if (InteractorComponent->HasFocus())
		{
			InteractorComponent->Interact();
			return;
		}

		FHitResult HitResult;
		if (!GetCursorHitWithIgnoredActors(ECC_Visibility, false, HitResult))
		{
			return;
		}

		APawn* MyPawn = GetPawn();
		if (!IsValid(MyPawn))
		{
			return;
		}

		// 거리 제한 없이 PC가 직접 이동 명령
		if (UPBEnvironmentSubsystem* EnvironmentSubsystem = GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>())
		{
			EnvironmentSubsystem->RequestMoveToLocation(this, HitResult.Location, 50.f, false);
		}

		// 파티 추적 서브시스템에 리더 이동 시작 통보
		if (!bWasLeaderMoving)
		{
			if (UPBPartyFollowSubsystem* FollowSys = GetWorld()->GetSubsystem<UPBPartyFollowSubsystem>())
			{
				if (APBCharacterBase* LeaderChar = Cast<APBCharacterBase>(MyPawn))
				{
					FollowSys->NotifyLeaderMoveStarted(LeaderChar);
				}
			}
			bWasLeaderMoving = true;		
		}
		
		PathDisplayComponent->ClearPath();
		if (IsValid(CursorVFX))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CursorVFX, HitResult.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
		}
		return;
	}
	case EPBPlayerControllerMode::None:
	default:
		return;
	}
}

void APBGameplayPlayerController::OnRightClick(const FInputActionValue& Value)
{
	// Targeting 모드 처리. MultiTarget: 마지막 후보 제거.
	if (CurrentMode == EPBPlayerControllerMode::Targeting)
	{
		if (TargetingComponent->IsMultiTargetMode())
		{
			TargetingComponent->RemoveLastTarget();
		}
		else
		{
			TargetingComponent->CancelTargeting();
		}
	}
}

void APBGameplayPlayerController::UpdateHoverPathDisplay(const FHitResult& HitResult)
{
	if (!PathDisplayComponent->IsPathDisplayEnabled())
	{
		return;
	}

	const FVector HoverLocation = HitResult.ImpactPoint;
	if (FVector::Dist(HoverLocation, LastHoverLocation) < PathUpdateMinDistance)
	{
		return;
	}

	LastHoverLocation = HoverLocation;
	RequestNavPathDisplay(HoverLocation);
}

void APBGameplayPlayerController::RequestNavPathDisplay(const FVector& TargetLocation)
{
	APawn* MyPawn = GetPawn();
	if (!IsValid(MyPawn))
	{
		return;
	}

	UPBEnvironmentSubsystem* EnvironmentSubsystem = GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>();
	if (!IsValid(EnvironmentSubsystem))
	{
		return;
	}

	const FPBPathFindResult PathCostResult = EnvironmentSubsystem->CalculatePathForAgent(this, TargetLocation, false);
	if (!PathCostResult.bIsValid || PathCostResult.PathPoints.IsEmpty())
	{
		PathDisplayComponent->ClearPath();
		return;
	}

	TArray<FVector> DisplayPathPoints = PathCostResult.PathPoints;
	PathDisplayComponent->DisplayPath(DisplayPathPoints, true);
}

void APBGameplayPlayerController::UpdateCameraCutout()
{
	APBGameplayPlayerState* PBPlayerState = GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PBPlayerState))
	{
		return;
	}

	TArray<AActor*> PartyMembers = PBPlayerState->GetPartyMembers();
	if (PartyMembers.IsEmpty())
	{
		return;
	}

	APlayerCameraManager* CameraManager = PlayerCameraManager;
	if (!IsValid(CameraManager))
	{
		return;
	}

	FVector CameraLocation = CameraManager->GetCameraLocation();
	TMap<TWeakObjectPtr<UMeshComponent>, FPBMeshFadeState> CurrentHits;
	TSet<TWeakObjectPtr<AActor>> CurrentActorHits;

	FCollisionQueryParams QueryParams(TEXT("CameraCutoutTrace"), false, GetPawn());
	QueryParams.AddIgnoredActors(PartyMembers);

	// 최대 4인까지만 안전하게 순회
	int32 MemberCount = FMath::Min(PartyMembers.Num(), 4);

	for (int32 Index = 0; Index < MemberCount; ++Index)
	{
		AActor* Member = PartyMembers[Index];
		if (!IsValid(Member))
		{
			continue;
		}

		TArray<FHitResult> HitResults;
		bool bHit = GetWorld()->LineTraceMultiByChannel(
			HitResults,
			CameraLocation,
			Member->GetActorLocation(),
			ECC_Visibility, // 채널 변경 가능
			QueryParams
		);

		if (bHit)
		{
			for (const FHitResult& Hit : HitResults)
			{
				AActor* HitActor = Hit.GetActor();
				if (!IsValid(HitActor))
				{
					continue;
				}

				CurrentActorHits.Add(HitActor);

				TArray<UMeshComponent*> HitMeshes;
				UPBGameplayStatics::GetAllMeshComponents(HitActor, HitMeshes);

				for (UMeshComponent* Mesh : HitMeshes)
				{
					if (!IsValid(Mesh))
					{
						continue;
					}

					FPBMeshFadeState& State = CurrentHits.FindOrAdd(Mesh);
					State.bHitByMember[Index] = true;

					int32 NumMaterials = Mesh->GetNumMaterials();
					for (int32 i = 0; i < NumMaterials; ++i)
					{
						UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
						if (!IsValid(MID))
						{
							MID = Mesh->CreateAndSetMaterialInstanceDynamic(i);
						}

						if (IsValid(MID))
						{
							FName LocParamName = FName(*FString::Printf(TEXT("HitLocation%d"), Index));
							FName FadeParamName = FName(*FString::Printf(TEXT("TraceFadeAmount%d"), Index));
							MID->SetVectorParameterValue(LocParamName, Hit.ImpactPoint);
							MID->SetScalarParameterValue(FadeParamName, 1.0f);
						}
					}
				}
			}
		}
	}

	// 기존에 투명화되었으나 이번 주기에 히트 상태가 풀린(파티원별) 메시 복구
	for (auto& Pair : FadedMeshes)
	{
		TWeakObjectPtr<UMeshComponent> FadedMesh = Pair.Key;
		if (!FadedMesh.IsValid())
		{
			continue;
		}

		UMeshComponent* Mesh = FadedMesh.Get();
		const FPBMeshFadeState& OldState = Pair.Value;
		const FPBMeshFadeState* NewStatePtr = CurrentHits.Find(Mesh);

		bool bNewHit[4] = {false, false, false, false};
		if (NewStatePtr)
		{
			for (int32 idx = 0; idx < 4; ++idx)
			{
				bNewHit[idx] = NewStatePtr->bHitByMember[idx];
			}
		}

		int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
			if (IsValid(MID))
			{
				for (int32 idx = 0; idx < 4; ++idx)
				{
					// 이전에는 가려졌는데(true), 현재 안 가려짐(false) -> 파라미터 0 반환
					if (OldState.bHitByMember[idx] && !bNewHit[idx])
					{
						FName FadeParamName = FName(*FString::Printf(TEXT("TraceFadeAmount%d"), idx));
						MID->SetScalarParameterValue(FadeParamName, 0.0f);
					}
				}
			}
		}
	}

	FadedMeshes = CurrentHits;
	FadedActors = CurrentActorHits;
}

bool APBGameplayPlayerController::GetCursorHitWithIgnoredActors(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& OutHitResult) const
{
	FVector WorldLocation, WorldDirection;
	if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		FVector Start = WorldLocation;
		FVector End = Start + (WorldDirection * 100000.f);

		FCollisionQueryParams QueryParams(TEXT("ClickTraceWithIgnore"), bTraceComplex, GetPawn());
		
		// 투명화된 장애물 액터들을 무시 목록에 추가 (CombatTarget이 아닌 경우만)
		for (const TWeakObjectPtr<AActor>& FadedActor : FadedActors)
		{
			// 전투 타겟 인터페이스를 가진 액터는 트레이스 무시 목록에서 제외
			if (!FadedActor.IsValid() || FadedActor->Implements<UPBCombatTarget>())
			{
				continue;
			}
			
			QueryParams.AddIgnoredActor(FadedActor.Get());
		}

		return GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, TraceChannel, QueryParams);
	}
	return false;
}

