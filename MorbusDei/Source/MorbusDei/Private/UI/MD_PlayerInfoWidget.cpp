#include "UI/MD_PlayerInfoWidget.h"

#include "Audio/MD_AudioZone.h"

void UMD_PlayerInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!VoiceLineDelegateHandle.IsValid())
	{
		VoiceLineDelegateHandle = AMD_AudioZone::OnVoiceLinePlaybackChanged.AddUObject(this, &UMD_PlayerInfoWidget::HandleVoiceLinePlaybackChanged);
	}

	HandleVoiceLinePlaybackChanged(AMD_AudioZone::IsAnyVoiceLinePlaying());
}

void UMD_PlayerInfoWidget::NativeDestruct()
{
	if (VoiceLineDelegateHandle.IsValid())
	{
		AMD_AudioZone::OnVoiceLinePlaybackChanged.Remove(VoiceLineDelegateHandle);

		VoiceLineDelegateHandle.Reset();
	}

	Super::NativeDestruct();
}

void UMD_PlayerInfoWidget::HandleVoiceLinePlaybackChanged(bool bIsPlaying)
{
	UpdateInteractionLabel(bIsPlaying);
}