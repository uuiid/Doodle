#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingExecutor.h"
#include "UObject/Object.h"

#include "DoodleLightingRoundRunner.generated.h"

class UMoviePipelineExecutorBase;
class UMoviePipelineExecutorJob;
struct FMoviePipelineOutputData;
struct FDoodleLaunchPlan;
struct FDoodleRoundReport;

UCLASS()
class UDoodleLightingRoundRunner : public UObject
{
	GENERATED_BODY()

public:
	void StartRoundFromLaunchPlan(const FDoodleLaunchPlan& InPlan);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundFinished, const FDoodleRoundReport&);
	FOnRoundFinished RoundFinished;

private:
	void FailRound(const FString& InError);

	void OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob);

	void OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData);

	void OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);

	const FDoodleTaskCandidate* FindCandidate(const FString& InCandidateId) const;
	FDoodleCandidateReport* FindCandidateReport(const FString& InCandidateId);

	bool StartRoundInternal(FString& OutError);
	FDoodleTaskPayload Payload;
	DoodleLightingExecutor::FAnchorCamera AnchorCamera;
	TArray<FDoodleCandidateReport> CandidateReports;
	TMap<FString, double> CandidateStartSeconds;

	int32 FrameRate = 25;
};
