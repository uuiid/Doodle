#pragma once
#include "UObject/Object.h"
#include "Misc/CoreDelegates.h"
#include "BatchRenderQueue.generated.h"

class UMoviePipelineExecutorBase;
class UMoviePipelineExecutorJob;
class UMoviePipelinePIEExecutor;
class UBatchRenderQueueEngineSubsystem;

DECLARE_DELEGATE(FOnBatchRenderExecutorFinished);

struct URenderJobInfo
{
    UPROPERTY(BlueprintReadWrite)
    FString Map;


    UPROPERTY(BlueprintReadWrite)
    FString Sequence;


    UPROPERTY(BlueprintReadWrite)
    FString OutputDir;
};

UCLASS(Blueprintable)
class BATCHRENDER_API UBatchRenderQueue : public UObject
{
    GENERATED_BODY()

	// UObject interface

    
public:

    UFUNCTION(BlueprintCallable, Category = "Doodle|BatchRender")
    void RenderMovies();
    TArray<URenderJobInfo> BatchJobs;
    
    FOnBatchRenderExecutorFinished& OnExecutorFinished()
    {
        return OnExecutorFinishedDelegate;
    }
    FOnBatchRenderExecutorFinished OnExecutorFinishedDelegate;
    //UFUNCTION(BlueprintCallable)
    //void DeleteFrame();
private:
    UFUNCTION()
   void OnQueueFinished(TArray<UMoviePipelineExecutorJob*> Jobs);

   bool DeleteFrame(FString Dir);
   void ConvertPNGSequenceToVideo(const FString& FFmpegPath, const FString& InputPattern, const FString& OutputFilePath, int32 FrameRate);
   void OnQueueFinished();


    UFUNCTION()
    void OnIndividualJobFinished(UMoviePipelineExecutorJob* InJob, bool bSuccess);

    UMoviePipelinePIEExecutor* Executor;
    UBatchRenderQueueEngineSubsystem* PipelineSubsystem;
    
};