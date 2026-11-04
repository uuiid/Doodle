#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "DoodleWebBridge.generated.h"

UCLASS(BlueprintType)
class DOODLELIGHTINGEDITOR_API UDoodleWebBridge : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Doodle")
	void Logout();

	UFUNCTION(BlueprintCallable, Category = "Doodle")
	FString SetAiLightIteration(const FString& InIteration);

	UFUNCTION(BlueprintCallable, Category = "Doodle")
	FString GetAiLightSceneContext();

	UFUNCTION(BlueprintPure, Category = "Doodle")
	static UDoodleWebBridge* Get();
};
