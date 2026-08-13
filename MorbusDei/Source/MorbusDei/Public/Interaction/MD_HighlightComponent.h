#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MD_HighlightComponent.generated.h"

class USceneComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MORBUSDEI_API UMD_HighlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMD_HighlightComponent();

	virtual void BeginPlay() override;

	void SetHighlightRoot(USceneComponent* NewHighlightRoot);
	void SetHighlighted(bool bHighlight);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Highlight")
	USceneComponent* HighlightRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Highlight")
	bool bCanHighlight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MD|Highlight")
	int32 CustomDepthStencilValue = 1;

	void SetupHighlightStencil();
};