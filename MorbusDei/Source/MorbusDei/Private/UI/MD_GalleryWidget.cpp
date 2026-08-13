#include "UI/MD_GalleryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/Widget.h"
#include "Components/WrapBox.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Tools/MD_MenuPreviewRig.h"
#include "UI/Focus/GameUIFocusItemWidgetBase.h"
#include "UI/MD_ActionHintWidget.h"

UMD_GalleryWidget::UMD_GalleryWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);

	SelectHintText.KeyboardKeyText = NSLOCTEXT("MDGallery", "SelectKeyboardKey", "Enter");
	SelectHintText.GamepadKeyText = NSLOCTEXT("MDGallery", "SelectGamepadKey", "A");
	SelectHintText.ActionText = NSLOCTEXT("MDGallery", "SelectAction", "Ansehen");

	RotateHintText.KeyboardKeyText = NSLOCTEXT("MDGallery", "RotateKeyboardKey", "LMB");
	RotateHintText.GamepadKeyText = NSLOCTEXT("MDGallery", "RotateGamepadKey", "RS");
	RotateHintText.ActionText = NSLOCTEXT("MDGallery", "RotateAction", "Drehen");

	MoveHintText.KeyboardKeyText = NSLOCTEXT("MDGallery", "MoveKeyboardKey", "WA\nSD");
	MoveHintText.GamepadKeyText = NSLOCTEXT("MDGallery", "MoveGamepadKey", "LS");
	MoveHintText.ActionText = NSLOCTEXT("MDGallery", "MoveAction", "Bewegen");

	ZoomHintText.KeyboardKeyText = NSLOCTEXT("MDGallery", "ZoomKeyboardKey", "Mausrad");
	ZoomHintText.GamepadKeyText = NSLOCTEXT("MDGallery", "ZoomGamepadKey", "LT\nRT");
	ZoomHintText.ActionText = NSLOCTEXT("MDGallery", "ZoomAction", "Zoomen");

	ResetHintText.KeyboardKeyText = NSLOCTEXT("MDGallery", "ResetKeyboardKey", "R");
	ResetHintText.GamepadKeyText = NSLOCTEXT("MDGallery", "ResetGamepadKey", "Y");
	ResetHintText.ActionText = NSLOCTEXT("MDGallery", "ResetAction", "Ansicht zurücksetzen");

	GalleryBackHintText.KeyboardKeyText = NSLOCTEXT("MDGallery", "BackKeyboardKey", "Esc");
	GalleryBackHintText.GamepadKeyText = NSLOCTEXT("MDGallery", "BackGamepadKey", "B");
	GalleryBackHintText.ActionText = NSLOCTEXT("MDGallery", "BackAction", "Zurück");

	InspectBackHintText.KeyboardKeyText = GalleryBackHintText.KeyboardKeyText;
	InspectBackHintText.GamepadKeyText = GalleryBackHintText.GamepadKeyText;
	InspectBackHintText.ActionText = NSLOCTEXT("MDGallery", "BackToSelectionAction", "Zur Auswahl");
}

void UMD_GalleryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	FindPreviewRig();
	RegisterGalleryFocusItems();
	OnBackActionHandled.AddUniqueDynamic(this, &UMD_GalleryWidget::HandleGalleryBackAction);
	RefreshActionHints();
	RequestFocusScreenActivation(true);
}

void UMD_GalleryWidget::NativeDestruct()
{
	ClearKeyboardPanInput();
	ClearGamepadPreviewInput();
	OnBackActionHandled.RemoveDynamic(this, &UMD_GalleryWidget::HandleGalleryBackAction);

	if (bClearPreviewOnDestruct)
	{
		ClearPreview();
	}

	Super::NativeDestruct();
}

