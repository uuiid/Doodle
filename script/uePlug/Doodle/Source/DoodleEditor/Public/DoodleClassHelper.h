#pragma once

#include "CoreMinimal.h"
#include "MoviePipeline.h"

#include "DoodleClassHelper.generated.h"

UCLASS()
class UDoodleClassHelper :public UObject
{
public:
    GENERATED_BODY()

    UFUNCTION()
    void OnBeginFrame();

    UFUNCTION()
    void OnMoviePipelineWorkFinished(FMoviePipelineOutputData Data);
private:
    UMoviePipeline* ActiveMoviePipeline;
};