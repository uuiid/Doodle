#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

namespace DoodleLightingBridge
{
	DOODLELIGHTINGEDITOR_API FString GetBridgeDir();

	DOODLELIGHTINGEDITOR_API FString MakeLaunchPlanPath(const FString& InSessionId, int32 InIteration);

	DOODLELIGHTINGEDITOR_API bool WriteLaunchPlan(const FDoodleLaunchPlan& InPlan, FString& OutPath, FString& OutError);

	DOODLELIGHTINGEDITOR_API bool ReadLaunchPlan(const FString& InPath, FDoodleLaunchPlan& OutPlan, FString& OutError);

	DOODLELIGHTINGEDITOR_API bool DeleteLaunchPlan(const FString& InPath, FString& OutError);
}
