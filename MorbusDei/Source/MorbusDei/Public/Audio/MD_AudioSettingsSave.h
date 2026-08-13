#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MD_AudioSettingsSave.generated.h"

UCLASS()
class MORBUSDEI_API UMD_AudioSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "MD|Audio")
	float MasterVolume = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "MD|Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "MD|Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "MD|Audio")
	float VoiceVolume = 1.0f;
};