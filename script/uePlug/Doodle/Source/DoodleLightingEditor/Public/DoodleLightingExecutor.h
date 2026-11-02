#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "Templates/ValueOrError.h"

namespace DoodleLightingExecutor
{
	struct FAnchorCamera
	{
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		float HorizontalFovDeg = 0.0f;
	};

	struct FRoundContext
	{
		FString BaselineLevel;
	};

	struct FCandidateRenderResult
	{
		int32 LightCount = 0;
	};

	struct FRoundRenderResult
	{
	};

	using FAnchorCameraResult = TValueOrError<FAnchorCamera, FString>;
	using FCandidateRenderResultOrError = TValueOrError<FCandidateRenderResult, FString>;
	using FRoundRenderResultOrError = TValueOrError<FRoundRenderResult, FString>;

	FAnchorCameraResult PrepareAnchorCamera(const FRoundContext& InContext);

	FCandidateRenderResultOrError ApplyCandidateToWorld(UWorld& InWorld, const FString& InFramesDir,
	                                                    const FDoodleLightingRig& InRig, const FAnchorCamera& InCamera);
}
