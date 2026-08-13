#include "Player/MD_PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Feedback/MD_FoleyEventRelayComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Haptics/MD_PlayerHapticFeedbackComponent.h"
#include "InputActionValue.h"
#include "Interaction/MD_PlayerInspectComponent.h"
#include "Interaction/MD_PlayerInteractionComponent.h"

AMD_PlayerCharacter::AMD_PlayerCharacter()
{
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

	InteractionComp = CreateDefaultSubobject<UMD_PlayerInteractionComponent>("InteractionComponent");
	InspectComp = CreateDefaultSubobject<UMD_PlayerInspectComponent>("InspectComponent");
	FoleyEventRelayComp = CreateDefaultSubobject<UMD_FoleyEventRelayComponent>("FoleyEventRelayComponent");
	HapticFeedbackComp = CreateDefaultSubobject<UMD_PlayerHapticFeedbackComponent>("HapticFeedbackComponent");
}

void AMD_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CachedPlayerController = Cast<APlayerController>(GetController());

	ULocalPlayer* LocalPlayer = CachedPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

void AMD_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMD_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Look);
	EnhancedInput->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AMD_PlayerCharacter::Zoom);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, InteractionComp, &UMD_PlayerInteractionComponent::Interact);
	EnhancedInput->BindAction(InspectAction, ETriggerEvent::Started, InteractionComp, &UMD_PlayerInteractionComponent::Inspect);
}

void AMD_PlayerCharacter::Move(const FInputActionValue& Value)
{
	if (InspectComp && InspectComp->IsInspecting())
	{
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();

	FRotator ControlRot = Controller->GetControlRotation();
	FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
}

void AMD_PlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (InspectComp && InspectComp->IsInspecting())
	{
		InspectComp->RotateInspectedItem(LookAxisVector);
		return;
	}

	float CurrentSensitivity = MouseSensitivity;

	if (CachedPlayerController)
	{
		const float GamepadX = CachedPlayerController->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
		const float GamepadY = CachedPlayerController->GetInputAnalogKeyState(EKeys::Gamepad_RightY);

		if (FMath::Abs(GamepadX) > 0.0f || FMath::Abs(GamepadY) > 0.0f)
		{
			CurrentSensitivity = GamepadSensitivity;
		}
	}

	LookAxisVector *= CurrentSensitivity;

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AMD_PlayerCharacter::Zoom(const FInputActionValue& Value)
{
	if (!InspectComp || !InspectComp->IsInspecting())
	{
		return;
	}

	const float ZoomInput = Value.Get<float>();
	InspectComp->ZoomInspectedItem(ZoomInput);
}
