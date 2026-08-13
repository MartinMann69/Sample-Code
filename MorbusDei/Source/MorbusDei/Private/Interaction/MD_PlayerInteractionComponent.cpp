#include "Interaction/MD_PlayerInteractionComponent.h"

#include "Audio/MD_AudioZone.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/MD_InspectableComponent.h"
#include "Interaction/MD_InteractInterface.h"
#include "Interaction/MD_PlayerInspectComponent.h"

namespace
{
	struct FMDInteractionCandidate
	{
		AActor* Actor = nullptr;
		FVector FocusLocation = FVector::ZeroVector;
		float Score = 0.0f;
	};

	TAutoConsoleVariable<int32> CVarMDInteractionDebug(TEXT("md.Interaction.Debug"), 0, TEXT("Draw interaction targeting debug. 0=Off, 1=Candidates, 2=+Aim rings, 3=+Interaction range."), ECVF_Cheat);

	bool ShouldDrawInteractionDebug(const int32 MinimumLevel = 1)
	{
		return CVarMDInteractionDebug.GetValueOnGameThread() >= MinimumLevel;
	}

	void DrawInteractionCandidateMarker(const UWorld* World, const FVector& Location, const FColor& Color, const float Radius = 6.0f)
	{
		if (!World)
		{
			return;
		}

		DrawDebugSphere(World, Location, Radius, 8, Color, false, 0.0f, 0, 0.0f);
	}

	void DrawInteractionAimRings(const UWorld* World, const FVector& ViewLocation, const FVector& ViewDirection, const float AcquireAngleDegrees, const float ReleaseAngleDegrees, const float InteractDistance)
	{
		if (!World || !ShouldDrawInteractionDebug(2))
		{
			return;
		}

		const FVector Forward = ViewDirection.GetSafeNormal();
		const FRotationMatrix ViewBasis(Forward.Rotation());
		const FVector Right = ViewBasis.GetUnitAxis(EAxis::Y);
		const FVector Up = ViewBasis.GetUnitAxis(EAxis::Z);
		const float RingDistance = FMath::Clamp(InteractDistance * 0.4f, 100.0f, 200.0f);
		const FVector RingCenter = ViewLocation + Forward * RingDistance;
		const float AcquireAngle = FMath::Clamp(AcquireAngleDegrees, 0.0f, 89.0f);
		const float ReleaseAngle = FMath::Max(AcquireAngle, FMath::Clamp(ReleaseAngleDegrees, 0.0f, 89.0f));
		const float AcquireRadius = FMath::Tan(FMath::DegreesToRadians(AcquireAngle)) * RingDistance;
		const float ReleaseRadius = FMath::Tan(FMath::DegreesToRadians(ReleaseAngle)) * RingDistance;

		DrawDebugCircle(World, RingCenter, AcquireRadius, 32, FColor(40, 150, 255), false, 0.0f, 0, 0.0f, Right, Up, false);
		DrawDebugCircle(World, RingCenter, ReleaseRadius, 32, FColor(180, 70, 255), false, 0.0f, 0, 0.0f, Right, Up, false);
	}

	void DrawInteractionRangeRing(const UWorld* World, const APawn* Pawn, const float InteractDistance)
	{
		if (!World || !IsValid(Pawn) || !ShouldDrawInteractionDebug(3) || InteractDistance <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		FVector BoundsOrigin;
		FVector BoundsExtent;
		Pawn->GetActorBounds(true, BoundsOrigin, BoundsExtent);

		FVector RingCenter = BoundsOrigin;
		RingCenter.Z -= BoundsExtent.Z - 2.0f;

		DrawDebugCircle(World, RingCenter, InteractDistance, 64, FColor(255, 130, 35), false, 0.0f, 0, 0.0f, FVector::ForwardVector, FVector::RightVector, false);
	}
}

UMD_PlayerInteractionComponent::UMD_PlayerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMD_PlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningPawn = Cast<APawn>(GetOwner());

	if (GetOwner())
	{
		InspectComp = GetOwner()->FindComponentByClass<UMD_PlayerInspectComponent>();
	}
}

void UMD_PlayerInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateInteractionFocus();
}

void UMD_PlayerInteractionComponent::Interact()
{
	if (InspectComp && InspectComp->IsInspecting())
	{
		return;
	}

	if (AMD_AudioZone::TrySkipActiveVoiceLine())
	{
		return;
	}

	if (!CurrentFocusedInteractable)
	{
		return;
	}

	if (!IMD_InteractInterface::Execute_CanInteract(CurrentFocusedInteractable))
	{
		return;
	}

	AActor* InteractedActor = CurrentFocusedInteractable;
	IMD_InteractInterface::Execute_Interact(InteractedActor, OwningPawn);
	OnInteractionExecuted.Broadcast(InteractedActor);
}

