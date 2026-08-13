#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MD_InteractInterface.h"
#include "TimerManager.h"
#include "MD_Interactable.generated.h"

class USoundBase;
class UMD_HighlightComponent;
class UMD_InspectableComponent;
class AMD_AudioZone;

UCLASS()
class MORBUSDEI_API AMD_Interactable : public AActor, public IMD_InteractInterface
{
	GENERATED_BODY()

public:
	AMD_Interactable();
	virtual void BeginPlay() override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual bool CanInteract_Implementation() const override;
	virtual void Highlight_Implementation(bool bHighlight) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* Root;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USceneComponent* HighlightedObjects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USceneComponent* ToggleableObjects;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Components")
	UMD_HighlightComponent* HighlightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Components")
	UMD_InspectableComponent* InspectableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Interaction")
	bool bCanInteract = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	TSubclassOf<AMD_AudioZone> AudioZoneClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	bool bSpawnAudioZoneOnlyOnce = true;

	UPROPERTY()
	AMD_AudioZone* SpawnedAudioZone = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Interaction")
	USoundBase* InteractionSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Inspection")
	USoundBase* InspectSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Interaction", meta = (ClampMin = "0.0"))
	float VoiceoverDelayAfterInteractionSound = 0.0f;

	UFUNCTION()
	void HandleSpawnedAudioZoneDestroyed(AActor* DestroyedActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "MD|Audio")
	void OnSpawnedAudioZoneFinished();

	void PlayAssignedAudioZone(APawn* Interactor);
	void PlayOneShotAtSelf(USoundBase* Sound) const;
	bool IsVoiceoverAudioBlocked() const;

	FTimerHandle VoiceoverDelayTimer;
	bool bVoiceoverStartPending = false;
};
