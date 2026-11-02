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
		FString LevelSequencePath;
	};

	struct FCandidateRenderResult
	{
		int32 LightCount = 0;
	};

	using FAnchorCameraResult = TValueOrError<FAnchorCamera, FString>;
	using FCandidateRenderResultOrError = TValueOrError<FCandidateRenderResult, FString>;

	FAnchorCameraResult PrepareAnchorCamera(const FRoundContext& InContext);

	FCandidateRenderResultOrError ApplyCandidateToWorld(UWorld& InWorld, const FDoodleTaskCandidate& InCandidate,
	                                                    const FAnchorCamera& InCamera);
}
