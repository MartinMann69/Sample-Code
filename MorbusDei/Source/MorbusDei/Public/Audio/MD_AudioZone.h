#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "MD_AudioZone.generated.h"

class UAudioComponent;
class UBoxComponent;
class USceneComponent;
class USoundAttenuation;
class USoundBase;
class UPrimitiveComponent;
class UNiagaraSystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FMDVoiceLinePlaybackChanged, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMDZonePlaybackStarted);

UENUM(BlueprintType)
enum class EMD_AudioZoneNiagaraSpawnTarget : uint8
{
	Actor UMETA(DisplayName = "Actor"),
	Player UMETA(DisplayName = "Player")
};

UCLASS()
class MORBUSDEI_API AMD_AudioZone : public AActor
{
	GENERATED_BODY()

public:
	AMD_AudioZone();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void PlayZoneSound();

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void StopZoneSound();

	UFUNCTION(BlueprintPure, Category = "MD|Audio")
	bool IsZoneSoundPlaying() const;

	static bool TrySkipActiveVoiceLine();
	static bool IsAnyVoiceLinePlaying();

	static FMDVoiceLinePlaybackChanged OnVoiceLinePlaybackChanged;

	/** Broadcast after playback starts successfully. */
	UPROPERTY(BlueprintAssignable, Category = "MD|Audio")
	FMDZonePlaybackStarted OnZonePlaybackStarted;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Audio|Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Audio|Components")
	UBoxComponent* Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Audio|Components")
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	USoundBase* SoundToPlay = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	USoundAttenuation* AttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	FVector TriggerExtent = FVector(200.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	bool bPlayOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	bool bPlayWhenPlayerEnters = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	bool bStopWhenPlayerLeaves = true;

	/** Passed to the sound parameter named by LoopParameterName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio")
	bool bLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (EditCondition = "bLoop"))
	FName LoopParameterName = TEXT("Loop");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (EditCondition = "!bLoop"))
	bool bDestroyAfterPlayback = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.01"))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.0"))
	float FadeInDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.0"))
	float FadeOutDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.0"))
	float SkipFadeOutDuration = 0.8f;

	/** Zero disables the playback limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.0"))
	float MaxPlaybackDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Voice")
	bool bStopOtherVoiceLinesOnPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Music")
	bool bStopOtherBackgroundMusicOnPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Niagara")
	UNiagaraSystem* TriggerNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Niagara", meta = (EditCondition = "TriggerNiagaraEffect != nullptr"))
	EMD_AudioZoneNiagaraSpawnTarget NiagaraSpawnTarget = EMD_AudioZoneNiagaraSpawnTarget::Actor;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "MD|Audio|Niagara", meta = (EditCondition = "TriggerNiagaraEffect != nullptr && NiagaraSpawnTarget == EMD_AudioZoneNiagaraSpawnTarget::Actor", EditConditionHides))
	AActor* NiagaraSpawnActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Audio|Niagara", meta = (EditCondition = "TriggerNiagaraEffect != nullptr && NiagaraSpawnTarget == EMD_AudioZoneNiagaraSpawnTarget::Player", EditConditionHides))
	FTransform PlayerNiagaraSpawnOffset = FTransform::Identity;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION()
	void HandleAudioFinished();
	void HandlePlaybackLimitReached();
	void ConfigureAudioComponent();
	void UpdateTriggerState();
	void StopCompetingVoiceLine();
	void StopCompetingBackgroundMusic();
	void ClearActiveVoiceLineIfNeeded();
	void ClearActiveBackgroundMusicIfNeeded();
	void SpawnTriggeredNiagaraEffect() const;
	void SkipZoneSound();

	bool IsPlayerActor(const AActor* Actor) const;
	bool UsesTrigger() const;

	bool bStopRequested = false;
	bool bPlaybackLimitReached = false;
	bool bSkipRequested = false;

	FTimerHandle PlaybackLimitTimer;

	static TWeakObjectPtr<AMD_AudioZone> ActiveVoiceLineZone;
	static TWeakObjectPtr<AMD_AudioZone> ActiveBackgroundMusicZone;

	static void BroadcastVoiceLinePlaybackState();
	static bool bLastBroadcastVoiceLinePlaying;
};
