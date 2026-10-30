#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "DoodleWebBridge.generated.h"

UCLASS()
class DOODLELIGHTINGEDITOR_API UDoodleWebBridge : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetCookies(const FString& InCookies);

	void SetCookiesFromNative(const FString& InCookies);
	static FString GetCookies();

	static UDoodleWebBridge* Get();
};