void UMD_GalleryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsInspectingPreview)
	{
		return;
	}

	if (!IsValid(PreviewRigRef))
	{
		FindPreviewRig();
	}

	if (!IsValid(PreviewRigRef))
	{
		return;
	}

	const FVector2D RotationInput = ApplyStickDeadZone(RightStickInput);
	if (!RotationInput.IsNearlyZero())
	{
		PreviewRigRef->RotatePreview(RotationInput.X * GamepadRotationInputSpeed * InDeltaTime, -RotationInput.Y * GamepadRotationInputSpeed * InDeltaTime);
	}

	const FVector2D KeyboardPanInput = GetKeyboardPanInput();
	const float ZoomPanScale = FMath::Lerp(GamepadPanZoomedInScale, 1.0f, PreviewRigRef->GetNormalizedZoomDistance());
	const FVector2D GamepadPanInput = ApplyStickDeadZone(LeftStickInput) * GamepadPanSensitivity * ZoomPanScale;
	const FVector2D PanInput = (KeyboardPanInput + GamepadPanInput).GetClampedToMaxSize(1.0f);
	if (!PanInput.IsNearlyZero())
	{
		PreviewRigRef->PanPreview(PanInput.X, PanInput.Y, InDeltaTime);
	}

	const float ZoomInput = RightTriggerInput - LeftTriggerInput;
	if (!FMath::IsNearlyZero(ZoomInput))
	{
		PreviewRigRef->ZoomPreview(ZoomInput * GamepadZoomInputSpeed * InDeltaTime);
	}
}

void UMD_GalleryWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	ClearKeyboardPanInput();
	ClearGamepadPreviewInput();
	Super::NativeOnFocusLost(InFocusEvent);
}

void UMD_GalleryWidget::FindPreviewRig()
{
	if (!GetWorld())
	{
		return;
	}

	PreviewRigRef = Cast<AMD_MenuPreviewRig>(UGameplayStatics::GetActorOfClass(GetWorld(), AMD_MenuPreviewRig::StaticClass()));
}

void UMD_GalleryWidget::ShowPreviewItem(TSubclassOf<AActor> PreviewClass)
{
	if (!IsValid(PreviewRigRef))
	{
		FindPreviewRig();
	}

	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->ShowPreview(PreviewClass);
		if (PreviewRigRef->HasPreview())
		{
			EnterPreviewInspectMode();
		}
	}
}

void UMD_GalleryWidget::ClearPreview()
{
	if (bIsInspectingPreview)
	{
		ReturnToGallerySelection();
	}

	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->ClearPreview();
	}
}

bool UMD_GalleryWidget::ReturnToGallerySelection()
{
	if (!bIsInspectingPreview)
	{
		return false;
	}

	bIsInspectingPreview = false;
	bDraggingPreview = false;
	ClearKeyboardPanInput();
	ClearGamepadPreviewInput();
	GalleryNavigationState.Reset();
	RefreshActionHints();
	OnInspectModeChanged.Broadcast(false);

	if (ReturnFromModalZone())
	{
		return true;
	}

	return SetNavigationFocusByIndex(GetActiveNavigationIndex());
}

FReply UMD_GalleryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsPointerOverPreviewArea(InMouseEvent))
	{
		if (IsValid(PreviewRigRef) && PreviewRigRef->HasPreview())
		{
			EnterPreviewInspectMode();
		}

		bDraggingPreview = true;
		LastMouseScreenPosition = InMouseEvent.GetScreenSpacePosition();
		SetKeyboardFocus();

		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMD_GalleryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingPreview)
	{
		bDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMD_GalleryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDraggingPreview)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	if (!IsValid(PreviewRigRef))
	{
		FindPreviewRig();
	}

	const FVector2D CurrentMousePosition = InMouseEvent.GetScreenSpacePosition();
	const FVector2D MouseDelta = CurrentMousePosition - LastMouseScreenPosition;
	LastMouseScreenPosition = CurrentMousePosition;

	if (IsValid(PreviewRigRef))
	{
		PreviewRigRef->RotatePreview(MouseDelta.X, MouseDelta.Y);
	}

	return FReply::Handled();
}

