#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MD_VideoManager.generated.h"

class UMediaPlayer;
class UMediaSource;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMDVideoFinished, FName, VideoId);

UCLASS()
class MORBUSDEI_API AMD_VideoManager : public AActor
{
	GENERATED_BODY()

public:
	AMD_VideoManager();

	UFUNCTION(BlueprintCallable, Category = "MD|Video")
	bool PlayVideo(UMediaSource* Source, FName VideoId, bool bCanSkip = true, bool bRestoreControl = true, bool bStopExistingSounds = false);

	UFUNCTION(BlueprintCallable, Category = "MD|Video")
	void SkipVideo();

	UPROPERTY(BlueprintAssignable, Category = "MD|Video")
	FMDVideoFinished OnVideoFinished;

protected:
	UPROPERTY(EditAnywhere, Category = "MD|Video")
	TObjectPtr<UMediaPlayer> MediaPlayer;

	UPROPERTY(EditAnywhere, Category = "MD|Video")
	TSubclassOf<UUserWidget> VideoWidgetClass;

private:
	UFUNCTION()
	void HandleEndReached();

	UFUNCTION()
	void HandleOpenFailed(FString FailedUrl);

	void FinishVideo();

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveWidget;

	TWeakObjectPtr<APlayerController> ActivePlayerController;

	FName ActiveVideoId;
	bool bPlaying = false;
	bool bCanCurrentlySkip = false;
	bool bRestoreControlAfterwards = true;
};