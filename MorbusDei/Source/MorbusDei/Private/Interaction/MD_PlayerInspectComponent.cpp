#include "Interaction/MD_PlayerInspectComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Interaction/MD_InspectableComponent.h"

UMD_PlayerInspectComponent::UMD_PlayerInspectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMD_PlayerInspectComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());
	EnsureInspectPivot();
}

void UMD_PlayerInspectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (InspectState != EMD_InspectState::Inactive && !HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	switch (InspectState)
	{
	case EMD_InspectState::Entering:
		UpdateEnterTransition(DeltaTime);
		break;

	case EMD_InspectState::Active:
		UpdateSmoothZoom(DeltaTime);
		if (InspectState == EMD_InspectState::Active)
		{
			UpdateSmoothRotation(DeltaTime);
		}
		break;

	case EMD_InspectState::Exiting:
		UpdateExitTransition(DeltaTime);
		break;

	case EMD_InspectState::Inactive:
	default:
		break;
	}
}

bool UMD_PlayerInspectComponent::StartInspect(UMD_InspectableComponent* Inspectable)
{
	if (InspectState != EMD_InspectState::Inactive)
	{
		return false;
	}

	if (!IsValid(Inspectable) || !Inspectable->CanInspect())
	{
		return false;
	}

	if (!IsValid(OwningPawn))
	{
		OwningPawn = Cast<APawn>(GetOwner());
	}

	if (!IsValid(OwningPawn) || !OwningPawn->GetController())
	{
		return false;
	}

	EnsureInspectPivot();

	if (!IsValid(InspectPivot))
	{
		return false;
	}

	StopOwnerMovementForInspect();

	if (!UpdateInspectViewFromCamera())
	{
		return false;
	}

	CurrentInspectDistance = Inspectable->GetDesiredInspectDistance();

	TargetInspectDistance = CurrentInspectDistance;

	RotationVelocity = FVector2D::ZeroVector;
	CurrentInspectYaw = 0.f;
	CurrentInspectPitch = 0.f;

	CurrentInspectable = Inspectable;

	const FTransform DesiredPivotTransform = MakeDesiredPivotTransform(CurrentInspectable);

	if (!CurrentInspectable->StartInspect(OwningPawn, InspectPivot))
	{
		CurrentInspectable = nullptr;
		return false;
	}

	OriginalPivotTransform = InspectPivot->GetComponentTransform();

	TransitionStartTransform = OriginalPivotTransform;
	TransitionTargetTransform = DesiredPivotTransform;
	TransitionElapsed = 0.f;

	SetInspectState(EMD_InspectState::Entering);
	return true;
}

void UMD_PlayerInspectComponent::EndInspect()
{
	if (InspectState == EMD_InspectState::Inactive)
	{
		return;
	}

	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	if (InspectState == EMD_InspectState::Exiting)
	{
		return;
	}

	TransitionStartTransform = InspectPivot->GetComponentTransform();
	TransitionTargetTransform = OriginalPivotTransform;
	TransitionElapsed = 0.f;
	RotationVelocity = FVector2D::ZeroVector;

	SetInspectState(EMD_InspectState::Exiting);
}

void UMD_PlayerInspectComponent::SetInspectState(EMD_InspectState NewState)
{
	InspectState = NewState;

	SetComponentTickEnabled(InspectState != EMD_InspectState::Inactive);
}

bool UMD_PlayerInspectComponent::HasValidInspectSession() const
{
	return IsValid(CurrentInspectable) && IsValid(InspectPivot);
}