FReply UMD_GalleryWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsPointerOverPreviewArea(InMouseEvent))
	{
		if (!IsValid(PreviewRigRef))
		{
			FindPreviewRig();
		}

		if (IsValid(PreviewRigRef))
		{
			PreviewRigRef->ZoomPreview(InMouseEvent.GetWheelDelta());
		}

		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UMD_GalleryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Handle pointer Escape before keyboard-navigation mode takes over.
	if (InKeyEvent.GetKey() == EKeys::Escape && IsPointerInputActive())
	{
		const EGameUIFocusZone SourceZone = GetCurrentFocusZone();
		if (HandleRootBackAction())
		{
			BroadcastBackActionHandled(SourceZone);
			return FReply::Handled();
		}
	}

	if (bIsInspectingPreview)
	{
		if (IsBackAction(InKeyEvent))
		{
			if (BackActionHint)
			{
				BackActionHint->PlayHandledFeedback();
			}
			ReturnToGallerySelection();
			return FReply::Handled();
		}

		if ((InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Top || InKeyEvent.GetKey() == EKeys::R) && IsValid(PreviewRigRef))
		{
			PreviewRigRef->ResetPreviewView();
			if (ResetActionHint)
			{
				ResetActionHint->PlayHandledFeedback();
			}
			return FReply::Handled();
		}

		const FReply PanReply = HandleKeyboardPanKeyDown(InKeyEvent);
		if (PanReply.IsEventHandled())
		{
			return PanReply;
		}
	}
	else
	{
		const FIntPoint Direction = GetDigitalNavigationDirection(InKeyEvent);
		if (Direction != FIntPoint::ZeroValue)
		{
			if (InKeyEvent.GetKey().IsGamepadKey())
			{
				NotifyGamepadInput();
			}
			else
			{
				NotifyNavigationInput();
			}

			if (CanProcessGalleryDigitalMove(InKeyEvent.IsRepeat()))
			{
				MoveGalleryFocus2D(Direction);
			}
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UMD_GalleryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bIsInspectingPreview)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	const FReply Reply = HandleKeyboardPanKeyDown(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UMD_GalleryWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bIsInspectingPreview)
	{
		return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
	}

	const FReply Reply = HandleKeyboardPanKeyUp(InKeyEvent);
	return Reply.IsEventHandled() ? Reply : Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply UMD_GalleryWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (bIsInspectingPreview && UpdatePreviewAnalogAxis(InAnalogEvent.GetKey(), InAnalogEvent.GetAnalogValue()))
	{
		NotifyGamepadInput(InAnalogEvent.GetAnalogValue());
		return FReply::Handled();
	}

	const FGameUIAnalogNavigationResult Result = GalleryNavigationState.ProcessAxis(
		InAnalogEvent.GetKey(), InAnalogEvent.GetAnalogValue(), FPlatformTime::Seconds(), AnalogNavigationConfig, EGameUIAnalogNavigationMode::TwoDimensional);
	if (!Result.bHandled)
	{
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}

	NotifyGamepadInput(InAnalogEvent.GetAnalogValue());
	if (Result.bShouldNavigate && MoveGalleryFocus2D(Result.Direction))
	{
		GalleryNavigationState.NotifyNavigationSucceeded();
	}
	else if (Result.bShouldNavigate)
	{
		GalleryNavigationState.NotifyNavigationBlocked();
	}
	return FReply::Handled();
}

bool UMD_GalleryWidget::HandleNavigationWidgetAnalogInput(UWidget* NavigationWidget, FKey Key, float Value)
{
	if (bIsInspectingPreview)
	{
		return UpdatePreviewAnalogAxis(Key, Value);
	}

	const FGameUIAnalogNavigationResult Result = GalleryNavigationState.ProcessAxis(Key, Value, FPlatformTime::Seconds(), AnalogNavigationConfig, EGameUIAnalogNavigationMode::TwoDimensional);
	if (!Result.bHandled)
	{
		return Super::HandleNavigationWidgetAnalogInput(NavigationWidget, Key, Value);
	}

	if (Result.bShouldNavigate && MoveGalleryFocus2D(Result.Direction))
	{
		GalleryNavigationState.NotifyNavigationSucceeded();
	}
	else if (Result.bShouldNavigate)
	{
		GalleryNavigationState.NotifyNavigationBlocked();
	}
	return true;
}

bool UMD_GalleryWidget::HandleNavigationWidgetDigitalInput(UWidget* NavigationWidget, FIntPoint Direction, bool bIsRepeat)
{
	if (bIsInspectingPreview)
	{
		return false;
	}

	if (CanProcessGalleryDigitalMove(bIsRepeat))
	{
		MoveGalleryFocus2D(Direction);
	}
	return true;
}

bool UMD_GalleryWidget::HandleRootBackAction_Implementation()
{
	if (Super::HandleRootBackAction_Implementation())
	{
		return true;
	}

	// The legacy Back button still owns the Blueprint layer pop.
	if (IsValid(GalleryBackButton))
	{
		GalleryBackButton->ActivateFocusItem();
		return true;
	}

	return false;
}

void UMD_GalleryWidget::RegisterGalleryFocusItems()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget(
		[this](UWidget* Widget)
		{
			if (!GalleryScrollBox)
			{
				GalleryScrollBox = Cast<UScrollBox>(Widget);
			}
			if (!GalleryItemsContainer)
			{
				GalleryItemsContainer = Cast<UWrapBox>(Widget);
			}

			UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget);
			if (FocusItem && FocusItem->GetFName() == TEXT("BaseButton_Back"))
			{
				GalleryBackButton = FocusItem;
			}
		});

	TArray<UWidget*> GalleryItems;
	if (GalleryItemsContainer)
	{
		for (int32 ChildIndex = 0; ChildIndex < GalleryItemsContainer->GetChildrenCount(); ++ChildIndex)
		{
			if (UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(GalleryItemsContainer->GetChildAt(ChildIndex)))
			{
				GalleryItems.Add(FocusItem);
			}
		}
	}
	else
	{
		WidgetTree->ForEachWidget(
			[this, &GalleryItems](UWidget* Widget)
			{
				UGameUIFocusItemWidgetBase* FocusItem = Cast<UGameUIFocusItemWidgetBase>(Widget);
				if (FocusItem && FocusItem != GalleryBackButton)
				{
					GalleryItems.Add(FocusItem);
				}
			});
	}

	SetNavigationWidgets(GalleryItems);
}

