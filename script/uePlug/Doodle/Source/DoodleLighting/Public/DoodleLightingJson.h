#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

namespace DoodleLightingJson
{
	DOODLELIGHTING_API bool ParseTaskPayload(const TSharedPtr<FJsonObject>& InRoot, FDoodleTaskPayload& OutPayload, FString& OutError);

	DOODLELIGHTING_API bool ParseLightingRig(const TSharedPtr<FJsonObject>& InRoot, FDoodleLightingRig& OutRig, FString& OutError);

	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeTaskPayload(const FDoodleTaskPayload& InPayload);

	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeLaunchPlan(const FDoodleLaunchPlan& InPlan);

	DOODLELIGHTING_API bool ParseLaunchPlan(const TSharedPtr<FJsonObject>& InRoot, FDoodleLaunchPlan& OutPlan, FString& OutError);

	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeRoundReport(const FDoodleRoundReport& InReport);

	DOODLELIGHTING_API bool ParseRoundReport(const TSharedPtr<FJsonObject>& InRoot, FDoodleRoundReport& OutReport, FString& OutError);
}