void UMD_PlayerInteractionComponent::Inspect()
{
	if (!InspectComp)
	{
		return;
	}

	if (InspectComp->IsInspecting())
	{
		InspectComp->EndInspect();
		return;
	}

	if (!CurrentFocusedInteractable)
	{
		return;
	}

	UMD_InspectableComponent* Inspectable = CurrentFocusedInteractable->FindComponentByClass<UMD_InspectableComponent>();

	if (!Inspectable || !Inspectable->CanInspect())
	{
		return;
	}

	if (InspectComp->StartInspect(Inspectable))
	{
		ClearInteractionFocus();
	}
}

void UMD_PlayerInteractionComponent::ClearInteractionFocus()
{
	SetInteractionFocus(nullptr);
}

void UMD_PlayerInteractionComponent::SetInteractionFocus(AActor* NewInteractable)
{
	if (CurrentFocusedInteractable == NewInteractable)
	{
		return;
	}

	AActor* PreviousInteractable = CurrentFocusedInteractable;
	if (IsValid(PreviousInteractable) && PreviousInteractable->Implements<UMD_InteractInterface>())
	{
		IMD_InteractInterface::Execute_Highlight(PreviousInteractable, false);
	}

	CurrentFocusedInteractable = NewInteractable;
	if (IsValid(CurrentFocusedInteractable))
	{
		IMD_InteractInterface::Execute_Highlight(CurrentFocusedInteractable, true);
	}

	OnInteractionFocusChanged.Broadcast(PreviousInteractable, CurrentFocusedInteractable);
}

