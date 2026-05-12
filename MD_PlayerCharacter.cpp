// Fill out your copyright notice in the Description page of Project Settings.

#include "MD_PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Camera/CameraComponent.h"

#include "MD_InteractInterface.h"
#include "DrawDebugHelpers.h"

// Sets default values
AMD_PlayerCharacter::AMD_PlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);
	
	SpringArmComp->TargetArmLength = 210.f;
	SpringArmComp->SocketOffset = FVector(0.f, 50.f, 55.f);
	SpringArmComp->bDoCollisionTest = true;

	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->CameraLagSpeed = 9.f;
	SpringArmComp->CameraLagMaxDistance = 35.f;

	SpringArmComp->bEnableCameraRotationLag = true;
	SpringArmComp->CameraRotationLagSpeed = 12.f;

	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->FieldOfView = 66.f;
}

// Called when the game starts or when spawned
void AMD_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CachedPlayerController = Cast<APlayerController>(GetController());

	ULocalPlayer* LocalPlayer = CachedPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

// Called every frame
void AMD_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateInteractionFocus();
}

// Called to bind functionality to input
void AMD_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Look);
	EnhancedInput->BindAction(MenuAction, ETriggerEvent::Started, this, &AMD_PlayerCharacter::ToggleEscapeMenu);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AMD_PlayerCharacter::HandleInteract);
}

void AMD_PlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	
	FRotator ControlRot = Controller->GetControlRotation();
	FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
}
void AMD_PlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AMD_PlayerCharacter::ToggleEscapeMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("Open Menu"));

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PauseMenuWidgetClass && !PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UUserWidget>(PC, PauseMenuWidgetClass);
	}

	if (PauseMenuWidget)
	{
		PauseMenuWidget->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void AMD_PlayerCharacter::HandleInteract()
{
	if (!CurrentFocusedInteractable)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Interacted"));
	IMD_InteractInterface::Execute_Interact(CurrentFocusedInteractable, this);
}

void AMD_PlayerCharacter::ClearInteractionFocus()
{
	if (CurrentFocusedInteractable &&
		CurrentFocusedInteractable->Implements<UMD_InteractInterface>())
	{
		IMD_InteractInterface::Execute_SetInteractPromptVisible(CurrentFocusedInteractable, false);
		IMD_InteractInterface::Execute_Highlight(CurrentFocusedInteractable, false);
	}

	CurrentFocusedInteractable = nullptr;
}

void AMD_PlayerCharacter::UpdateInteractionFocus()
{
	FVector Start;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(Start, ViewRotation);

	const FVector End = Start + (ViewRotation.Vector() * InteractDistance);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	if (!bHit || !Hit.GetActor())
	{
		ClearInteractionFocus();
		return;
	}

	AActor* HitActor = Hit.GetActor();

	if (!HitActor->Implements<UMD_InteractInterface>())
	{
		ClearInteractionFocus();
		return;
	}

	const bool bCanInteract = IMD_InteractInterface::Execute_CanInteract(HitActor);
	if (!bCanInteract)
	{
		ClearInteractionFocus();
		return;
	}

	if (CurrentFocusedInteractable == HitActor)
	{
		return;
	}

	ClearInteractionFocus();

	CurrentFocusedInteractable = HitActor;
	UE_LOG(LogTemp, Warning, TEXT("Hit"));
	IMD_InteractInterface::Execute_SetInteractPromptVisible(CurrentFocusedInteractable, true);
	IMD_InteractInterface::Execute_Highlight(CurrentFocusedInteractable, true);
}