void UMD_PlayerInspectComponent::ForceCleanupInspection()
{
	UMD_InspectableComponent* InspectableToRestore = IsValid(CurrentInspectable) ? CurrentInspectable : nullptr;

	CurrentInspectable = nullptr;

	if (IsValid(InspectPivot))
	{
		InspectPivot->SetWorldTransform(OriginalPivotTransform);
	}

	if (InspectableToRestore)
	{
		InspectableToRestore->EndInspect();
	}

	CurrentInspectDistance = 0.f;
	TargetInspectDistance = 0.f;
	TransitionElapsed = 0.f;
	RotationVelocity = FVector2D::ZeroVector;
	CurrentInspectYaw = 0.f;
	CurrentInspectPitch = 0.f;
	OriginalPivotTransform = FTransform::Identity;
	TransitionStartTransform = FTransform::Identity;
	TransitionTargetTransform = FTransform::Identity;

	SetInspectState(EMD_InspectState::Inactive);
}

void UMD_PlayerInspectComponent::UpdateEnterTransition(float DeltaTime)
{
	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	if (!UpdateInspectViewFromCamera())
	{
		ForceCleanupInspection();
		return;
	}
	TransitionTargetTransform = MakeDesiredPivotTransform(CurrentInspectable);

	TransitionElapsed += DeltaTime;

	const float Duration = CurrentInspectable->GetEnterDuration();

	const float Alpha = Duration <= KINDA_SMALL_NUMBER ? 1.f : FMath::Clamp(TransitionElapsed / Duration, 0.f, 1.f);

	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	FTransform NewTransform;
	NewTransform.Blend(TransitionStartTransform, TransitionTargetTransform, SmoothAlpha);

	InspectPivot->SetWorldTransform(NewTransform);

	if (Alpha >= 1.f)
	{
		InspectPivot->SetWorldTransform(TransitionTargetTransform);
		SetInspectState(EMD_InspectState::Active);
	}
}

void UMD_PlayerInspectComponent::UpdateExitTransition(float DeltaTime)
{
	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	TransitionElapsed += DeltaTime;

	const float Duration = CurrentInspectable->GetExitDuration();

	const float Alpha = Duration <= KINDA_SMALL_NUMBER ? 1.f : FMath::Clamp(TransitionElapsed / Duration, 0.f, 1.f);

	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	FTransform NewTransform;
	NewTransform.Blend(TransitionStartTransform, TransitionTargetTransform, SmoothAlpha);

	InspectPivot->SetWorldTransform(NewTransform);

	if (Alpha >= 1.f)
	{
		InspectPivot->SetWorldTransform(TransitionTargetTransform);

		UMD_InspectableComponent* FinishedInspectable = CurrentInspectable;

		CurrentInspectable = nullptr;
		FinishedInspectable->EndInspect();

		SetInspectState(EMD_InspectState::Inactive);
	}
}

void UMD_PlayerInspectComponent::RotateInspectedItem(const FVector2D& LookInput)
{
	if (InspectState != EMD_InspectState::Active)
	{
		return;
	}

	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	if (!CurrentInspectable->CanRotateDuringInspect())
	{
		return;
	}

	RotationVelocity += LookInput * CurrentInspectable->GetRotationSpeed();

	if (MaxRotationVelocity > 0.f)
	{
		RotationVelocity = RotationVelocity.GetClampedToMaxSize(MaxRotationVelocity);
	}
}

void UMD_PlayerInspectComponent::UpdateSmoothRotation(float DeltaTime)
{
	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	if (RotationVelocity.IsNearlyZero())
	{
		return;
	}

	CurrentInspectYaw += RotationVelocity.X;

	float PitchAmount = RotationVelocity.Y;

	if (bLimitInspectPitch)
	{
		const float NewPitch = FMath::Clamp(CurrentInspectPitch + PitchAmount, MinInspectPitch, MaxInspectPitch);

		PitchAmount = NewPitch - CurrentInspectPitch;
		CurrentInspectPitch = NewPitch;

		if (FMath::IsNearlyZero(PitchAmount))
		{
			RotationVelocity.Y = 0.f;
		}
	}
	else
	{
		CurrentInspectPitch += PitchAmount;
	}

	InspectPivot->SetWorldTransform(MakeDesiredPivotTransform(CurrentInspectable));

	RotationVelocity.X = FMath::FInterpTo(RotationVelocity.X, 0.f, DeltaTime, RotationDamping);

	RotationVelocity.Y = FMath::FInterpTo(RotationVelocity.Y, 0.f, DeltaTime, RotationDamping);
}

