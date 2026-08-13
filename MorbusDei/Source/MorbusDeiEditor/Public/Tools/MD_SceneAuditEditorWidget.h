#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "EditorUtilityWidget.h"
#include "MD_SceneAuditEditorWidget.generated.h"

class AActor;
class UButton;
class UCheckBox;
class UMeshComponent;
class UMaterialInterface;
class UTextBlock;

UENUM(BlueprintType)
enum class EMD_MaterialAuditIssueType : uint8
{
	MissingMaterial UMETA(DisplayName="Missing Material"),
	DefaultMaterial UMETA(DisplayName="UE Default Material")
};

USTRUCT(BlueprintType)
struct MORBUSDEIEDITOR_API FMD_MaterialAuditIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	TObjectPtr<UMeshComponent> MeshComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	int32 MaterialIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	FName MaterialSlotName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	EMD_MaterialAuditIssueType IssueType = EMD_MaterialAuditIssueType::MissingMaterial;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	FText ActorLabel;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	FText ComponentName;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	FText IssueText;
};

UCLASS(BlueprintType)
class MORBUSDEIEDITOR_API UMD_SceneAuditIssueObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	FMD_MaterialAuditIssue Issue;

	UFUNCTION(BlueprintCallable, Category="MD|Material Audit")
	void SelectActor() const;
};

UCLASS(BlueprintType, Blueprintable)
class MORBUSDEIEDITOR_API UMD_SceneAuditIssueRowWidget
	: public UUserWidget
	, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildNativeWidgetTree();
	void UpdateFromIssue();

	UFUNCTION()
	void HandleSelectClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMD_SceneAuditIssueObject> IssueItem;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActorLabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ComponentText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MaterialSlotText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> IssueText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SelectButton;
};

UCLASS(NotBlueprintable)
class MORBUSDEIEDITOR_API UMD_SceneAuditListView : public UListView
{
	GENERATED_BODY()

public:
	UMD_SceneAuditListView(const FObjectInitializer& ObjectInitializer);
};

UCLASS(BlueprintType, Blueprintable)
class MORBUSDEIEDITOR_API UMD_SceneAuditEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UMD_SceneAuditEditorWidget();

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	TArray<FMD_MaterialAuditIssue> AuditIssues;

	UPROPERTY(BlueprintReadOnly, Category="MD|Material Audit")
	TArray<TObjectPtr<UMD_SceneAuditIssueObject>> AuditIssueItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Material Audit")
	bool bIncludeHiddenActors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MD|Material Audit")
	TArray<FString> DefaultMaterialAssetPaths;

	UFUNCTION(BlueprintCallable, Category="MD|Material Audit")
	TArray<FMD_MaterialAuditIssue> RefreshMaterialAudit();

	UFUNCTION(BlueprintCallable, Category="MD|Material Audit")
	TArray<UObject*> RefreshMaterialAuditListItems();

	UFUNCTION(BlueprintCallable, Category="MD|Material Audit")
	void RefreshMaterialAuditListView(UListView* ListView);

	UFUNCTION(BlueprintCallable, Category="MD|Material Audit")
	void SelectAuditResult(const FMD_MaterialAuditIssue& Issue) const;

	UFUNCTION(BlueprintCallable, Category="MD|Material Audit")
	void SelectAuditIssueObject(const UMD_SceneAuditIssueObject* IssueObject) const;

	UFUNCTION(BlueprintPure, Category="MD|Material Audit")
	int32 GetIssueCount() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	bool IsDefaultMaterial(UMaterialInterface* Material) const;
	void AddIssue(AActor* Actor, UMeshComponent* MeshComponent, int32 MaterialIndex, FName MaterialSlotName, EMD_MaterialAuditIssueType IssueType);

private:
	void BuildNativeWidgetTree();
	void UpdateIssueSummary();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleSelectAllClicked();

	UFUNCTION()
	void HandleIncludeHiddenChanged(bool bIsChecked);

	UPROPERTY(Transient)
	TObjectPtr<UMD_SceneAuditListView> AuditListView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> IssueSummaryText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SelectAllButton;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> IncludeHiddenCheckBox;
};
