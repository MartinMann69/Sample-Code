#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_PlayerInspectComponent.generated.h"

class APawn;
class USceneComponent;
class UMD_InspectableComponent;

UENUM(BlueprintType)
enum class EMD_InspectState : uint8
{
	Inactive,
	Entering,
	Active,
	Exiting
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_PlayerInspectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_PlayerInspectComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool StartInspect(UMD_InspectableComponent* Inspectable);
	void EndInspect();
	void RotateInspectedItem(const FVector2D& LookInput);
	void ZoomInspectedItem(float ZoomInput);

	bool IsInspecting() const;

protected:
	float CurrentInspectDistance = 0.f;
	float TransitionElapsed = 0.f;
	float TargetInspectDistance = 0.f;

	UPROPERTY(EditAnywhere, Category = "MD|Inspection|Zoom", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 12.f;

	FVector2D RotationVelocity = FVector2D::ZeroVector;
	float CurrentInspectYaw = 0.f;
	float CurrentInspectPitch = 0.f;

	UPROPERTY(EditAnywhere, Category = "MD|Inspection|Rotation", meta = (ClampMin = "0.0"))
	float RotationDamping = 8.f;

	UPROPERTY(EditAnywhere, Category = "MD|Inspection|Rotation", meta = (ClampMin = "0.0"))
	float MaxRotationVelocity = 2.f;

	UPROPERTY(EditAnywhere, Category = "MD|Inspection|Rotation", meta = (EditCondition = "bLimitInspectPitch"))
	float MinInspectPitch = -90.f;

	UPROPERTY(EditAnywhere, Category = "MD|Inspection|Rotation", meta = (EditCondition = "bLimitInspectPitch"))
	float MaxInspectPitch = 90.f;

	UPROPERTY(EditAnywhere, Category = "MD|Inspection|Rotation")
	bool bLimitInspectPitch = true;

	FVector InspectViewLocation = FVector::ZeroVector;
	FRotator InspectViewRotation = FRotator::ZeroRotator;
	FTransform OriginalPivotTransform = FTransform::Identity;
	FTransform TransitionStartTransform = FTransform::Identity;
	FTransform TransitionTargetTransform = FTransform::Identity;

	UPROPERTY()
	APawn* OwningPawn = nullptr;

	UPROPERTY()
	UMD_InspectableComponent* CurrentInspectable = nullptr;

	UPROPERTY()
	USceneComponent* InspectPivot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Inspection")
	EMD_InspectState InspectState = EMD_InspectState::Inactive;

	void SetInspectState(EMD_InspectState NewState);
	bool HasValidInspectSession() const;
	void ForceCleanupInspection();

	void EnsureInspectPivot();
	void UpdateInspectPivotLocation();
	void UpdateEnterTransition(float DeltaTime);
	void UpdateExitTransition(float DeltaTime);
	void UpdateSmoothZoom(float DeltaTime);
	void UpdateSmoothRotation(float DeltaTime);

	bool UpdateInspectViewFromCamera();
	FTransform MakeDesiredPivotTransform(const UMD_InspectableComponent* Inspectable) const;
	void StopOwnerMovementForInspect();
};