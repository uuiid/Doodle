#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "MetaHumanFaceSmokeTest.generated.h"

class UFootageCaptureData;

UCLASS()
class DOODLEMETAHUMANFACE_API UMetaHumanFaceSmokeTest : public UObject
{
    GENERATED_BODY()

  public:
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "DoodleMetaHumanFace")
    static bool RunPreflight();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "DoodleMetaHumanFace")
    static bool RunMonocular(const FString& InFramesDir, const FString& InOutDir);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "DoodleMetaHumanFace")
    static bool CaptureDataRoundTrip(const FString& InFramesDir);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "DoodleMetaHumanFace")
    static void DumpCurveNames(const FString& InOutDir);

  private:
    static UFootageCaptureData* LoadOrBuildCaptureData(const FString& InFramesDir);
};
