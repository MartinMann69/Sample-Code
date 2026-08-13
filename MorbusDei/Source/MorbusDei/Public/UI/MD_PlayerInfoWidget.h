#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MD_PlayerInfoWidget.generated.h"

class UTextBlock;

UCLASS(Abstract, Blueprintable)
class MORBUSDEI_API UMD_PlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "MD|UI")
	void UpdateInteractionLabel(bool bVoiceLinePlaying);

private:
	void HandleVoiceLinePlaybackChanged(bool bIsPlaying);

	FDelegateHandle VoiceLineDelegateHandle;
};
