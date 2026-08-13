#include "Interaction/MD_InspectableComponent.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

UMD_InspectableComponent::UMD_InspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMD_InspectableComponent::CanInspect() const
{
	return bCanInspect && !bIsInspecting;
}

bool UMD_InspectableComponent::StartInspect(APawn* Interactor, USceneComponent* InspectPivot)
{
	AActor* Owner = GetOwner();

	if (!CanInspect() || !Owner || !Interactor || !InspectPivot)
	{
		return false;
	}

	if (InspectSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InspectSound, Owner->GetActorLocation());
	}

	bIsInspecting = true;
	OriginalTransform = Owner->GetActorTransform();
	bOriginalActorCollisionEnabled = Owner->GetActorEnableCollision();

	DisableOwnerPhysics();
	Owner->SetActorEnableCollision(false);

	const FVector OriginalBoundsCenter = GetInspectableBoundsCenter();

	InspectPivot->SetWorldLocationAndRotation(OriginalBoundsCenter, Owner->GetActorRotation());

	Owner->AttachToComponent(InspectPivot, FAttachmentTransformRules::KeepWorldTransform);

	return true;
}

void UMD_InspectableComponent::EndInspect()
{
	AActor* Owner = GetOwner();

	if (!bIsInspecting || !Owner)
	{
		return;
	}

	Owner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	Owner->SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);

	RestoreOwnerPhysics();
	Owner->SetActorEnableCollision(bOriginalActorCollisionEnabled);

	bIsInspecting = false;
}

bool UMD_InspectableComponent::IsInspecting() const
{
	return bIsInspecting;
}

FVector UMD_InspectableComponent::GetInspectableBoundsCenter() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	FBox Bounds(EForceInit::ForceInit);

	TArray<UStaticMeshComponent*> MeshComponents;
	Owner->GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* MeshComp : MeshComponents)
	{
		if (MeshComp && MeshComp->GetStaticMesh())
		{
			Bounds += MeshComp->Bounds.GetBox();
		}
	}

	if (Bounds.IsValid)
	{
		return Bounds.GetCenter();
	}

	return Owner->GetActorLocation();
}

float UMD_InspectableComponent::GetInspectableBoundsRadius() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.f;
	}

	FBox Bounds(EForceInit::ForceInit);

	TArray<UStaticMeshComponent*> MeshComponents;
	Owner->GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* MeshComp : MeshComponents)
	{
		if (MeshComp && MeshComp->GetStaticMesh())
		{
			Bounds += MeshComp->Bounds.GetBox();
		}
	}

	if (Bounds.IsValid)
	{
		return Bounds.GetExtent().Size();
	}

	return 0.f;
}

void UMD_InspectableComponent::DisableOwnerPhysics()
{
	SimulatingPrimitiveComponents.Reset();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive && Primitive->IsSimulatingPhysics())
		{
			SimulatingPrimitiveComponents.Add(Primitive);
			Primitive->SetSimulatePhysics(false);
		}
	}
}

void UMD_InspectableComponent::RestoreOwnerPhysics()
{
	for (UPrimitiveComponent* Primitive : SimulatingPrimitiveComponents)
	{
		if (Primitive)
		{
			Primitive->SetSimulatePhysics(true);
		}
	}

	SimulatingPrimitiveComponents.Reset();
}

float UMD_InspectableComponent::GetDesiredInspectDistance() const
{
	const float MinDistance = FMath::Min(MinInspectDistance, MaxInspectDistance);
	const float MaxDistance = FMath::Max(MinInspectDistance, MaxInspectDistance);

	if (!bUseAutomaticDistance)
	{
		return FMath::Clamp(InspectDistance, MinDistance, MaxDistance);
	}

	const float BoundsRadius = GetInspectableBoundsRadius();

	const float AutomaticDistance = BoundsRadius * AutomaticDistanceMultiplier + AutomaticDistancePadding;

	return FMath::Clamp(AutomaticDistance, MinDistance, MaxDistance);
}