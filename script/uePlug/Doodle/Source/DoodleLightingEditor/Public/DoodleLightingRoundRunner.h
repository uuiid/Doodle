#pragma once

#include "CoreMinimal.h"
#include "DoodleKitsuClient.h"
#include "DoodleLightingExecutor.h"
#include "DoodleLightingTypes.h"
#include "UObject/Object.h"

#include "DoodleLightingRoundRunner.generated.h"

class ULevelSequence;
class UMoviePipelineExecutorBase;
class UMoviePipelineExecutorJob;
class UMoviePipelinePrimaryConfig;
class UMoviePipelineQueueSubsystem;
class UWorld;
struct FMoviePipelineOutputData;

struct DOODLELIGHTINGEDITOR_API FDoodleRenderedCandidate
{
	FString CandidateId;
	FString FramesDir;
};

UCLASS()
class UDoodleLightingRoundRunner : public UObject
{
	GENERATED_BODY()

public:
	DoodleLightingExecutor::FRoundRenderResultOrError StartRender(
		const DoodleKitsuClient::FDoodleLightingIterationValue& InIteration
	);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundFinished, const TArray<FDoodleRenderedCandidate>&);
	FOnRoundFinished RoundFinished;

private:
	DoodleLightingExecutor::FRoundRenderResultOrError PrepareRound();

	DoodleLightingExecutor::FRoundRenderResultOrError StartCandidate(int32 InIndex);

	void FinishRound();

	void RemoveCandidateFromWorld();

	void OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob);

	void OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData);

	void OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);

	DoodleKitsuClient::FDoodleLightingIterationValue Iteration;
	FString OutDir;
	int32 CurrentIndex = 0;
	TArray<FDoodleRenderedCandidate> RenderedCandidates;

	UMoviePipelineQueueSubsystem* QueueSubsystem = nullptr;
	UMoviePipelinePrimaryConfig* BaseConfig = nullptr;
	UMoviePipelineExecutorBase* ActiveExecutor = nullptr;
	ULevelSequence* LoadedSequence = nullptr;
	UWorld* LoadedMap = nullptr;
	FFrameRate AnchorFrameRate;
	DoodleLightingExecutor::FAnchorCamera AnchorCamera;

	FString CandidateFramesDir;
	FDoodleLightingRig CandidateRig;
	DoodleLightingExecutor::FCandidateRenderResult CandidateSpawned;
	double CandidateStartSeconds = 0.0;
};