#include "Interaction/MD_HighlightComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"

UMD_HighlightComponent::UMD_HighlightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMD_HighlightComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupHighlightStencil();
}

void UMD_HighlightComponent::SetHighlightRoot(USceneComponent* NewHighlightRoot)
{
	HighlightRoot = NewHighlightRoot;
}

void UMD_HighlightComponent::SetupHighlightStencil()
{
	if (!HighlightRoot)
	{
		return;
	}

	TArray<USceneComponent*> ChildComponents;
	HighlightRoot->GetChildrenComponents(true, ChildComponents);

	for (USceneComponent* Child : ChildComponents)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Child))
		{
			Primitive->SetCustomDepthStencilValue(CustomDepthStencilValue);
		}
	}
}

void UMD_HighlightComponent::SetHighlighted(bool bHighlight)
{
	if (!bCanHighlight || !HighlightRoot)
	{
		return;
	}

	TArray<USceneComponent*> ChildComponents;
	HighlightRoot->GetChildrenComponents(true, ChildComponents);

	for (USceneComponent* Child : ChildComponents)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Child))
		{
			Primitive->SetRenderCustomDepth(bHighlight);
		}
	}
}