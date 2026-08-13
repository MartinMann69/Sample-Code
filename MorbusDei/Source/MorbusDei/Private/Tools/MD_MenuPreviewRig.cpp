#include "Tools/MD_MenuPreviewRig.h"

#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

AMD_MenuPreviewRig::AMD_MenuPreviewRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bFindCameraComponentWhenViewTarget = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PreviewLight = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewLight"));
	PreviewLight->SetupAttachment(RootComponent);

	PreviewPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewPivot"));
	PreviewPivot->SetupAttachment(RootComponent);

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(PreviewPivot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 700.0f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->SetRelativeLocation(FVector::ZeroVector);

	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(SpringArm);
	PreviewCamera->SetAutoActivate(true);
	PreviewCamera->SetRelativeLocation(FVector::ZeroVector);
	PreviewCamera->FieldOfView = 35.0f;

	RectLight = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight"));
	RectLight->SetupAttachment(PreviewLight);
	RectLight->SetRelativeLocation(FVector(-200.0f, -200.0f, 0.0f));
	RectLight->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
	RectLight->SetIntensity(5000.0f);
	RectLight->SetSourceWidth(550.0f);
	RectLight->SetSourceHeight(550.0f);

	RectLight1 = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight1"));
	RectLight1->SetupAttachment(PreviewLight);
	RectLight1->SetRelativeLocation(FVector(-200.0f, 200.0f, 0.0f));
	RectLight1->SetRelativeRotation(FRotator(0.0f, -45.0f, 0.0f));
	RectLight1->SetIntensity(5000.0f);
	RectLight1->SetSourceWidth(550.0f);
	RectLight1->SetSourceHeight(550.0f);

	RectLight2 = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight2"));
	RectLight2->SetupAttachment(PreviewLight);
	RectLight2->SetRelativeLocation(FVector(-200.0f, 0.0f, 200.0f));
	RectLight2->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	RectLight2->SetIntensity(5000.0f);
	RectLight2->SetSourceWidth(550.0f);
	RectLight2->SetSourceHeight(550.0f);

	RectLight3 = CreateDefaultSubobject<URectLightComponent>(TEXT("RectLight3"));
	RectLight3->SetupAttachment(PreviewLight);
	RectLight3->SetRelativeLocation(FVector(-200.0f, 0.0f, -200.0f));
	RectLight3->SetRelativeRotation(FRotator(45.0f, 0.0f, 0.0f));
	RectLight3->SetIntensity(5000.0f);
	RectLight3->SetSourceWidth(550.0f);
	RectLight3->SetSourceHeight(550.0f);
}

void AMD_MenuPreviewRig::BeginPlay()
{
	Super::BeginPlay();

	if (SpringArm)
	{
		DefaultSpringArmRelativeLocation = SpringArm->GetRelativeLocation();
		CameraZoomReferenceDistance = SpringArm->TargetArmLength;
	}

	if (PreviewPivot)
	{
		DefaultPreviewPivotRelativeLocation = PreviewPivot->GetRelativeLocation();
	}

	if (PreviewLight)
	{
		DefaultPreviewLightRelativeLocation = PreviewLight->GetRelativeLocation();
	}

	SetZoomDistanceImmediate(DefaultZoomDistance);
	SetActorTickEnabled(false);
}

void AMD_MenuPreviewRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!SpringArm)
	{
		return;
	}

	bool bNeedsTick = false;

	if (FMath::IsNearlyEqual(CurrentZoomDistance, TargetZoomDistance, 0.1f))
	{
		SetZoomDistanceImmediate(TargetZoomDistance);
	}
	else
	{
		CurrentZoomDistance = FMath::FInterpTo(CurrentZoomDistance, TargetZoomDistance, DeltaSeconds, ZoomInterpSpeed);
		ApplyZoomDistance();
		bNeedsTick = true;
	}

	if (CameraPanOffset.Equals(TargetCameraPanOffset, 0.1f))
	{
		CameraPanOffset = TargetCameraPanOffset;
		ApplyCameraPanOffset();
	}
	else
	{
		CameraPanOffset = FMath::VInterpTo(CameraPanOffset, TargetCameraPanOffset, DeltaSeconds, CameraPanInterpSpeed);
		ApplyCameraPanOffset();
		bNeedsTick = true;
	}

	if (!bNeedsTick)
	{
		SetActorTickEnabled(false);
	}
}

