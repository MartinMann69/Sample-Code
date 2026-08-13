#include "Tools/MD_SceneAuditEditorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/MeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateTypes.h"
#include "UObject/SoftObjectPath.h"

#define LOCTEXT_NAMESPACE "MDSceneAuditEditorWidget"

namespace MDSceneAuditWidget
{
	constexpr float RowPadding = 4.0f;

	FSlateChildSize FillSize(float Weight = 1.0f)
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Weight;
		return Size;
	}

	UTextBlock* CreateText(UWidgetTree* Tree, const FName Name, const FText& Text, const int32 FontSize = 10)
	{
		UTextBlock* TextBlock = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(Text);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.88f, 0.88f)));

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		return TextBlock;
	}

	UButton* CreateButton(UWidgetTree* Tree, const FName ButtonName, const FName TextName, const FText& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		Button->SetBackgroundColor(FLinearColor(0.08f, 0.32f, 0.55f));

		UTextBlock* ButtonText = CreateText(Tree, TextName, Label, 10);
		ButtonText->SetJustification(ETextJustify::Center);
		Button->AddChild(ButtonText);
		return Button;
	}

	void ConfigureColumn(UHorizontalBoxSlot* Slot, const float Weight)
	{
		Slot->SetSize(FillSize(Weight));
		Slot->SetPadding(FMargin(RowPadding, 2.0f));
		Slot->SetVerticalAlignment(VAlign_Center);
	}
}

UMD_SceneAuditEditorWidget::UMD_SceneAuditEditorWidget()
{
	DefaultMaterialAssetPaths =
	{
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"),
		TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")
	};
}

UMD_SceneAuditListView::UMD_SceneAuditListView(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static const FSoftClassPath EntryWidgetClassPath
	(
		TEXT("/Game/MorbusDei/Blueprints/WBP_SceneAuditIssueRow.WBP_SceneAuditIssueRow_C")
	);

	if (UClass* LoadedEntryWidgetClass = EntryWidgetClassPath.TryLoadClass<UUserWidget>())
	{
		EntryWidgetClass = LoadedEntryWidgetClass;
	}
	else
	{
		EntryWidgetClass = UMD_SceneAuditIssueRowWidget::StaticClass();
	}
}

TSharedRef<SWidget> UMD_SceneAuditIssueRowWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void UMD_SceneAuditIssueRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSelectClicked);
		SelectButton->OnClicked.AddDynamic(this, &ThisClass::HandleSelectClicked);
	}

	UpdateFromIssue();
}

void UMD_SceneAuditIssueRowWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	IssueItem = Cast<UMD_SceneAuditIssueObject>(ListItemObject);
	UpdateFromIssue();
}

void UMD_SceneAuditIssueRowWidget::BuildNativeWidgetTree()
{
	static const FName RootName(TEXT("MDSceneAuditIssueRowRoot"));
	if (WidgetTree && WidgetTree->RootWidget && WidgetTree->RootWidget->GetFName() == RootName)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), RootName);
	RootBorder->SetPadding(FMargin(3.0f, 2.0f));
	RootBorder->SetBrushColor(FLinearColor(0.025f, 0.025f, 0.03f, 0.55f));
	RootBorder->SetHorizontalAlignment(HAlign_Fill);
	RootBorder->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = RootBorder;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("IssueColumns"));
	RootBorder->AddChild(Row);

	ActorLabelText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("ActorLabel"), FText::GetEmpty());
	MDSceneAuditWidget::ConfigureColumn(Row->AddChildToHorizontalBox(ActorLabelText), 1.35f);

	ComponentText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("Component"), FText::GetEmpty());
	MDSceneAuditWidget::ConfigureColumn(Row->AddChildToHorizontalBox(ComponentText), 1.15f);

	MaterialSlotText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("MaterialSlot"), FText::GetEmpty());
	MDSceneAuditWidget::ConfigureColumn(Row->AddChildToHorizontalBox(MaterialSlotText), 1.0f);

	IssueText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("Issue"), FText::GetEmpty());
	MDSceneAuditWidget::ConfigureColumn(Row->AddChildToHorizontalBox(IssueText), 0.9f);

	SelectButton = MDSceneAuditWidget::CreateButton(WidgetTree, TEXT("SelectActorButton"), TEXT("SelectActorButtonText"), LOCTEXT("SelectActorButton", "Select"));
	UHorizontalBoxSlot* SelectSlot = Row->AddChildToHorizontalBox(SelectButton);
	SelectSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	SelectSlot->SetPadding(FMargin(6.0f, 1.0f));
	SelectSlot->SetVerticalAlignment(VAlign_Center);
}

