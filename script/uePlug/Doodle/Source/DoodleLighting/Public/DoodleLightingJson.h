#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

namespace DoodleLightingJson
{
	DOODLELIGHTING_API bool ParseLightingRig(const TSharedPtr<FJsonObject>& InRoot, FDoodleLightingRig& OutRig, FString& OutError);

	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeLightingRig(const FDoodleLightingRig& InRig);

	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeRenderPayload(const FDoodleLaunchPlan& InPlan);

	DOODLELIGHTING_API bool ParseRenderPayload(const TSharedPtr<FJsonObject>& InRoot, FDoodleLaunchPlan& OutPlan, FString& OutError);
}
