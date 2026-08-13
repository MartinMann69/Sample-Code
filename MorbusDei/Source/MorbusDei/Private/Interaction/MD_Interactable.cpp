#include "Interaction/MD_Interactable.h"

#include "Audio/MD_AudioZone.h"
#include "Engine/Engine.h"
#include "Interaction/MD_HighlightComponent.h"
#include "Interaction/MD_InspectableComponent.h"
#include "Kismet/GameplayStatics.h"

AMD_Interactable::AMD_Interactable()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	HighlightedObjects = CreateDefaultSubobject<USceneComponent>(TEXT("HighlightedObjects"));
	HighlightedObjects->SetupAttachment(Root);

	ToggleableObjects = CreateDefaultSubobject<USceneComponent>(TEXT("ToggleableObjects"));
	ToggleableObjects->SetupAttachment(Root);

	HighlightComponent = CreateDefaultSubobject<UMD_HighlightComponent>(TEXT("HighlightComponent"));
	HighlightComponent->SetHighlightRoot(HighlightedObjects);

	InspectableComponent = CreateDefaultSubobject<UMD_InspectableComponent>(TEXT("InspectableComponent"));
}

void AMD_Interactable::BeginPlay()
{
	Super::BeginPlay();

	if (InspectableComponent)
	{
		InspectableComponent->SetInspectSound(InspectSound);
	}
}

void AMD_Interactable::Interact_Implementation(APawn* Interactor)
{
	const bool bVoiceoverBlocked = IsVoiceoverAudioBlocked();

	if (!bVoiceoverBlocked)
	{
		PlayOneShotAtSelf(InteractionSound);

		if (AudioZoneClass)
		{
			if (VoiceoverDelayAfterInteractionSound > 0.0f)
			{
				bVoiceoverStartPending = true;

				FTimerDelegate VoiceoverDelegate;
				VoiceoverDelegate.BindUObject(this, &AMD_Interactable::PlayAssignedAudioZone, Interactor);

				GetWorldTimerManager().SetTimer(VoiceoverDelayTimer, VoiceoverDelegate, VoiceoverDelayAfterInteractionSound, false);
			}
			else
			{
				PlayAssignedAudioZone(Interactor);
			}
		}
	}

	if (!ToggleableObjects)
	{
		return;
	}

	TArray<USceneComponent*> ToggleChildComponents;
	ToggleableObjects->GetChildrenComponents(true, ToggleChildComponents);

	for (USceneComponent* Child : ToggleChildComponents)
	{
		Child->ToggleVisibility();
	}
}

bool AMD_Interactable::CanInteract_Implementation() const
{
	return bCanInteract;
}

void AMD_Interactable::Highlight_Implementation(bool bHighlight)
{
	if (HighlightComponent)
	{
		HighlightComponent->SetHighlighted(bHighlight);
	}
}

void AMD_Interactable::PlayAssignedAudioZone(APawn* Interactor)
{
	bVoiceoverStartPending = false;

	if (!AudioZoneClass)
	{
		return;
	}

	if (IsValid(SpawnedAudioZone) && SpawnedAudioZone->IsZoneSoundPlaying())
	{
		return;
	}

	const bool bShouldSpawnAudioZone = !IsValid(SpawnedAudioZone) && (!bSpawnAudioZoneOnlyOnce || SpawnedAudioZone == nullptr);

	if (bShouldSpawnAudioZone)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Interactor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedAudioZone = GetWorld()->SpawnActor<AMD_AudioZone>(AudioZoneClass, GetActorTransform(), SpawnParams);

		if (IsValid(SpawnedAudioZone))
		{
			SpawnedAudioZone->OnDestroyed.AddDynamic(this, &AMD_Interactable::HandleSpawnedAudioZoneDestroyed);
		}
	}

	if (IsValid(SpawnedAudioZone))
	{
		SpawnedAudioZone->PlayZoneSound();
	}
}

void AMD_Interactable::HandleSpawnedAudioZoneDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == SpawnedAudioZone)
	{
		OnSpawnedAudioZoneFinished();
	}
}

void AMD_Interactable::PlayOneShotAtSelf(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

bool AMD_Interactable::IsVoiceoverAudioBlocked() const
{
	return bVoiceoverStartPending || (IsValid(SpawnedAudioZone) && SpawnedAudioZone->IsZoneSoundPlaying());
}