void UMD_SceneAuditIssueRowWidget::UpdateFromIssue()
{
	const bool bHasValidIssue = IsValid(IssueItem) && IsValid(IssueItem->Issue.Actor);
	if (SelectButton)
	{
		SelectButton->SetIsEnabled(bHasValidIssue);
	}

	if (!IsValid(IssueItem))
	{
		if (ActorLabelText) ActorLabelText->SetText(FText::GetEmpty());
		if (ComponentText) ComponentText->SetText(FText::GetEmpty());
		if (MaterialSlotText) MaterialSlotText->SetText(FText::GetEmpty());
		if (IssueText) IssueText->SetText(FText::GetEmpty());
		return;
	}

	const FMD_MaterialAuditIssue& Issue = IssueItem->Issue;
	if (ActorLabelText) ActorLabelText->SetText(Issue.ActorLabel);
	if (ComponentText) ComponentText->SetText(Issue.ComponentName);
	if (IssueText)
	{
		IssueText->SetText(Issue.IssueText);
		const FLinearColor IssueColor = Issue.IssueType == EMD_MaterialAuditIssueType::MissingMaterial
			? FLinearColor(1.0f, 0.28f, 0.22f)
			: FLinearColor(1.0f, 0.68f, 0.18f);
		IssueText->SetColorAndOpacity(FSlateColor(IssueColor));
	}

	if (MaterialSlotText)
	{
		const FString SlotDescription = Issue.MaterialSlotName.IsNone()
			? FString::Printf(TEXT("Slot %d"), Issue.MaterialIndex)
			: FString::Printf(TEXT("%s (%d)"), *Issue.MaterialSlotName.ToString(), Issue.MaterialIndex);
		MaterialSlotText->SetText(FText::FromString(SlotDescription));
	}
}

void UMD_SceneAuditIssueRowWidget::HandleSelectClicked()
{
	if (IsValid(IssueItem))
	{
		IssueItem->SelectActor();
	}
}

TSharedRef<SWidget> UMD_SceneAuditEditorWidget::RebuildWidget()
{
	BuildNativeWidgetTree();
	return Super::RebuildWidget();
}

void UMD_SceneAuditEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshClicked);
		RefreshButton->OnClicked.AddDynamic(this, &ThisClass::HandleRefreshClicked);
	}

	if (SelectAllButton)
	{
		SelectAllButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSelectAllClicked);
		SelectAllButton->OnClicked.AddDynamic(this, &ThisClass::HandleSelectAllClicked);
	}

	if (IncludeHiddenCheckBox)
	{
		IncludeHiddenCheckBox->OnCheckStateChanged.RemoveDynamic(this, &ThisClass::HandleIncludeHiddenChanged);
		IncludeHiddenCheckBox->OnCheckStateChanged.AddDynamic(this, &ThisClass::HandleIncludeHiddenChanged);
		IncludeHiddenCheckBox->SetIsChecked(bIncludeHiddenActors);
	}

	if (!IsDesignTime())
	{
		RefreshMaterialAuditListView(AuditListView);
	}
}

