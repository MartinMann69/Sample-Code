#pragma once

#include "CoreMinimal.h"
#include "UI/MD_MenuLayerScreenWidget.h"
#include "MD_GalleryWidget.generated.h"

class UWidget;
class UScrollBox;
class UWrapBox;
class AMD_MenuPreviewRig;
class UMD_ActionHintWidget;
class UGameUIFocusItemWidgetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMDGalleryInspectModeChanged, bool, bIsInspecting);

USTRUCT(BlueprintType)
struct FMDGalleryActionHintDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Gallery|Hints")
	FText KeyboardKeyText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Gallery|Hints")
	FText GamepadKeyText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Gallery|Hints")
	FText ActionText;
};

UCLASS()
class MORBUSDEI_API UMD_GalleryWidget : public UMD_MenuLayerScreenWidget
{
	GENERATED_BODY()
public:
	UMD_GalleryWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "MD|Gallery")
	void ShowPreviewItem(TSubclassOf<AActor> PreviewClass);

	UFUNCTION(BlueprintCallable, Category = "MD|Gallery")
	void ClearPreview();

	UFUNCTION(BlueprintCallable, Category = "MD|Gallery|Input")
	bool ReturnToGallerySelection();

	UFUNCTION(BlueprintPure, Category = "MD|Gallery|Input")
	bool IsInspectingPreview() const { return bIsInspectingPreview; }

	UPROPERTY(BlueprintAssignable, Category = "MD|Gallery|Input")
	FMDGalleryInspectModeChanged OnInspectModeChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual bool HandleNavigationWidgetAnalogInput(UWidget* NavigationWidget, FKey Key, float Value) override;
	virtual bool HandleNavigationWidgetDigitalInput(UWidget* NavigationWidget, FIntPoint Direction, bool bIsRepeat) override;
	virtual bool HandleRootBackAction_Implementation() override;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly)
	TObjectPtr<UWidget> PreviewInputArea;

	/** Optional named binding; otherwise the first ScrollBox is used. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery")
	TObjectPtr<UScrollBox> GalleryScrollBox;

	/** Optional named binding; otherwise the first WrapBox is used. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery")
	TObjectPtr<UWrapBox> GalleryItemsContainer;

	/** Optional WBP_ActionHint bindings. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery|Hints")
	TObjectPtr<UMD_ActionHintWidget> SelectActionHint;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery|Hints")
	TObjectPtr<UMD_ActionHintWidget> RotateActionHint;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery|Hints")
	TObjectPtr<UMD_ActionHintWidget> MoveActionHint;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery|Hints")
	TObjectPtr<UMD_ActionHintWidget> ZoomActionHint;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery|Hints")
	TObjectPtr<UMD_ActionHintWidget> ResetActionHint;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "MD|Gallery|Hints")
	TObjectPtr<UMD_ActionHintWidget> BackActionHint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition SelectHintText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition RotateHintText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition MoveHintText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition ZoomHintText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition ResetHintText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition GalleryBackHintText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Hints", meta = (ShowOnlyInnerProperties))
	FMDGalleryActionHintDefinition InspectBackHintText;

	UPROPERTY(BlueprintReadOnly, Category = "MD|Gallery")
	TObjectPtr<AMD_MenuPreviewRig> PreviewRigRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Gallery")
	bool bClearPreviewOnDestruct = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InspectStickDeadZone = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Input", meta = (ClampMin = "0.0"))
	float GamepadRotationInputSpeed = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Input", meta = (ClampMin = "0.0"))
	float GamepadZoomInputSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Input", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float GamepadPanSensitivity = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Input", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float GamepadPanZoomedInScale = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Navigation", meta = (ClampMin = "1"))
	int32 GalleryColumnCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MD|Gallery|Navigation", meta = (ClampMin = "0.05"))
	float GalleryDigitalRepeatInterval = 0.14f;

private:
	bool bDraggingPreview = false;
	bool bIsInspectingPreview = false;
	bool bPanUpHeld = false;
	bool bPanDownHeld = false;
	bool bPanLeftHeld = false;
	bool bPanRightHeld = false;
	FVector2D LastMouseScreenPosition = FVector2D::ZeroVector;
	FVector2D LeftStickInput = FVector2D::ZeroVector;
	FVector2D RightStickInput = FVector2D::ZeroVector;
	float LeftTriggerInput = 0.0f;
	float RightTriggerInput = 0.0f;
	FGameUIAnalogNavigationState GalleryNavigationState;
	double LastGalleryDigitalMoveTime = -1000.0;

	UPROPERTY(Transient)
	TObjectPtr<UGameUIFocusItemWidgetBase> GalleryBackButton;

	void FindPreviewRig();
	void RegisterGalleryFocusItems();
	void RefreshActionHints();

	UFUNCTION()
	void HandleGalleryBackAction(EGameUIFocusZone SourceZone);

	void EnterPreviewInspectMode();
	void ClearGamepadPreviewInput();
	bool MoveGalleryFocus2D(FIntPoint Direction);
	bool CanProcessGalleryDigitalMove(bool bIsRepeat);
	FVector2D ApplyStickDeadZone(FVector2D Input) const;
	bool UpdatePreviewAnalogAxis(FKey Key, float Value);
	static bool IsBackAction(const FKeyEvent& KeyEvent);
	static FIntPoint GetDigitalNavigationDirection(const FKeyEvent& KeyEvent);
	bool IsPointerOverPreviewArea(const FPointerEvent& MouseEvent) const;
	FReply HandleKeyboardPanKeyDown(const FKeyEvent& KeyEvent);
	FReply HandleKeyboardPanKeyUp(const FKeyEvent& KeyEvent);
	void ClearKeyboardPanInput();
	FVector2D GetKeyboardPanInput() const;
};