void AMD_MenuPreviewRig::ShowPreview(TSubclassOf<AActor> PreviewClass)
{
	if (!PreviewClass || !GetWorld() || !PreviewPivot || !SpawnPoint)
	{
		return;
	}

	ClearPreview();

	PreviewYaw = 0.0f;
	PreviewPitch = 0.0f;
	PreviewPivot->SetRelativeRotation(FRotator::ZeroRotator);
	PreviewPivot->SetRelativeLocation(DefaultPreviewPivotRelativeLocation);
	if (PreviewLight)
	{
		PreviewLight->SetRelativeLocation(DefaultPreviewLightRelativeLocation);
	}
	CurrentZoomDistance = CameraZoomReferenceDistance;
	TargetZoomDistance = CameraZoomReferenceDistance;
	if (SpringArm)
	{
		SpringArm->TargetArmLength = CameraZoomReferenceDistance;
	}
	ResetCameraPan();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform(PreviewSpawnRotation, SpawnPoint->GetComponentLocation(), SpawnPoint->GetComponentScale());

	CurrentPreviewActor = GetWorld()->SpawnActor<AActor>(PreviewClass, SpawnTransform, SpawnParams);

	if (!CurrentPreviewActor)
	{
		return;
	}

	CurrentPreviewActor->SetActorEnableCollision(false);

	float DesiredZoomDistance = GetClampedZoomDistance(DefaultZoomDistance);

	FVector MeshCenter;
	FVector MeshExtent;
	if (GetPreviewActorBounds(CurrentPreviewActor, MeshCenter, MeshExtent))
	{
		const FVector Offset = SpawnPoint->GetComponentLocation() - MeshCenter;
		CurrentPreviewActor->SetActorLocation(CurrentPreviewActor->GetActorLocation() + Offset, false, nullptr, ETeleportType::TeleportPhysics);

		DesiredZoomDistance = GetAutomaticZoomDistance(MeshExtent);
	}

	CurrentPreviewActor->AttachToComponent(PreviewPivot, FAttachmentTransformRules::KeepWorldTransform);

	SetZoomDistanceImmediate(DesiredZoomDistance);
	InitialPreviewZoomDistance = DesiredZoomDistance;
	SetActorTickEnabled(true);
}

void AMD_MenuPreviewRig::ClearPreview()
{
	if (IsValid(CurrentPreviewActor))
	{
		CurrentPreviewActor->Destroy();
	}

	CurrentPreviewActor = nullptr;
	CurrentZoomDistance = CameraZoomReferenceDistance;
	TargetZoomDistance = CameraZoomReferenceDistance;
	if (SpringArm)
	{
		SpringArm->TargetArmLength = CameraZoomReferenceDistance;
	}
	ResetCameraPan();
	if (PreviewPivot)
	{
		PreviewPivot->SetRelativeLocation(DefaultPreviewPivotRelativeLocation);
	}
	if (PreviewLight)
	{
		PreviewLight->SetRelativeLocation(DefaultPreviewLightRelativeLocation);
	}
	SetActorTickEnabled(false);
}

void AMD_MenuPreviewRig::RotatePreview(float DeltaX, float DeltaY)
{
	PreviewYaw -= DeltaX * RotationSpeed;
	PreviewPitch = FMath::Clamp(PreviewPitch + DeltaY * RotationSpeed, -180.0f, 180.0f);

	PreviewPivot->SetRelativeRotation(FRotator(PreviewPitch, PreviewYaw, 0.0f));
}