void UMD_SceneAuditEditorWidget::BuildNativeWidgetTree()
{
	static const FName RootName(TEXT("MDSceneAuditNativeRoot"));
	if (WidgetTree && WidgetTree->RootWidget && WidgetTree->RootWidget->GetFName() == RootName)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), RootName);
	RootBorder->SetPadding(FMargin(12.0f));
	RootBorder->SetBrushColor(FLinearColor(0.018f, 0.018f, 0.023f, 1.0f));
	RootBorder->SetHorizontalAlignment(HAlign_Fill);
	RootBorder->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SceneAuditLayout"));
	RootBorder->AddChild(RootBox);

	auto AddVerticalWidget = [RootBox](UWidget* Widget, const FMargin SlotPadding, const ESlateSizeRule::Type SizeRule)
	{
		UVerticalBoxSlot* Slot = RootBox->AddChildToVerticalBox(Widget);
		Slot->SetPadding(SlotPadding);
		Slot->SetSize(FSlateChildSize(SizeRule));
		Slot->SetHorizontalAlignment(HAlign_Fill);
		return Slot;
	};

	UTextBlock* TitleText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("Title"), LOCTEXT("SceneAuditTitle", "Scene Material Audit"), 20);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.78f, 1.0f)));
	AddVerticalWidget(TitleText, FMargin(2.0f, 2.0f, 2.0f, 4.0f), ESlateSizeRule::Automatic);

	UTextBlock* DescriptionText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("DescriptionText"), LOCTEXT("SceneAuditHelp", "Finds mesh material slots that are empty or still use an Unreal default material."));
	DescriptionText->SetAutoWrapText(true);
	AddVerticalWidget(DescriptionText, FMargin(2.0f, 0.0f, 2.0f, 10.0f), ESlateSizeRule::Automatic);

	UHorizontalBox* Controls = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Controls"));
	AddVerticalWidget(Controls, FMargin(0.0f, 0.0f, 0.0f, 8.0f), ESlateSizeRule::Automatic);

	RefreshButton = MDSceneAuditWidget::CreateButton(WidgetTree, TEXT("RefreshButton"), TEXT("RefreshButtonText"), LOCTEXT("RefreshButton", "Refresh Audit"));
	UHorizontalBoxSlot* RefreshSlot = Controls->AddChildToHorizontalBox(RefreshButton);
	RefreshSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	RefreshSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

	SelectAllButton = MDSceneAuditWidget::CreateButton(WidgetTree, TEXT("SelectAllButton"), TEXT("SelectAllButtonText"), LOCTEXT("SelectAllButton", "Select All Issues"));
	UHorizontalBoxSlot* SelectAllSlot = Controls->AddChildToHorizontalBox(SelectAllButton);
	SelectAllSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	SelectAllSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

	IncludeHiddenCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("IncludeHiddenCheckBox"));
	UHorizontalBoxSlot* CheckBoxSlot = Controls->AddChildToHorizontalBox(IncludeHiddenCheckBox);
	CheckBoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	CheckBoxSlot->SetPadding(FMargin(0.0f, 2.0f, 5.0f, 0.0f));
	CheckBoxSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* IncludeHiddenLabel = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("IncludeHiddenLabel"), LOCTEXT("IncludeHiddenLabel", "Include hidden actors"));
	UHorizontalBoxSlot* HiddenLabelSlot = Controls->AddChildToHorizontalBox(IncludeHiddenLabel);
	HiddenLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	HiddenLabelSlot->SetVerticalAlignment(VAlign_Center);

	IssueSummaryText = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("IssueSummary"), LOCTEXT("NotScanned", "Not scanned yet"));
	IssueSummaryText->SetJustification(ETextJustify::Right);
	UHorizontalBoxSlot* SummarySlot = Controls->AddChildToHorizontalBox(IssueSummaryText);
	SummarySlot->SetSize(MDSceneAuditWidget::FillSize());
	SummarySlot->SetPadding(FMargin(10.0f, 2.0f, 2.0f, 0.0f));
	SummarySlot->SetVerticalAlignment(VAlign_Center);

	UBorder* HeaderBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ColumnHeaderBorder"));
	HeaderBorder->SetBrushColor(FLinearColor(0.06f, 0.06f, 0.075f, 1.0f));
	HeaderBorder->SetPadding(FMargin(3.0f, 3.0f));
	AddVerticalWidget(HeaderBorder, FMargin(0.0f, 0.0f, 0.0f, 2.0f), ESlateSizeRule::Automatic);

	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ColumnHeaders"));
	HeaderBorder->AddChild(HeaderRow);

	auto AddHeader = [this, HeaderRow](const FName Name, const FText& Label, const float Weight)
	{
		UTextBlock* Header = MDSceneAuditWidget::CreateText(WidgetTree, Name, Label, 9);
		Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.75f, 0.88f)));
		MDSceneAuditWidget::ConfigureColumn(HeaderRow->AddChildToHorizontalBox(Header), Weight);
	};

	AddHeader(TEXT("ActorHeader"), LOCTEXT("ActorHeader", "ACTOR"), 1.35f);
	AddHeader(TEXT("ComponentHeader"), LOCTEXT("ComponentHeader", "COMPONENT"), 1.15f);
	AddHeader(TEXT("SlotHeader"), LOCTEXT("SlotHeader", "MATERIAL SLOT"), 1.0f);
	AddHeader(TEXT("IssueHeader"), LOCTEXT("IssueHeader", "ISSUE"), 0.9f);

	UTextBlock* ActionHeader = MDSceneAuditWidget::CreateText(WidgetTree, TEXT("ActionHeader"), LOCTEXT("ActionHeader", "ACTION"), 9);
	UHorizontalBoxSlot* ActionHeaderSlot = HeaderRow->AddChildToHorizontalBox(ActionHeader);
	ActionHeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ActionHeaderSlot->SetPadding(FMargin(10.0f, 2.0f));
	ActionHeaderSlot->SetVerticalAlignment(VAlign_Center);

	AuditListView = WidgetTree->ConstructWidget<UMD_SceneAuditListView>(UMD_SceneAuditListView::StaticClass(), TEXT("AuditListView"));
	AddVerticalWidget(AuditListView, FMargin(0.0f), ESlateSizeRule::Fill);
}

