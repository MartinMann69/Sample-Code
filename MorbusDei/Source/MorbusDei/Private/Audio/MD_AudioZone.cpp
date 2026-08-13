#include "Audio/MD_AudioZone.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

TWeakObjectPtr<AMD_AudioZone> AMD_AudioZone::ActiveVoiceLineZone = nullptr;
TWeakObjectPtr<AMD_AudioZone> AMD_AudioZone::ActiveBackgroundMusicZone = nullptr;
FMDVoiceLinePlaybackChanged AMD_AudioZone::OnVoiceLinePlaybackChanged;
bool AMD_AudioZone::bLastBroadcastVoiceLinePlaying = false;

AMD_AudioZone::AMD_AudioZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetHiddenInGame(true);

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(Root);
	AudioComponent->bAutoActivate = false;
}

void AMD_AudioZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateTriggerState();
	AudioComponent->SetAttenuationSettings(AttenuationSettings);
}

void AMD_AudioZone::BeginPlay()
{
	Super::BeginPlay();

	UpdateTriggerState();

	if (UsesTrigger())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMD_AudioZone::HandleBeginOverlap);

		Trigger->OnComponentEndOverlap.AddDynamic(this, &AMD_AudioZone::HandleEndOverlap);
	}

	AudioComponent->OnAudioFinished.AddDynamic(this, &AMD_AudioZone::HandleAudioFinished);

	ConfigureAudioComponent();

	if (bPlayOnBeginPlay)
	{
		PlayZoneSound();
	}
}

void AMD_AudioZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearActiveVoiceLineIfNeeded();
	ClearActiveBackgroundMusicIfNeeded();

	BroadcastVoiceLinePlaybackState();

	Super::EndPlay(EndPlayReason);
}

void AMD_AudioZone::PlayZoneSound()
{
	if (!SoundToPlay || AudioComponent->IsPlaying())
	{
		return;
	}

	bStopRequested = false;
	bPlaybackLimitReached = false;
	bSkipRequested = false;

	ConfigureAudioComponent();
	StopCompetingVoiceLine();
	StopCompetingBackgroundMusic();

	if (!LoopParameterName.IsNone())
	{
		AudioComponent->SetBoolParameter(LoopParameterName, bLoop);
	}

	if (FadeInDuration > 0.0f)
	{
		AudioComponent->FadeIn(FadeInDuration, 1.0f);
	}
	else
	{
		AudioComponent->Play();
	}

	if (!AudioComponent->IsPlaying())
	{
		return;
	}

	OnZonePlaybackStarted.Broadcast();

	BroadcastVoiceLinePlaybackState();

	SpawnTriggeredNiagaraEffect();

	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);

	if (MaxPlaybackDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(PlaybackLimitTimer, this, &AMD_AudioZone::HandlePlaybackLimitReached, MaxPlaybackDuration, false);
	}
}

void AMD_AudioZone::StopZoneSound()
{
	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);

	if (!AudioComponent->IsPlaying())
	{
		return;
	}

	bStopRequested = true;

	if (FadeOutDuration > 0.0f)
	{
		AudioComponent->FadeOut(FadeOutDuration, 0.0f);
	}
	else
	{
		AudioComponent->Stop();
	}
}

void AMD_AudioZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bPlayWhenPlayerEnters && IsPlayerActor(OtherActor))
	{
		PlayZoneSound();
	}
}

void AMD_AudioZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (bStopWhenPlayerLeaves && IsPlayerActor(OtherActor))
	{
		StopZoneSound();
	}
}

void AMD_AudioZone::SkipZoneSound()
{
	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);

	if (!AudioComponent || !AudioComponent->IsPlaying())
	{
		return;
	}

	bStopRequested = true;
	bSkipRequested = true;

	if (SkipFadeOutDuration > 0.0f)
	{
		AudioComponent->FadeOut(SkipFadeOutDuration, 0.0f);
	}
	else
	{
		AudioComponent->Stop();
	}
}

bool AMD_AudioZone::TrySkipActiveVoiceLine()
{
	AMD_AudioZone* ActiveZone = ActiveVoiceLineZone.Get();
	if (!ActiveZone || !ActiveZone->IsZoneSoundPlaying())
	{
		return false;
	}

	ActiveZone->SkipZoneSound();
	return true;
}

bool AMD_AudioZone::IsAnyVoiceLinePlaying()
{
	AMD_AudioZone* ActiveZone = ActiveVoiceLineZone.Get();

	return IsValid(ActiveZone) && ActiveZone->IsZoneSoundPlaying();
}

