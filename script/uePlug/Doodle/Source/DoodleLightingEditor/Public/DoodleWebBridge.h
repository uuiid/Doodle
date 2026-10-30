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
	void Logout();

	static UDoodleWebBridge* Get();
};