void UMD_SceneAuditIssueObject::SelectActor() const
{
	if (!GEditor || !IsValid(Issue.Actor))
	{
		return;
	}

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(Issue.Actor, true, true, true);
	GEditor->NoteSelectionChange();
}

TArray<FMD_MaterialAuditIssue> UMD_SceneAuditEditorWidget::RefreshMaterialAudit()
{
	AuditIssues.Reset();
	AuditIssueItems.Reset();

	if (!GEditor)
	{
		return AuditIssues;
	}

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld)
	{
		return AuditIssues;
	}

	for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!IsValid(Actor) || Actor->IsTemplate())
		{
			continue;
		}

		if (!bIncludeHiddenActors && Actor->IsHiddenEd())
		{
			continue;
		}

		TArray<UMeshComponent*> MeshComponents;
		Actor->GetComponents<UMeshComponent>(MeshComponents);

		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (!IsValid(MeshComponent))
			{
				continue;
			}

			const TArray<FName> MaterialSlotNames = MeshComponent->GetMaterialSlotNames();
			const int32 MaterialCount = MeshComponent->GetNumMaterials();

			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				const FName MaterialSlotName = MaterialSlotNames.IsValidIndex(MaterialIndex) ? MaterialSlotNames[MaterialIndex] : NAME_None;

				UMaterialInterface* Material = MeshComponent->GetMaterial(MaterialIndex);
				if (!Material)
				{
					AddIssue(Actor, MeshComponent, MaterialIndex, MaterialSlotName, EMD_MaterialAuditIssueType::MissingMaterial);
					continue;
				}

				if (IsDefaultMaterial(Material))
				{
					AddIssue(Actor, MeshComponent, MaterialIndex, MaterialSlotName, EMD_MaterialAuditIssueType::DefaultMaterial);
				}
			}
		}
	}

	return AuditIssues;
}

TArray<UObject*> UMD_SceneAuditEditorWidget::RefreshMaterialAuditListItems()
{
	RefreshMaterialAudit();

	TArray<UObject*> ListItems;
	ListItems.Reserve(AuditIssueItems.Num());

	for (UMD_SceneAuditIssueObject* IssueItem : AuditIssueItems)
	{
		ListItems.Add(IssueItem);
	}

	return ListItems;
}

void UMD_SceneAuditEditorWidget::RefreshMaterialAuditListView(UListView* ListView)
{
	TArray<UObject*> ListItems = RefreshMaterialAuditListItems();
	if (ListView)
	{
		ListView->SetListItems(ListItems);
	}

	UpdateIssueSummary();
}