void AMD_MenuPreviewRig::ZoomPreview(float WheelDelta)
{
	if (!IsValid(CurrentPreviewActor) || !SpringArm)
	{
		return;
	}

	TargetZoomDistance = GetClampedZoomDistance(TargetZoomDistance - WheelDelta * ZoomSpeed);
	SetActorTickEnabled(true);
}

void AMD_MenuPreviewRig::PanPreview(float HorizontalDirection, float VerticalDirection, float DeltaSeconds)
{
	if (!IsValid(CurrentPreviewActor) || !SpringArm)
	{
		return;
	}

	const FVector2D PanInput(HorizontalDirection, VerticalDirection);
	if (PanInput.IsNearlyZero())
	{
		return;
	}
	const float PanMagnitude = FMath::Clamp(PanInput.Size(), 0.0f, 1.0f);

	const FVector PanWorldDirection = PreviewCamera 
		? PreviewCamera->GetRightVector() * PanInput.X + PreviewCamera->GetUpVector() * PanInput.Y
		: GetActorRightVector() * PanInput.X + GetActorUpVector() * PanInput.Y;

	if (PanWorldDirection.IsNearlyZero())
	{
		return;
	}

	const USceneComponent* AttachParent = SpringArm->GetAttachParent();
	const FVector LocalPanDelta = AttachParent 
		? AttachParent->GetComponentTransform().InverseTransformVectorNoScale(PanWorldDirection.GetSafeNormal() * CameraPanSpeed * PanMagnitude * DeltaSeconds)
		: PanWorldDirection.GetSafeNormal() * CameraPanSpeed * PanMagnitude * DeltaSeconds;

	TargetCameraPanOffset = (TargetCameraPanOffset + LocalPanDelta).GetClampedToMaxSize(MaxCameraPanOffset);
	SetActorTickEnabled(true);
}

void AMD_MenuPreviewRig::ResetCameraPan()
{
	CameraPanOffset = FVector::ZeroVector;
	TargetCameraPanOffset = FVector::ZeroVector;
	ApplyCameraPanOffset();
}

void AMD_MenuPreviewRig::ResetPreviewView()
{
	if (!IsValid(CurrentPreviewActor) || !PreviewPivot)
	{
		return;
	}

	PreviewYaw = 0.0f;
	PreviewPitch = 0.0f;
	PreviewPivot->SetRelativeRotation(FRotator::ZeroRotator);
	ResetCameraPan();
	SetZoomDistanceImmediate(InitialPreviewZoomDistance);
}

float AMD_MenuPreviewRig::GetNormalizedZoomDistance() const
{
	const float MinimumDistance = FMath::Min(MinZoom, MaxZoom);
	const float MaximumDistance = FMath::Max(MinZoom, MaxZoom);
	if (FMath::IsNearlyEqual(MinimumDistance, MaximumDistance))
	{
		return 1.0f;
	}

	return FMath::GetMappedRangeValueClamped(FVector2D(MinimumDistance, MaximumDistance), FVector2D(0.0f, 1.0f), CurrentZoomDistance);
}

bool AMD_MenuPreviewRig::GetPreviewActorBounds(AActor* Actor, FVector& OutCenter, FVector& OutExtent) const
{
	if (!Actor)
	{
		return false;
	}

	TArray<UMeshComponent*> MeshComponents;
	Actor->GetComponents<UMeshComponent>(MeshComponents);

	FBox CombinedBox(ForceInit);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent) || !MeshComponent->IsRegistered() || !MeshComponent->IsVisible())
		{
			continue;
		}

		if (MeshComponent->Bounds.BoxExtent.IsNearlyZero())
		{
			continue;
		}

		CombinedBox += MeshComponent->Bounds.GetBox();
	}

	if (!CombinedBox.IsValid)
	{
		return false;
	}

	OutCenter = CombinedBox.GetCenter();
	OutExtent = CombinedBox.GetExtent();
	return true;
}