void UMD_GalleryWidget::RefreshActionHints()
{
	if (SelectActionHint)
	{
		SelectActionHint->ConfigureHint(SelectHintText.KeyboardKeyText, SelectHintText.GamepadKeyText, SelectHintText.ActionText);
		SelectActionHint->SetVisibility(bIsInspectingPreview ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (RotateActionHint)
	{
		RotateActionHint->ConfigureHint(RotateHintText.KeyboardKeyText, RotateHintText.GamepadKeyText, RotateHintText.ActionText);
		RotateActionHint->SetVisibility(bIsInspectingPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (MoveActionHint)
	{
		MoveActionHint->ConfigureHint(MoveHintText.KeyboardKeyText, MoveHintText.GamepadKeyText, MoveHintText.ActionText);
		MoveActionHint->SetVisibility(bIsInspectingPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ZoomActionHint)
	{
		ZoomActionHint->ConfigureHint(ZoomHintText.KeyboardKeyText, ZoomHintText.GamepadKeyText, ZoomHintText.ActionText);
		ZoomActionHint->SetVisibility(bIsInspectingPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ResetActionHint)
	{
		ResetActionHint->ConfigureHint(ResetHintText.KeyboardKeyText, ResetHintText.GamepadKeyText, ResetHintText.ActionText);
		ResetActionHint->SetVisibility(bIsInspectingPreview ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (BackActionHint)
	{
		const FMDGalleryActionHintDefinition& BackHintText = bIsInspectingPreview ? InspectBackHintText : GalleryBackHintText;
		BackActionHint->ConfigureHint(BackHintText.KeyboardKeyText, BackHintText.GamepadKeyText, BackHintText.ActionText);
		BackActionHint->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UMD_GalleryWidget::HandleGalleryBackAction(const EGameUIFocusZone SourceZone)
{
	if (BackActionHint)
	{
		BackActionHint->PlayHandledFeedback();
	}
}

void UMD_GalleryWidget::EnterPreviewInspectMode()
{
	if (bIsInspectingPreview || !IsValid(PreviewRigRef) || !PreviewRigRef->HasPreview())
	{
		return;
	}

	bIsInspectingPreview = true;
	ClearGamepadPreviewInput();
	GalleryNavigationState.Reset();
	EnterModalZone(this);
	RefreshActionHints();
	OnInspectModeChanged.Broadcast(true);
}

void UMD_GalleryWidget::ClearGamepadPreviewInput()
{
	LeftStickInput = FVector2D::ZeroVector;
	RightStickInput = FVector2D::ZeroVector;
	LeftTriggerInput = 0.0f;
	RightTriggerInput = 0.0f;
}

bool UMD_GalleryWidget::MoveGalleryFocus2D(FIntPoint Direction)
{
	Direction.X = FMath::Clamp(Direction.X, -1, 1);
	Direction.Y = FMath::Clamp(Direction.Y, -1, 1);
	if (Direction == FIntPoint::ZeroValue || !NavigationEntries.IsValidIndex(GetActiveNavigationIndex()))
	{
		return false;
	}

	const int32 ColumnCount = FMath::Max(GalleryColumnCount, 1);
	const int32 CurrentIndex = GetActiveNavigationIndex();
	const int32 CurrentRow = CurrentIndex / ColumnCount;
	const int32 CurrentColumn = CurrentIndex % ColumnCount;
	int32 TargetIndex = INDEX_NONE;

	if (Direction.X != 0)
	{
		const int32 TargetColumn = CurrentColumn + Direction.X;
		if (TargetColumn >= 0 && TargetColumn < ColumnCount)
		{
			const int32 CandidateIndex = CurrentRow * ColumnCount + TargetColumn;
			if (NavigationEntries.IsValidIndex(CandidateIndex))
			{
				TargetIndex = CandidateIndex;
			}
		}
	}
	else
	{
		const int32 TargetRow = CurrentRow + Direction.Y;
		const int32 TargetRowStart = TargetRow * ColumnCount;
		if (TargetRow >= 0 && NavigationEntries.IsValidIndex(TargetRowStart))
		{
			const int32 TargetRowEnd = FMath::Min(TargetRowStart + ColumnCount - 1, NavigationEntries.Num() - 1);
			TargetIndex = FMath::Min(TargetRowStart + CurrentColumn, TargetRowEnd);
		}
	}

	if (!NavigationEntries.IsValidIndex(TargetIndex))
	{
		return false;
	}

	UWidget* TargetWidget = NavigationEntries[TargetIndex].NavigationWidget;
	if (!IsValid(TargetWidget) || !TargetWidget->GetIsEnabled())
	{
		return false;
	}

	const ESlateVisibility TargetVisibility = TargetWidget->GetVisibility();
	if (TargetVisibility == ESlateVisibility::Collapsed || TargetVisibility == ESlateVisibility::Hidden)
	{
		return false;
	}

	if (!SetNavigationFocusByIndex(TargetIndex))
	{
		return false;
	}

	if (GalleryScrollBox)
	{
		GalleryScrollBox->ScrollWidgetIntoView(TargetWidget, false, EDescendantScrollDestination::IntoView, 12.0f);
	}
	return true;
}

bool UMD_GalleryWidget::CanProcessGalleryDigitalMove(const bool bIsRepeat)
{
	const double CurrentTime = FPlatformTime::Seconds();
	if (bIsRepeat && CurrentTime - LastGalleryDigitalMoveTime < GalleryDigitalRepeatInterval)
	{
		return false;
	}

	LastGalleryDigitalMoveTime = CurrentTime;
	return true;
}

FVector2D UMD_GalleryWidget::ApplyStickDeadZone(FVector2D Input) const
{
	const float Magnitude = Input.Size();
	if (Magnitude <= InspectStickDeadZone)
	{
		return FVector2D::ZeroVector;
	}

	const float NormalizedMagnitude = FMath::GetMappedRangeValueClamped(FVector2D(InspectStickDeadZone, 1.0f), FVector2D(0.0f, 1.0f), Magnitude);
	return Input.GetSafeNormal() * NormalizedMagnitude;
}

bool UMD_GalleryWidget::UpdatePreviewAnalogAxis(FKey Key, float Value)
{
	if (Key == EKeys::Gamepad_LeftX)
	{
		LeftStickInput.X = Value;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		LeftStickInput.Y = Value;
	}
	else if (Key == EKeys::Gamepad_RightX)
	{
		RightStickInput.X = Value;
	}
	else if (Key == EKeys::Gamepad_RightY)
	{
		RightStickInput.Y = Value;
	}
	else if (Key == EKeys::Gamepad_LeftTriggerAxis)
	{
		LeftTriggerInput = FMath::Clamp(Value, 0.0f, 1.0f);
	}
	else if (Key == EKeys::Gamepad_RightTriggerAxis)
	{
		RightTriggerInput = FMath::Clamp(Value, 0.0f, 1.0f);
	}
	else
	{
		return false;
	}
	return true;
}

bool UMD_GalleryWidget::IsBackAction(const FKeyEvent& KeyEvent)
{
	return KeyEvent.GetKey() == EKeys::Escape || KeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right;
}

FIntPoint UMD_GalleryWidget::GetDigitalNavigationDirection(const FKeyEvent& KeyEvent)
{
	if (!FSlateApplication::IsInitialized())
	{
		return FIntPoint::ZeroValue;
	}

	switch (FSlateApplication::Get().GetNavigationDirectionFromKey(KeyEvent))
	{
	case EUINavigation::Left:
		return FIntPoint(-1, 0);
	case EUINavigation::Right:
		return FIntPoint(1, 0);
	case EUINavigation::Up:
		return FIntPoint(0, -1);
	case EUINavigation::Down:
		return FIntPoint(0, 1);
	default:
		return FIntPoint::ZeroValue;
	}
}

bool UMD_GalleryWidget::IsPointerOverPreviewArea(const FPointerEvent& MouseEvent) const
{
	if (!PreviewInputArea)
	{
		return false;
	}

	const FGeometry AreaGeometry = PreviewInputArea->GetCachedGeometry();
	const FVector2D LocalPosition = AreaGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D LocalSize = AreaGeometry.GetLocalSize();

	return LocalPosition.X >= 0.0f && LocalPosition.Y >= 0.0f && LocalPosition.X <= LocalSize.X && LocalPosition.Y <= LocalSize.Y;
}

FReply UMD_GalleryWidget::HandleKeyboardPanKeyDown(const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();

	if (Key == EKeys::A)
	{
		bPanLeftHeld = true;
	}
	else if (Key == EKeys::D)
	{
		bPanRightHeld = true;
	}
	else if (Key == EKeys::W)
	{
		bPanUpHeld = true;
	}
	else if (Key == EKeys::S)
	{
		bPanDownHeld = true;
	}
	else
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

FReply UMD_GalleryWidget::HandleKeyboardPanKeyUp(const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();

	if (Key == EKeys::A)
	{
		bPanLeftHeld = false;
	}
	else if (Key == EKeys::D)
	{
		bPanRightHeld = false;
	}
	else if (Key == EKeys::W)
	{
		bPanUpHeld = false;
	}
	else if (Key == EKeys::S)
	{
		bPanDownHeld = false;
	}
	else
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

void UMD_GalleryWidget::ClearKeyboardPanInput()
{
	bPanUpHeld = false;
	bPanDownHeld = false;
	bPanLeftHeld = false;
	bPanRightHeld = false;
}

FVector2D UMD_GalleryWidget::GetKeyboardPanInput() const
{
	const float HorizontalDirection = (bPanRightHeld ? 1.0f : 0.0f) - (bPanLeftHeld ? 1.0f : 0.0f);
	const float VerticalDirection = (bPanUpHeld ? 1.0f : 0.0f) - (bPanDownHeld ? 1.0f : 0.0f);

	return FVector2D(HorizontalDirection, VerticalDirection).GetSafeNormal();
}