void UMD_SceneAuditEditorWidget::SelectAuditResult(const FMD_MaterialAuditIssue& Issue) const
{
	if (!GEditor || !IsValid(Issue.Actor))
	{
		return;
	}

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(Issue.Actor, true, true, true);
	GEditor->NoteSelectionChange();
}

void UMD_SceneAuditEditorWidget::SelectAuditIssueObject(const UMD_SceneAuditIssueObject* IssueObject) const
{
	if (!IssueObject)
	{
		return;
	}

	SelectAuditResult(IssueObject->Issue);
}

int32 UMD_SceneAuditEditorWidget::GetIssueCount() const
{
	return AuditIssues.Num();
}

void UMD_SceneAuditEditorWidget::HandleRefreshClicked()
{
	RefreshMaterialAuditListView(AuditListView);
}

void UMD_SceneAuditEditorWidget::HandleSelectAllClicked()
{
	if (!GEditor)
	{
		return;
	}

	GEditor->SelectNone(false, true);

	TSet<AActor*> SelectedActors;
	for (const FMD_MaterialAuditIssue& Issue : AuditIssues)
	{
		AActor* Actor = Issue.Actor.Get();
		if (!IsValid(Actor) || SelectedActors.Contains(Actor))
		{
			continue;
		}

		SelectedActors.Add(Actor);
		GEditor->SelectActor(Actor, true, false, true);
	}

	GEditor->NoteSelectionChange();
}

void UMD_SceneAuditEditorWidget::HandleIncludeHiddenChanged(const bool bIsChecked)
{
	bIncludeHiddenActors = bIsChecked;
	RefreshMaterialAuditListView(AuditListView);
}

void UMD_SceneAuditEditorWidget::UpdateIssueSummary()
{
	TSet<const AActor*> AffectedActors;
	for (const FMD_MaterialAuditIssue& Issue : AuditIssues)
	{
		if (IsValid(Issue.Actor))
		{
			AffectedActors.Add(Issue.Actor.Get());
		}
	}

	if (IssueSummaryText)
	{
		IssueSummaryText->SetText(FText::Format(
			LOCTEXT("IssueSummaryFormat", "{0} issue(s) across {1} actor(s)"),
			FText::AsNumber(AuditIssues.Num()),
			FText::AsNumber(AffectedActors.Num())));
	}

	if (SelectAllButton)
	{
		SelectAllButton->SetIsEnabled(!AffectedActors.IsEmpty());
	}
}

bool UMD_SceneAuditEditorWidget::IsDefaultMaterial(UMaterialInterface* Material) const
{
	if (!Material)
	{
		return false;
	}

	if (Material == UMaterial::GetDefaultMaterial(MD_Surface))
	{
		return true;
	}

	const FString MaterialPath = Material->GetPathName();
	for (const FString& DefaultMaterialAssetPath : DefaultMaterialAssetPaths)
	{
		if (MaterialPath.Equals(DefaultMaterialAssetPath, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void UMD_SceneAuditEditorWidget::AddIssue(AActor* Actor, UMeshComponent* MeshComponent, int32 MaterialIndex, FName MaterialSlotName, EMD_MaterialAuditIssueType IssueType)
{
	if (!IsValid(Actor) || !IsValid(MeshComponent))
	{
		return;
	}

	FMD_MaterialAuditIssue& Issue = AuditIssues.AddDefaulted_GetRef();
	Issue.Actor = Actor;
	Issue.MeshComponent = MeshComponent;
	Issue.MaterialIndex = MaterialIndex;
	Issue.MaterialSlotName = MaterialSlotName;
	Issue.IssueType = IssueType;
	Issue.ActorLabel = FText::FromString(Actor->GetActorLabel());
	Issue.ComponentName = FText::FromString(MeshComponent->GetName());
	Issue.IssueText = IssueType == EMD_MaterialAuditIssueType::MissingMaterial
		? LOCTEXT("MissingMaterialIssue", "Missing material")
		: LOCTEXT("DefaultMaterialIssue", "UE default material");

	UMD_SceneAuditIssueObject* IssueItem = NewObject<UMD_SceneAuditIssueObject>(this);
	IssueItem->Issue = Issue;
	AuditIssueItems.Add(IssueItem);
}

#undef LOCTEXT_NAMESPACE