float AMD_MenuPreviewRig::GetClampedZoomDistance(float Distance) const
{
	const float MinDistance = FMath::Min(MinZoom, MaxZoom);
	const float MaxDistance = FMath::Max(MinZoom, MaxZoom);

	return FMath::Clamp(Distance, MinDistance, MaxDistance);
}

float AMD_MenuPreviewRig::GetAutomaticZoomDistance(const FVector& BoundsExtent) const
{
	const float BoundsRadius = BoundsExtent.Size();
	const float AutomaticDistance = BoundsRadius * AutomaticDistanceMultiplier + AutomaticDistancePadding;

	return GetClampedZoomDistance(AutomaticDistance);
}

FVector AMD_MenuPreviewRig::GetPreviewFramingLocalOffset() const
{
	const float ReferenceDistance = FMath::Max(PreviewFramingReferenceDistance, 1.0f);
	const float DistanceScale = FMath::Max(CurrentZoomDistance, 1.0f) / ReferenceDistance;
	const FVector2D ScaledFramingOffset = PreviewFramingOffset * DistanceScale;
	const FVector FramingWorldOffset = PreviewCamera
		? PreviewCamera->GetRightVector() * ScaledFramingOffset.X + PreviewCamera->GetUpVector() * ScaledFramingOffset.Y
		: GetActorRightVector() * ScaledFramingOffset.X + GetActorUpVector() * ScaledFramingOffset.Y;

	const USceneComponent* PivotParent = PreviewPivot ? PreviewPivot->GetAttachParent() : nullptr;
	return PivotParent ? PivotParent->GetComponentTransform().InverseTransformVectorNoScale(FramingWorldOffset) : FramingWorldOffset;
}

void AMD_MenuPreviewRig::SetZoomDistanceImmediate(float Distance)
{
	CurrentZoomDistance = GetClampedZoomDistance(Distance);
	TargetZoomDistance = CurrentZoomDistance;

	if (SpringArm)
	{
		SpringArm->TargetArmLength = CameraZoomReferenceDistance;
	}

	ApplyZoomDistance();
}

void AMD_MenuPreviewRig::ApplyZoomDistance()
{
	if (!PreviewPivot)
	{
		return;
	}

	const float ZoomOffsetDistance = CameraZoomReferenceDistance - CurrentZoomDistance;
	FVector ZoomWorldOffset = FVector::ZeroVector;

	if (PreviewCamera)
	{
		ZoomWorldOffset = -PreviewCamera->GetForwardVector() * ZoomOffsetDistance;
	}
	else if (SpringArm)
	{
		const FVector ToCamera = SpringArm->GetSocketLocation(USpringArmComponent::SocketName) - PreviewPivot->GetComponentLocation();
		ZoomWorldOffset = ToCamera.GetSafeNormal() * ZoomOffsetDistance;
	}

	const USceneComponent* PivotParent = PreviewPivot->GetAttachParent();
	const FVector LocalZoomOffset = PivotParent ? PivotParent->GetComponentTransform().InverseTransformVectorNoScale(ZoomWorldOffset) : ZoomWorldOffset;
	const FVector LocalPlacementOffset = GetPreviewFramingLocalOffset() + LocalZoomOffset;

	PreviewPivot->SetRelativeLocation(DefaultPreviewPivotRelativeLocation + LocalPlacementOffset);
	if (PreviewLight)
	{
		PreviewLight->SetRelativeLocation(DefaultPreviewLightRelativeLocation + LocalPlacementOffset);
	}
}

void AMD_MenuPreviewRig::ApplyCameraPanOffset()
{
	if (SpringArm)
	{
		SpringArm->SetRelativeLocation(DefaultSpringArmRelativeLocation + CameraPanOffset);
	}
}
