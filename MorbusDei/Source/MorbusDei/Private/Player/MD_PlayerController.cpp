#include "Player/MD_PlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Containers/Ticker.h"
#include "Input/MD_InputDeviceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MD_PauseMenuWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogMDPlayerController, Log, All);

bool AMD_PlayerController::OpenPauseMenu()
{
	if (PauseMenuState != EPauseMenuState::Closed)
	{
		return false;
	}

	if (!ExecutePauseMenuLayerToggle())
	{
		RestoreGameplayInput();
		return false;
	}

	if (!ActivePauseMenuWidget.IsValid())
	{
		UE_LOG(LogMDPlayerController, Error, TEXT("Pause layer toggle did not construct a UMD_PauseMenuWidget. Reparent WBP_PauseMenu to the native pause widget class."));

		// Roll back a layer that failed to create the expected widget.
		PauseMenuState = EPauseMenuState::Open;
		ExecutePauseMenuLayerToggle();
		PauseMenuState = EPauseMenuState::Closed;
		RestoreGameplayInput();
		return false;
	}

	if (!ApplyPauseMenuInput(ActivePauseMenuWidget.Get()))
	{
		UE_LOG(LogMDPlayerController, Error, TEXT("Failed to apply pause-menu input state; rolling the layer push back."));
		ExecutePauseMenuLayerToggle();
		ActivePauseMenuWidget.Reset();
		PauseMenuState = EPauseMenuState::Closed;
		RestoreGameplayInput();
		return false;
	}

	return true;
}

bool AMD_PlayerController::RequestClosePauseMenu()
{
	UMD_PauseMenuWidget* PauseMenuWidget = ActivePauseMenuWidget.Get();
	if (PauseMenuState != EPauseMenuState::Open || !PauseMenuWidget)
	{
		return false;
	}

	PauseMenuState = EPauseMenuState::Closing;
	if (!PauseMenuWidget->BeginCloseTransition())
	{
		PauseMenuState = EPauseMenuState::Open;
		return false;
	}

	return true;
}

bool AMD_PlayerController::IsPauseMenuOpen() const
{
	return PauseMenuState != EPauseMenuState::Closed;
}

void AMD_PlayerController::RegisterPauseMenuWidget(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (!IsValid(PauseMenuWidget))
	{
		return;
	}

	if (UMD_PauseMenuWidget* PreviousWidget = ActivePauseMenuWidget.Get())
	{
		if (PreviousWidget == PauseMenuWidget)
		{
			return;
		}

		PreviousWidget->OnCloseTransitionFinished().Remove(CloseTransitionFinishedHandle);
		UE_LOG(LogMDPlayerController, Warning, TEXT("Replacing an active pause widget. Previous=%s New=%s"), *GetNameSafe(PreviousWidget), *GetNameSafe(PauseMenuWidget));
	}

	ActivePauseMenuWidget = PauseMenuWidget;
	CloseTransitionFinishedHandle = PauseMenuWidget->OnCloseTransitionFinished().AddUObject(this, &AMD_PlayerController::HandleCloseTransitionFinished);
	PauseMenuState = EPauseMenuState::Open;
}

void AMD_PlayerController::UnregisterPauseMenuWidget(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (ActivePauseMenuWidget.Get() != PauseMenuWidget)
	{
		return;
	}

	if (PauseMenuWidget)
	{
		PauseMenuWidget->OnCloseTransitionFinished().Remove(CloseTransitionFinishedHandle);
	}
	CloseTransitionFinishedHandle.Reset();
	ActivePauseMenuWidget.Reset();

	if (PauseMenuState != EPauseMenuState::Closing)
	{
		PauseMenuState = EPauseMenuState::Closed;
	}
}

void AMD_PlayerController::HandleCloseTransitionFinished(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (PauseMenuState != EPauseMenuState::Closing || ActivePauseMenuWidget.Get() != PauseMenuWidget)
	{
		return;
	}

	FinalizePauseMenuClose();
}

