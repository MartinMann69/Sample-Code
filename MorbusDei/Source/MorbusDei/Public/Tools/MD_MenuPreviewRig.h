#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MD_MenuPreviewRig.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class URectLightComponent;

UCLASS()
class MORBUSDEI_API AMD_MenuPreviewRig : public AActor
{
	GENERATED_BODY()

public:
	AMD_MenuPreviewRig();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void ShowPreview(TSubclassOf<AActor> PreviewClass);

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void ClearPreview();

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void RotatePreview(float DeltaX, float DeltaY);

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void ZoomPreview(float WheelDelta);

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void PanPreview(float HorizontalDirection, float VerticalDirection, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void ResetCameraPan();

	UFUNCTION(BlueprintCallable, Category = "MD|Menu Preview")
	void ResetPreviewView();

	UFUNCTION(BlueprintPure, Category = "MD|Menu Preview")
	bool HasPreview() const { return IsValid(CurrentPreviewActor); }

	/** 0 is fully zoomed in at MinZoom, 1 is fully zoomed out at MaxZoom. */
	UFUNCTION(BlueprintPure, Category = "MD|Menu Preview|Zoom")
	float GetNormalizedZoomDistance() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview")
	TObjectPtr<USceneComponent> PreviewPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview")
	TObjectPtr<USceneComponent> SpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview")
	TObjectPtr<UCameraComponent> PreviewCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview")
	TObjectPtr<USceneComponent> PreviewLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview")
	float RotationSpeed = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview")
	float ZoomSpeed = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview")
	float CameraPanSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview", meta = (ClampMin = "0.0"))
	float MaxCameraPanOffset = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview", meta = (ClampMin = "0.1"))
	float CameraPanInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview|Zoom", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview")
	float MinZoom = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview")
	float MaxZoom = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview|Zoom")
	float DefaultZoomDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview|Zoom", meta = (ClampMin = "0.1"))
	float AutomaticDistanceMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview|Zoom", meta = (ClampMin = "0.0"))
	float AutomaticDistancePadding = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview|Framing",
		meta = (ToolTip = "Positive X moves the preview center right. Positive Y moves it up."))
	FVector2D PreviewFramingOffset = FVector2D(90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview|Framing", meta = (ClampMin = "1.0"))
	float PreviewFramingReferenceDistance = 700.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Menu Preview|Lighting")
	TObjectPtr<URectLightComponent> RectLight3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Menu Preview")
	FRotator PreviewSpawnRotation = FRotator::ZeroRotator;

private:
	UPROPERTY()
	TObjectPtr<AActor> CurrentPreviewActor;

	float PreviewYaw = 0.0f;
	float PreviewPitch = 0.0f;
	float CurrentZoomDistance = 700.0f;
	float TargetZoomDistance = 700.0f;
	float CameraZoomReferenceDistance = 700.0f;
	float InitialPreviewZoomDistance = 700.0f;
	FVector DefaultPreviewPivotRelativeLocation = FVector::ZeroVector;
	FVector DefaultPreviewLightRelativeLocation = FVector::ZeroVector;
	FVector DefaultSpringArmRelativeLocation = FVector::ZeroVector;
	FVector CameraPanOffset = FVector::ZeroVector;
	FVector TargetCameraPanOffset = FVector::ZeroVector;

	bool GetPreviewActorBounds(AActor* Actor, FVector& OutCenter, FVector& OutExtent) const;
	float GetClampedZoomDistance(float Distance) const;
	float GetAutomaticZoomDistance(const FVector& BoundsExtent) const;
	FVector GetPreviewFramingLocalOffset() const;
	void SetZoomDistanceImmediate(float Distance);
	void ApplyZoomDistance();
	void ApplyCameraPanOffset();
};
