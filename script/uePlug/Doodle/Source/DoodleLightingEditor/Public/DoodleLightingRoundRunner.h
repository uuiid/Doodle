#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingExecutor.h"
#include "DoodleLightingTypes.h"
#include "UObject/Object.h"

#include "DoodleLightingRoundRunner.generated.h"

class ULevelSequence;
class UMoviePipelineExecutorBase;
class UMoviePipelineExecutorJob;
class UWorld;
struct FMoviePipelineOutputData;

UCLASS()
class UDoodleLightingRoundRunner : public UObject
{
	GENERATED_BODY()

public:
	DoodleLightingExecutor::FRoundRenderResultOrError StartRender(
		const FString& InRenderMap, const FString& InBaselineLevel, const FString& InMoviePipelineConfig,
		const FString& InOutDir, const FDoodleLightingRig& InRig
	);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundFinished, const FDoodleRoundReport&);
	FOnRoundFinished RoundFinished;

private:
	void FailRound(const FString& InError);

	void OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob);

	void OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData);

	void OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);

	ULevelSequence* LoadedSequence = nullptr;
	UWorld* LoadedMap = nullptr;
	bool bUsingLoadedSequence = false;
	FString CandidateFramesDir;
	FDoodleLightingRig CandidateRig;
	DoodleLightingExecutor::FAnchorCamera AnchorCamera;
	FDoodleCandidateReport CandidateReport;
	double CandidateStartSeconds = 0.0;
	int32 FrameRate = 25;
};