void AMD_PlayerController::FinalizePauseMenuClose()
{
	UMD_PauseMenuWidget* PauseMenuWidget = ActivePauseMenuWidget.Get();
	if (PauseMenuWidget)
	{
		PauseMenuWidget->OnCloseTransitionFinished().Remove(CloseTransitionFinishedHandle);
	}
	CloseTransitionFinishedHandle.Reset();

	const bool bLayerToggleExecuted = ExecutePauseMenuLayerToggle();
	if (!bLayerToggleExecuted && PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
	}

	ActivePauseMenuWidget.Reset();
	PauseMenuState = EPauseMenuState::Closed;
	RestoreGameplayInput();
}

bool AMD_PlayerController::ExecutePauseMenuLayerToggle()
{
	// The Blueprint layer stack still exposes this compatibility function.
	static const FName ToggleFunctionName(TEXT("TogglePauseMenu"));
	UFunction* ToggleFunction = FindFunction(ToggleFunctionName);
	if (!ToggleFunction)
	{
		UE_LOG(LogMDPlayerController, Error, TEXT("BP_MainPlayerController must provide the existing TogglePauseMenu layer bridge."));
		return false;
	}

	ProcessEvent(ToggleFunction, nullptr);
	return true;
}

bool AMD_PlayerController::ApplyPauseMenuInput(UMD_PauseMenuWidget* PauseMenuWidget)
{
	if (!IsValid(PauseMenuWidget) || !UGameplayStatics::SetGamePaused(this, true))
	{
		return false;
	}
	EnablePauseMenuFullTick();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	const UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>() : nullptr;
	bShowMouseCursor = !InputDeviceSubsystem || !InputDeviceSubsystem->IsUsingGamepad();
	return true;
}

bool AMD_PlayerController::RestorePauseMenuFocus()
{
	UMD_PauseMenuWidget* PauseMenuWidget = ActivePauseMenuWidget.Get();
	if (PauseMenuState != EPauseMenuState::Open || !IsValid(PauseMenuWidget))
	{
		return false;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	const UMD_InputDeviceSubsystem* InputDeviceSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMD_InputDeviceSubsystem>() : nullptr;
	bShowMouseCursor = !InputDeviceSubsystem || !InputDeviceSubsystem->IsUsingGamepad();

	// A world timer cannot run reliably while the world remains paused.
	const TWeakObjectPtr<AMD_PlayerController> WeakPlayerController(this);
	const TWeakObjectPtr<UMD_PauseMenuWidget> WeakPauseMenuWidget(PauseMenuWidget);
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakPlayerController, WeakPauseMenuWidget](const float DeltaTime)
			{
				AMD_PlayerController* PlayerController = WeakPlayerController.Get();
				UMD_PauseMenuWidget* DeferredPauseMenuWidget = WeakPauseMenuWidget.Get();
				if (PlayerController && DeferredPauseMenuWidget && PlayerController->PauseMenuState == EPauseMenuState::Open && PlayerController->ActivePauseMenuWidget.Get() == DeferredPauseMenuWidget)
				{
					DeferredPauseMenuWidget->InitializeFocusScreen(true);
				}
				return false;
			}
		)
	);
	return true;
}

void AMD_PlayerController::RestoreGameplayInput()
{
	UGameplayStatics::SetGamePaused(this, false);
	RestorePauseMenuFullTick();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
	FlushPressedKeys();
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void AMD_PlayerController::EnablePauseMenuFullTick()
{
	if (bPauseMenuOverridesFullTick)
	{
		return;
	}

	bPreviousFullTickWhenPaused = bShouldPerformFullTickWhenPaused;
	bShouldPerformFullTickWhenPaused = true;
	bPauseMenuOverridesFullTick = true;
}

void AMD_PlayerController::RestorePauseMenuFullTick()
{
	if (!bPauseMenuOverridesFullTick)
	{
		return;
	}

	bShouldPerformFullTickWhenPaused = bPreviousFullTickWhenPaused;
	bPauseMenuOverridesFullTick = false;
}
