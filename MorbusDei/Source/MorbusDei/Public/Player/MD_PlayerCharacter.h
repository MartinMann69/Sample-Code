#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MD_PlayerCharacter.generated.h"

class APlayerController;
class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class UMD_FoleyEventRelayComponent;
class UMD_PlayerHapticFeedbackComponent;
class UMD_PlayerInspectComponent;
class UMD_PlayerInteractionComponent;
class USpringArmComponent;
struct FInputActionValue;

UCLASS()
class MORBUSDEI_API AMD_PlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMD_PlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float GamepadSensitivity = 1.0f;

protected:
	UPROPERTY()
	APlayerController* CachedPlayerController = nullptr;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere)
	UMD_PlayerInteractionComponent* InteractionComp;

	UPROPERTY(VisibleAnywhere)
	UMD_PlayerInspectComponent* InspectComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Feedback")
	UMD_FoleyEventRelayComponent* FoleyEventRelayComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Haptics")
	UMD_PlayerHapticFeedbackComponent* HapticFeedbackComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InspectAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ZoomAction;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);
};