void AMD_AudioZone::BroadcastVoiceLinePlaybackState()
{
	const bool bIsPlaying = IsAnyVoiceLinePlaying();

	if (bLastBroadcastVoiceLinePlaying == bIsPlaying)
	{
		return;
	}

	bLastBroadcastVoiceLinePlaying = bIsPlaying;
	OnVoiceLinePlaybackChanged.Broadcast(bIsPlaying);
}

void AMD_AudioZone::HandleAudioFinished()
{
	GetWorldTimerManager().ClearTimer(PlaybackLimitTimer);

	const bool bFinishedNaturally = !bStopRequested;
	const bool bShouldDestroy = bDestroyAfterPlayback && !bLoop && (bFinishedNaturally || bPlaybackLimitReached || bSkipRequested);

	bStopRequested = false;
	bPlaybackLimitReached = false;
	bSkipRequested = false;

	ClearActiveVoiceLineIfNeeded();
	ClearActiveBackgroundMusicIfNeeded();

	BroadcastVoiceLinePlaybackState();

	if (bShouldDestroy)
	{
		Destroy();
	}
}

void AMD_AudioZone::ConfigureAudioComponent()
{
	AudioComponent->SetSound(SoundToPlay);
	AudioComponent->SetAttenuationSettings(AttenuationSettings);
	AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
	AudioComponent->SetPitchMultiplier(PitchMultiplier);
}

bool AMD_AudioZone::IsPlayerActor(const AActor* Actor) const
{
	const APawn* Pawn = Cast<APawn>(Actor);

	return Pawn && Pawn->IsPlayerControlled();
}

void AMD_AudioZone::HandlePlaybackLimitReached()
{
	bPlaybackLimitReached = true;
	StopZoneSound();
}

bool AMD_AudioZone::UsesTrigger() const
{
	return bPlayWhenPlayerEnters || bStopWhenPlayerLeaves;
}

void AMD_AudioZone::UpdateTriggerState()
{
	const bool bUsesTrigger = UsesTrigger();

	Trigger->SetBoxExtent(TriggerExtent);
	Trigger->SetVisibility(bUsesTrigger);
	Trigger->SetHiddenInGame(true);
	Trigger->SetGenerateOverlapEvents(bUsesTrigger);
	Trigger->SetCollisionEnabled(bUsesTrigger ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool AMD_AudioZone::IsZoneSoundPlaying() const
{
	return AudioComponent && AudioComponent->IsPlaying();
}

void AMD_AudioZone::StopCompetingVoiceLine()
{
	if (!bStopOtherVoiceLinesOnPlay || bStopOtherBackgroundMusicOnPlay)
	{
		return;
	}

	if (ActiveVoiceLineZone.IsValid() && ActiveVoiceLineZone.Get() != this)
	{
		ActiveVoiceLineZone->SkipZoneSound();
	}

	ActiveVoiceLineZone = this;
}

void AMD_AudioZone::StopCompetingBackgroundMusic()
{
	if (!bStopOtherBackgroundMusicOnPlay)
	{
		return;
	}

	if (ActiveBackgroundMusicZone.IsValid() && ActiveBackgroundMusicZone.Get() != this)
	{
		ActiveBackgroundMusicZone->StopZoneSound();
	}

	ActiveBackgroundMusicZone = this;
}

void AMD_AudioZone::ClearActiveVoiceLineIfNeeded()
{
	if (ActiveVoiceLineZone.Get() == this)
	{
		ActiveVoiceLineZone.Reset();
	}
}

void AMD_AudioZone::ClearActiveBackgroundMusicIfNeeded()
{
	if (ActiveBackgroundMusicZone.Get() == this)
	{
		ActiveBackgroundMusicZone.Reset();
	}
}

void AMD_AudioZone::SpawnTriggeredNiagaraEffect() const
{
	if (!TriggerNiagaraEffect)
	{
		return;
	}

	FTransform SpawnTransform = FTransform::Identity;

	switch (NiagaraSpawnTarget)
	{
		case EMD_AudioZoneNiagaraSpawnTarget::Player:
		{
			const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
			if (!PlayerPawn)
			{
				return;
			}

			SpawnTransform = PlayerNiagaraSpawnOffset * PlayerPawn->GetActorTransform();
			break;
		}

		case EMD_AudioZoneNiagaraSpawnTarget::Actor:
		default:
		{
			if (!IsValid(NiagaraSpawnActor))
			{
				return;
			}

			SpawnTransform = NiagaraSpawnActor->GetActorTransform();
			break;
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TriggerNiagaraEffect, SpawnTransform.GetLocation(), SpawnTransform.Rotator(), SpawnTransform.GetScale3D());
}
