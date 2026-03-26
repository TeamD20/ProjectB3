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
#include "ProjectB3/Game/PBGameplayHUD.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Navigation/CrowdFollowingComponent.h"

#include "ProjectB3/Combat/IPBCombatTarget.h"
#include "ProjectB3/Environment/PBPathDisplayComponent.h"
#include "ProjectB3/UI/Cursor/PBDefaultCursorWidget.h"
#include "ProjectB3/UI/Cursor/PBTargetingCursorWidget.h"

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

void APBGameplayPlayerController::FadeInFromBlack()
{
	if (APBGameplayHUD* GameplayHUD = GetHUD<APBGameplayHUD>())
	{
		GameplayHUD->ScheduleInitializeHUDWidgets(FadeInInitialDelay);
	}

	if (APlayerCameraManager* CamManager = PlayerCameraManager)
	{
		CamManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);

		if (FadeInInitialDelay <= 0.0f)
		{
			CamManager->StartCameraFade(1.0f, 0.0f, FadeInDuration, FLinearColor::Black, false, true);
			return;
		}

		TWeakObjectPtr<APlayerCameraManager> WeakCamManager = CamManager;
		FTimerDelegate FadeDelegate = FTimerDelegate::CreateWeakLambda(this,
			[this, WeakCamManager]()
			{
				if (WeakCamManager.IsValid())
				{
					WeakCamManager->StartCameraFade(1.0f, 0.0f, FadeInDuration, FLinearColor::Black, false, true);
				}
			});

		FTimerHandle FadeInDelayHandle;
		GetWorldTimerManager().SetTimer(FadeInDelayHandle, FadeDelegate, FadeInInitialDelay, false);
	}
}

void APBGameplayPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FadeInFromBlack();

	CurrentMouseCursor = EMouseCursor::Default;
	
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
			InteractorComponent->TryFocus(CursorHit.GetActor());
		}
		if (CurrentMode == EPBPlayerControllerMode::TurnMovement)
		{
			UpdateHoverPathDisplay(CursorHit);

			// 커서 위젯에 이동 거리 표시
			if (IsValid(DefaultCursorWidget))
			{
				const float Meters = PathDisplayComponent->GetLastTotalDistance() / 100.f;
				DefaultCursorWidget->SetDistance(Meters);
			}
		}
		if (CurrentMode == EPBPlayerControllerMode::Targeting)
		{
			TargetingComponent->UpdateTargetingFromHit(CursorHit);

			// 타겟팅 중 폰을 커서 방향으로 회전
			if (APawn* MyPawn = GetPawn())
			{
				const FVector PawnLocation = MyPawn->GetActorLocation();
				const FVector ToTarget = CursorHit.ImpactPoint - PawnLocation;
				const FRotator TargetRotation = FRotator(0.f, ToTarget.Rotation().Yaw, 0.f);
				MyPawn->SetActorRotation(FMath::RInterpTo(
					MyPawn->GetActorRotation(), TargetRotation, DeltaTime, 10.f));
			}
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
			const bool bIsInDialogue = DialogueManagerComponent->IsPlaying();
			
			if (bWasLeaderMoving && !bMovingNow && !bIsInDialogue)
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
	
	if (NewMode == EPBPlayerControllerMode::Targeting)
	{
		if (!IsValid(TargetingCursorWidget) && IsValid(TargetingCursorWidgetClass))
		{
			TargetingCursorWidget = CreateWidget<UPBTargetingCursorWidget>(this, TargetingCursorWidgetClass);
		}
		if (IsValid(TargetingCursorWidget))
		{
			SetMouseCursorWidget(EMouseCursor::Default, TargetingCursorWidget);
		}
	}
	else
	{
		if (!IsValid(DefaultCursorWidget) && IsValid(DefaultCursorWidgetClass))
		{
			DefaultCursorWidget = CreateWidget<UPBDefaultCursorWidget>(this, DefaultCursorWidgetClass);
		}
		if (IsValid(DefaultCursorWidget))
		{
			DefaultCursorWidget->ClearDistance();
			SetMouseCursorWidget(EMouseCursor::Default, DefaultCursorWidget);
		}
	}

	// 이전 모드 종료 처리
	if (CurrentMode == EPBPlayerControllerMode::Targeting)
	{
		// MultiTarget 게이지 바인딩 해제
		TargetingComponent->OnSelectionChanged.RemoveAll(this);

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

	// MultiTarget 모드: 게이지 바인딩
	if (Request.Mode == EPBTargetingMode::MultiTarget && IsValid(TargetingCursorWidget))
	{
		TargetingCursorWidget->SetGaugeCount(0, Request.MaxTargetCount);
		TargetingComponent->OnSelectionChanged.AddUObject(this, &APBGameplayPlayerController::OnTargetSelectionChanged);
	}
}

void APBGameplayPlayerController::ExitCurrentMode()
{
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

void APBGameplayPlayerController::OnGameOver()
{
	SetControllerMode(EPBPlayerControllerMode::None);
	
	if (!IsValid(GameOverWidgetClass))
	{
		return;
	}
	
	UPBUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UPBUIManagerSubsystem>(GetLocalPlayer());
	if (!IsValid(UIManager) || !IsValid(InventoryWidgetClass))
	{
		return;
	}
	
	UIManager->PushUI(GameOverWidgetClass);
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
			// 이벤트 전송 — HandleMoveEvent가 동기적으로 실행되어 캐시를 소모
			ASC->HandleGameplayEvent(PBGameplayTags::Event_Movement_MoveCommand, &EventData);
		}

		// 이동 이벤트 처리 완료 후 캐시 소모 종료
		if (UPBEnvironmentSubsystem* EnvSys = GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>())
		{
			EnvSys->EndEnvironmentCache();
		}

		// 기존 미리보기 경로 지움
		PathDisplayComponent->ClearPath();
			
		// 클릭 위치에 VFX 소환
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
			
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyPawn))
		{
			// 행동 불능 상태: 이동 불가
			if (ASC->HasMatchingGameplayTag(PBGameplayTags::Character_State_Incapacitated))
			{
				return;
			}
		}

		// 거리 제한 없이 PC가 직접 이동 명령
		if (UPBEnvironmentSubsystem* EnvironmentSubsystem = GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>())
		{
			EnvironmentSubsystem->RequestMoveToLocation(this, HitResult.Location, 50.f, false);
			EnvironmentSubsystem->EndEnvironmentCache();
		}

		// 파티 추적 서브시스템에 리더 이동 시작 통보
		if (!bWasLeaderMoving)
		{
			if (APBCharacterBase* LeaderChar = Cast<APBCharacterBase>(MyPawn))
			{
				if (UPBPartyFollowSubsystem* FollowSys = GetWorld()->GetSubsystem<UPBPartyFollowSubsystem>())
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
		if (TargetingComponent->IsMultiTargetMode() && TargetingComponent->NumSelectedTargets() > 0)
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
	
	// hover마다 캐시를 flush하고 새로 시작 — 클릭 시 EndEnvironmentCache로 소모
	EnvironmentSubsystem->BeginEnvironmentCache();

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

		if (!bHit)
		{
			continue;
		}
		
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
					if (IsValid(MID))
					{
						FName LocParamName = FName(*FString::Printf(TEXT("HitLocation%d"), Index));
						FName FadeParamName = FName(*FString::Printf(TEXT("TraceFadeAmount%d"), Index));
						MID->SetVectorParameterValue(LocParamName, Hit.ImpactPoint);
						MID->SetScalarParameterValue(FadeParamName, 1.0f);
					}
					else
					{
						MID = Mesh->CreateAndSetMaterialInstanceDynamic(i);
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
			if (!IsValid(MID))
			{
				continue;
			}
			
			for (int32 idx = 0; idx < 4; ++idx)
			{
				// 이전에는 가려졌는데(true), 현재 안 가려짐(false) -> 파라미터 0 반환
				if (!OldState.bHitByMember[idx] || bNewHit[idx])
				{
					continue;
				}
				
				FName FadeParamName = FName(*FString::Printf(TEXT("TraceFadeAmount%d"), idx));
				MID->SetScalarParameterValue(FadeParamName, 0.0f);
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

void APBGameplayPlayerController::OnTargetSelectionChanged(const FPBAbilityTargetData& TargetData)
{
	if (!IsValid(TargetingCursorWidget))
	{
		return;
	}

	const int32 Current = TargetingComponent->NumSelectedTargets();
	const int32 Max = TargetingComponent->GetMaxTargetCount();
	TargetingCursorWidget->SetGaugeCount(Current, Max);
}

