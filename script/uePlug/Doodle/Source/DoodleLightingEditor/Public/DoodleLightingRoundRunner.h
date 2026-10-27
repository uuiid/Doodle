#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "UObject/Object.h"

#include "DoodleLightingRoundRunner.generated.h"

class UMoviePipelineExecutorBase;
class UMoviePipelineExecutorJob;
struct FMoviePipelineOutputData;

UCLASS()
class UDoodleLightingRoundRunner : public UObject
{
	GENERATED_BODY()

public:
	static bool HasAiLightParams();

	void Main(float InDeltaTime);

	void StartRoundFromLaunchPlan(const FDoodleLaunchPlan& InPlan);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundFinished, const FDoodleRoundReport&);
	FOnRoundFinished RoundFinished;

	static void RequestExit(int32 InExitCode);

private:
	bool bInteractiveMode = false;

	void StartRound();

	void FailRound(const FString& InError);

	void OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob);

	void OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData);

	void OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);

	const FDoodleTaskCandidate* FindCandidate(const FString& InCandidateId) const;
	FDoodleCandidateReport* FindCandidateReport(const FString& InCandidateId);

	FString PayloadPath;

	bool StartRoundInternal(FString& OutError);
	FDoodleTaskPayload Payload;
	TArray<FDoodleCandidateReport> CandidateReports;
	TMap<FString, double> CandidateStartSeconds;

	int32 FrameRate = 25;

	bool bRoundStarted = false;
};