void UMD_PlayerInteractionComponent::UpdateInteractionFocus()
{
	if (InspectComp && InspectComp->IsInspecting())
	{
		ClearInteractionFocus();
		return;
	}

	if (!IsValid(OwningPawn) || !OwningPawn->GetController() || !GetWorld())
	{
		ClearInteractionFocus();
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	OwningPawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection = ViewRotation.Vector();
	DrawInteractionAimRings(GetWorld(), ViewLocation, ViewDirection, FocusAcquireAngleDegrees, FocusReleaseAngleDegrees, InteractDistance);

	DrawInteractionRangeRing(GetWorld(), OwningPawn, InteractDistance);
	if (AActor* DirectCandidate = FindDirectInteractionCandidate(ViewLocation, ViewDirection))
	{
		if (ShouldDrawInteractionDebug())
		{
			DrawInteractionCandidateMarker(GetWorld(), GetInteractionFocusLocation(DirectCandidate), FColor::Cyan, 9.0f);
		}

		SetInteractionFocus(DirectCandidate);
		return;
	}

	SetInteractionFocus(FindBestInteractionCandidate(ViewLocation, ViewDirection));
}

AActor* UMD_PlayerInteractionComponent::FindDirectInteractionCandidate(const FVector& ViewLocation, const FVector& ViewDirection) const
{
	if (!GetWorld() || !IsValid(OwningPawn) || InteractDistance <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	const float CameraToPawnDistance = FVector::Distance(ViewLocation, OwningPawn->GetActorLocation());
	const FVector TraceEnd = ViewLocation + ViewDirection.GetSafeNormal() * (InteractDistance + CameraToPawnDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningPawn);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	if (!IsInteractionCandidateAvailable(HitActor))
	{
		return nullptr;
	}

	const float PawnDistanceToHit = FVector::Distance(OwningPawn->GetActorLocation(), Hit.ImpactPoint);

	return PawnDistanceToHit <= InteractDistance ? HitActor : nullptr;
}

AActor* UMD_PlayerInteractionComponent::FindBestInteractionCandidate(const FVector& ViewLocation, const FVector& ViewDirection) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(OwningPawn) || InteractDistance <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningPawn);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, OwningPawn->GetActorLocation(), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(InteractDistance), QueryParams);

	TSet<AActor*> ProcessedActors;
	TArray<FMDInteractionCandidate> Candidates;

	const FVector NormalizedViewDirection = ViewDirection.GetSafeNormal();
	const float AcquireAngle = FMath::Clamp(FocusAcquireAngleDegrees, 0.0f, 89.0f);
	const float ReleaseAngle = FMath::Max(AcquireAngle, FMath::Clamp(FocusReleaseAngleDegrees, 0.0f, 89.0f));
	const float AcquireMinimumAimDot = FMath::Cos(FMath::DegreesToRadians(AcquireAngle));
	const float ReleaseMinimumAimDot = FMath::Cos(FMath::DegreesToRadians(ReleaseAngle));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!IsValid(Candidate) || ProcessedActors.Contains(Candidate))
		{
			continue;
		}

		ProcessedActors.Add(Candidate);

		if (!IsInteractionCandidateAvailable(Candidate))
		{
			continue;
		}

		const FVector FocusLocation = GetInteractionFocusLocation(Candidate);
		const FVector ToCandidate = FocusLocation - ViewLocation;
		const float PawnDistance = FVector::Distance(OwningPawn->GetActorLocation(), FocusLocation);

		if (ToCandidate.IsNearlyZero() || PawnDistance > InteractDistance)
		{
			continue;
		}

		const float AimDot = FVector::DotProduct(NormalizedViewDirection, ToCandidate.GetSafeNormal());
		const bool bIsCurrentFocus = Candidate == CurrentFocusedInteractable;
		const float EligibilityMinimumAimDot = bIsCurrentFocus ? ReleaseMinimumAimDot : AcquireMinimumAimDot;

		if (AimDot < EligibilityMinimumAimDot)
		{
			if (ShouldDrawInteractionDebug())
			{
				DrawInteractionCandidateMarker(World, FocusLocation, FColor(90, 90, 90));
			}
			continue;
		}

		const float AimScore = AcquireMinimumAimDot < 1.0f - KINDA_SMALL_NUMBER
			? FMath::Clamp((AimDot - AcquireMinimumAimDot) / (1.0f - AcquireMinimumAimDot), 0.0f, 1.0f)
			: 1.0f;
		const float DistanceScore = 1.0f - FMath::Clamp(PawnDistance / InteractDistance, 0.0f, 1.0f);

		FMDInteractionCandidate& ScoredCandidate = Candidates.AddDefaulted_GetRef();
		ScoredCandidate.Actor = Candidate;
		ScoredCandidate.FocusLocation = FocusLocation;
		ScoredCandidate.Score = AimScore * AimAlignmentWeight + DistanceScore * DistanceWeight;

		if (bIsCurrentFocus)
		{
			ScoredCandidate.Score += CurrentFocusScoreBonus;
		}
	}

	Candidates.Sort([](const FMDInteractionCandidate& Left, const FMDInteractionCandidate& Right) { return Left.Score > Right.Score; });

	AActor* BestCandidate = nullptr;
	FVector BestFocusLocation = FVector::ZeroVector;
	const bool bDrawDebug = ShouldDrawInteractionDebug();

	for (const FMDInteractionCandidate& Candidate : Candidates)
	{
		const bool bHasLineOfSight = HasLineOfSightToCandidate(Candidate.Actor, ViewLocation, Candidate.FocusLocation);

		if (bDrawDebug)
		{
			DrawInteractionCandidateMarker(World, Candidate.FocusLocation, bHasLineOfSight ? FColor::Yellow : FColor::Red);
		}

		if (!BestCandidate && bHasLineOfSight)
		{
			BestCandidate = Candidate.Actor;
			BestFocusLocation = Candidate.FocusLocation;

			if (!bDrawDebug)
			{
				return BestCandidate;
			}
		}
	}

	if (bDrawDebug && BestCandidate)
	{
		DrawInteractionCandidateMarker(World, BestFocusLocation, FColor::Green, 9.0f);
	}

	return BestCandidate;
}

bool UMD_PlayerInteractionComponent::IsInteractionCandidateAvailable(AActor* Candidate) const
{
	if (!IsValid(Candidate) || !Candidate->Implements<UMD_InteractInterface>())
	{
		return false;
	}

	const bool bCanInteract = IMD_InteractInterface::Execute_CanInteract(Candidate);
	const UMD_InspectableComponent* Inspectable = Candidate->FindComponentByClass<UMD_InspectableComponent>();
	const bool bCanInspect = IsValid(Inspectable) && Inspectable->CanInspect();

	return bCanInteract || bCanInspect;
}

FVector UMD_PlayerInteractionComponent::GetInteractionFocusLocation(const AActor* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return FVector::ZeroVector;
	}

	FVector BoundsOrigin;
	FVector BoundsExtent;
	Candidate->GetActorBounds(true, BoundsOrigin, BoundsExtent);
	return BoundsOrigin;
}

bool UMD_PlayerInteractionComponent::HasLineOfSightToCandidate(const AActor* Candidate, const FVector& ViewLocation, const FVector& FocusLocation) const
{
	if (!IsValid(Candidate) || !GetWorld())
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningPawn);

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, FocusLocation, ECC_Visibility, QueryParams);

	return !bHit || Hit.GetActor() == Candidate;
}
