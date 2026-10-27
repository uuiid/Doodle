#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

namespace DoodleLightingExecutor
{
	struct FRoundContext
	{
		FString CameraFilePath;
	};

	struct FCandidateRenderResult
	{
		bool bSucceeded = false;
		FString Error;
		int32 LightCount = 0;
	};

	bool PrepareAnchorCamera(const FRoundContext& InContext, FString& OutError);

	FCandidateRenderResult ApplyCandidateToWorld(UWorld& InWorld, const FDoodleTaskCandidate& InCandidate);
}