void UMD_PlayerInspectComponent::ZoomInspectedItem(float ZoomInput)
{
	if (InspectState != EMD_InspectState::Active)
	{
		return;
	}

	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	const float MinDistance = FMath::Min(CurrentInspectable->GetMinInspectDistance(), CurrentInspectable->GetMaxInspectDistance());

	const float MaxDistance = FMath::Max(CurrentInspectable->GetMinInspectDistance(), CurrentInspectable->GetMaxInspectDistance());

	TargetInspectDistance = FMath::Clamp(TargetInspectDistance + ZoomInput * CurrentInspectable->GetZoomSpeed(), MinDistance, MaxDistance);
}

void UMD_PlayerInspectComponent::UpdateSmoothZoom(float DeltaTime)
{
	if (!HasValidInspectSession())
	{
		ForceCleanupInspection();
		return;
	}

	if (!UpdateInspectViewFromCamera())
	{
		ForceCleanupInspection();
		return;
	}

	const float NewDistance = FMath::FInterpTo(CurrentInspectDistance, TargetInspectDistance, DeltaTime, ZoomInterpSpeed);

	CurrentInspectDistance = NewDistance;
	UpdateInspectPivotLocation();
}

bool UMD_PlayerInspectComponent::IsInspecting() const
{
	return InspectState != EMD_InspectState::Inactive;
}

void UMD_PlayerInspectComponent::EnsureInspectPivot()
{
	if (InspectPivot || !GetOwner())
	{
		return;
	}

	InspectPivot = NewObject<USceneComponent>(GetOwner(), TEXT("InspectPivot"));
	InspectPivot->SetMobility(EComponentMobility::Movable);

	GetOwner()->AddInstanceComponent(InspectPivot);
	InspectPivot->RegisterComponent();
}

void UMD_PlayerInspectComponent::UpdateInspectPivotLocation()
{
	if (!InspectPivot)
	{
		return;
	}

	const FVector NewLocation = InspectViewLocation + InspectViewRotation.Vector() * CurrentInspectDistance;

	InspectPivot->SetWorldLocation(NewLocation);
}

bool UMD_PlayerInspectComponent::UpdateInspectViewFromCamera()
{
	if (!OwningPawn || !OwningPawn->GetController())
	{
		return false;
	}

	OwningPawn->GetController()->GetPlayerViewPoint(InspectViewLocation, InspectViewRotation);

	return true;
}

FTransform UMD_PlayerInspectComponent::MakeDesiredPivotTransform(const UMD_InspectableComponent* Inspectable) const
{
	const FRotationMatrix ViewMatrix(InspectViewRotation);

	const FVector Forward = ViewMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = ViewMatrix.GetUnitAxis(EAxis::Y);
	const FVector Up = ViewMatrix.GetUnitAxis(EAxis::Z);

	FVector LocalOffset = FVector::ZeroVector;
	FQuat PresentationRotation = FQuat::Identity;

	if (Inspectable)
	{
		const FVector InspectOffset = Inspectable->GetInspectOffset();

		LocalOffset = Forward * InspectOffset.X + Right * InspectOffset.Y + Up * InspectOffset.Z;

		PresentationRotation = Inspectable->GetInitialInspectRotation().Quaternion();
	}

	const FQuat UserRotation = FRotator(CurrentInspectPitch, CurrentInspectYaw, 0.f).Quaternion();

	const FQuat DesiredRotation = InspectViewRotation.Quaternion() * UserRotation * PresentationRotation;

	const FVector PivotLocation = InspectViewLocation + Forward * CurrentInspectDistance + LocalOffset;

	return FTransform(DesiredRotation, PivotLocation, FVector::OneVector);
}

void UMD_PlayerInspectComponent::StopOwnerMovementForInspect()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPawn);
	if (!OwnerCharacter || !OwnerCharacter->GetCharacterMovement())
	{
		return;
	}

	OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
}