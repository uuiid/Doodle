#pragma once
#include "CoreMinimal.h"

#include "EditorFramework/AssetImportData.h"
#include "AbcWrap/DoodleAbcImportSetting.h"
#include "DoodleAbcAssetImportData.generated.h"

UCLASS()
class DOODLEEDITOR_API UDoodleAbcAssetImportData : public UAssetImportData
{
    GENERATED_UCLASS_BODY()
public:
    UDoodleAbcAssetImportData();

    UPROPERTY()
    TArray<FString> TrackNames;

    UPROPERTY()
    FDooAbcSamplingSettings SamplingSettings;
};