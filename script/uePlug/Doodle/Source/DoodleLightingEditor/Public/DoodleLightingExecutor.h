#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "Templates/ValueOrError.h"

class AActor;
class APostProcessVolume;
class UWorld;

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
		TArray<TWeakObjectPtr<AActor>> SpawnedActors;
		TWeakObjectPtr<APostProcessVolume> OwnedPostProcessVolume;
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

	void RemoveCandidate(const FCandidateRenderResult& InResult);
}
