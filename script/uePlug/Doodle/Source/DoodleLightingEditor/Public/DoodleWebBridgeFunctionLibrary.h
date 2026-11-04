#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DoodleWebBridgeFunctionLibrary.generated.h"

UCLASS()
class DOODLELIGHTINGEDITOR_API UDoodleWebBridgeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Doodle")
	static FString SetAiLightIterationFromFile(const FString& InFilePath);
};
