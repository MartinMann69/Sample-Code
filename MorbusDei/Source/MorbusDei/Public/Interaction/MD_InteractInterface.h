#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MD_InteractInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMD_InteractInterface : public UInterface
{
	GENERATED_BODY()
};

class MORBUSDEI_API IMD_InteractInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MD|Interaction")
	void Interact(APawn* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MD|Interaction")
	bool CanInteract() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MD|Highlight")
	void Highlight(bool bHighlight);
};
