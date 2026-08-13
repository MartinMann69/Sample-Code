#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MD_GameInstance.generated.h"

class UMD_AudioSettingsSave;
class USoundClass;
class USoundMix;

UCLASS()
class MORBUSDEI_API UMD_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void SetMasterVolume(float Value);

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void SetMusicVolume(float Value);

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void SetSFXVolume(float Value);

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void SetVoiceVolume(float Value);

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	void ApplyAudioSettings();

	UFUNCTION(BlueprintCallable, Category = "MD|Audio")
	bool SaveAudioSettings();

	UFUNCTION(BlueprintPure, Category = "MD|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "MD|Audio")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintPure, Category = "MD|Audio")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "MD|Audio")
	float GetVoiceVolume() const;

protected:
	virtual void OnStart() override;

	// Assign SM_VolumeSettings in the Blueprint child.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Audio")
	TObjectPtr<USoundMix> VolumeSettings = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Audio")
	TObjectPtr<USoundClass> MusicSoundClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Audio")
	TObjectPtr<USoundClass> SFXSoundClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Audio")
	TObjectPtr<USoundClass> VoiceSoundClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Audio")
	TArray<TObjectPtr<USoundClass>> MasterOnlySoundClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Audio", meta = (ClampMin = "0.0"))
	float VolumeFadeDuration = 0.1f;

private:
	void LoadAudioSettings();
	UMD_AudioSettingsSave* GetOrCreateAudioSettings();
	void ApplyClassVolume(USoundClass* SoundClass, float Volume) const;

	UPROPERTY(Transient)
	TObjectPtr<UMD_AudioSettingsSave> AudioSettings = nullptr;

	const FString AudioSettingsSlot = TEXT("AudioSettings_v3");
	const int32 AudioSettingsUserIndex = 0;
};