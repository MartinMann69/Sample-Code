#include "MD_GameInstance.h"

#include "Audio/MD_AudioSettingsSave.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"

void UMD_GameInstance::Init()
{
	Super::Init();

	LoadAudioSettings();
}

void UMD_GameInstance::OnStart()
{
	Super::OnStart();

	if (VolumeSettings)
	{
		UGameplayStatics::SetBaseSoundMix(this, VolumeSettings);
	}

	ApplyAudioSettings();
}

void UMD_GameInstance::SetMasterVolume(float Value)
{
	UMD_AudioSettingsSave* Settings = GetOrCreateAudioSettings();
	Settings->MasterVolume = FMath::Clamp(Value, 0.0f, 1.0f);

	ApplyAudioSettings();
}

void UMD_GameInstance::SetMusicVolume(float Value)
{
	UMD_AudioSettingsSave* Settings = GetOrCreateAudioSettings();
	Settings->MusicVolume = FMath::Clamp(Value, 0.0f, 1.0f);

	ApplyAudioSettings();
}

void UMD_GameInstance::SetSFXVolume(float Value)
{
	UMD_AudioSettingsSave* Settings = GetOrCreateAudioSettings();
	Settings->SFXVolume = FMath::Clamp(Value, 0.0f, 1.0f);

	ApplyAudioSettings();
}

void UMD_GameInstance::SetVoiceVolume(float Value)
{
	UMD_AudioSettingsSave* Settings = GetOrCreateAudioSettings();
	Settings->VoiceVolume = FMath::Clamp(Value, 0.0f, 1.0f);

	ApplyAudioSettings();
}

void UMD_GameInstance::ApplyAudioSettings()
{
	UMD_AudioSettingsSave* Settings = GetOrCreateAudioSettings();

	if (!Settings || !VolumeSettings)
	{
		return;
	}

	const float Master = FMath::Clamp(Settings->MasterVolume, 0.0f, 1.0f);
	const float Music = FMath::Clamp(Settings->MusicVolume, 0.0f, 1.0f);
	const float SFX = FMath::Clamp(Settings->SFXVolume, 0.0f, 1.0f);
	const float Voice = FMath::Clamp(Settings->VoiceVolume, 0.0f, 1.0f);

	for (USoundClass* SoundClass : MasterOnlySoundClasses)
	{
		ApplyClassVolume(SoundClass, Master);
	}

	ApplyClassVolume(MusicSoundClass, Master * Music);
	ApplyClassVolume(SFXSoundClass, Master * SFX);
	ApplyClassVolume(VoiceSoundClass, Master * Voice);
}

bool UMD_GameInstance::SaveAudioSettings()
{
	UMD_AudioSettingsSave* Settings = GetOrCreateAudioSettings();

	return Settings && UGameplayStatics::SaveGameToSlot(Settings, AudioSettingsSlot, AudioSettingsUserIndex);
}

float UMD_GameInstance::GetMasterVolume() const
{
	return AudioSettings ? AudioSettings->MasterVolume : 1.0f;
}

float UMD_GameInstance::GetMusicVolume() const
{
	return AudioSettings ? AudioSettings->MusicVolume : 1.0f;
}

float UMD_GameInstance::GetSFXVolume() const
{
	return AudioSettings ? AudioSettings->SFXVolume : 1.0f;
}

float UMD_GameInstance::GetVoiceVolume() const
{
	return AudioSettings ? AudioSettings->VoiceVolume : 1.0f;
}

void UMD_GameInstance::LoadAudioSettings()
{
	if (UGameplayStatics::DoesSaveGameExist(AudioSettingsSlot, AudioSettingsUserIndex))
	{
		AudioSettings = Cast<UMD_AudioSettingsSave>(UGameplayStatics::LoadGameFromSlot(AudioSettingsSlot, AudioSettingsUserIndex));
	}

	if (!AudioSettings)
	{
		AudioSettings = Cast<UMD_AudioSettingsSave>(UGameplayStatics::CreateSaveGameObject(UMD_AudioSettingsSave::StaticClass()));

		SaveAudioSettings();
	}
}

UMD_AudioSettingsSave* UMD_GameInstance::GetOrCreateAudioSettings()
{
	if (!AudioSettings)
	{
		AudioSettings = Cast<UMD_AudioSettingsSave>(UGameplayStatics::CreateSaveGameObject(UMD_AudioSettingsSave::StaticClass()));
	}

	return AudioSettings;
}

void UMD_GameInstance::ApplyClassVolume(USoundClass* SoundClass, float Volume) const
{
	if (!VolumeSettings || !SoundClass)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(this, VolumeSettings, SoundClass, FMath::Clamp(Volume, 0.0f, 1.0f), 1.0f, VolumeFadeDuration, false);
}