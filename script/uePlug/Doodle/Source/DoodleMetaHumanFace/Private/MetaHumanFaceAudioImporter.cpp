#include "MetaHumanFaceAudioImporter.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveTimecodeInfo.h"
#include "UObject/Package.h"

namespace
{
    const TCHAR* const BinkAudioDecoderModule = TEXT("BinkAudioDecoder");

    bool LoadBinkAudioDecoder() {
        if (FModuleManager::Get().IsModuleLoaded(BinkAudioDecoderModule)) {
            return true;
        }

        return FModuleManager::Get().LoadModule(BinkAudioDecoderModule) != nullptr;
    }

    bool IsWavHeaderValid(const FString& InWavFile) {
        TArray<uint8> Header;
        IFileManager& FileManager = IFileManager::Get();
        if (!FileManager.FileExists(*InWavFile)) {
            return false;
        }

        const int64 FileSize = FileManager.FileSize(*InWavFile);
        if (FileSize < 44) {
            return false;
        }

        return FFileHelper::LoadFileToArray(Header, *InWavFile) && Header.Num() >= 12 &&
               Header[0] == 'R' && Header[1] == 'I' && Header[2] == 'F' && Header[3] == 'F';
    }
}

FMetaHumanFaceAudioImporter::FMetaHumanFaceAudioImporter() {
}

FMetaHumanFaceAudioImporter::~FMetaHumanFaceAudioImporter() {
    Release();
}

USoundWave* FMetaHumanFaceAudioImporter::ImportWav(
    const FString& InWavFile, const FString& InPackagePath, const FTimecode& InStartTimecode,
    const FFrameRate& InTimecodeRate, FString& OutError
) {
    if (!IsWavHeaderValid(InWavFile)) {
        OutError = FString::Printf(TEXT("wav file is missing or invalid: %s"), *InWavFile);
        return nullptr;
    }

    if (!LoadBinkAudioDecoder()) {
        OutError = FString::Printf(
            TEXT("failed to load the %s module, the imported sound wave cannot be decoded"), BinkAudioDecoderModule
        );
        return nullptr;
    }

    const FString PackagePath = InPackagePath.IsEmpty() ? FString(TEXT("/Temp")) : InPackagePath;
    const FString AssetName = FPaths::GetBaseFilename(InWavFile);

    UAssetImportTask* Task = NewObject<UAssetImportTask>();
    Task->AddToRoot();
    Task->Filename = FPaths::ConvertRelativePathToFull(InWavFile);
    Task->DestinationPath = PackagePath;
    Task->DestinationName = AssetName;
    Task->bAutomated = true;
    Task->bReplaceExisting = true;
    Task->bSave = false;

    IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
    AssetTools.ImportAssetTasks({ Task });

    USoundWave* SoundWave = nullptr;
    const TArray<UObject*>& ImportedObjects = Task->GetObjects();
    for (UObject* Object : ImportedObjects) {
        if (USoundWave* Found = Cast<USoundWave>(Object)) {
            SoundWave = Found;
            break;
        }
    }

    if (SoundWave == nullptr) {
        OutError = FString::Printf(TEXT("failed to import wav %s into %s"), *InWavFile, *PackagePath);
        Task->RemoveFromRoot();
        return nullptr;
    }

    FSoundWaveTimecodeInfo TimecodeInfo;
    const int32 SampleRate = SoundWave->GetSampleRateForCurrentPlatform();
    const double RateDecimal = InTimecodeRate.AsDecimal();
    const int32 StartFrame = InStartTimecode.ToFrameNumber(InTimecodeRate).Value;
    const double StartSeconds =
        RateDecimal > 0.0 ? static_cast<double>(StartFrame) / RateDecimal : 0.0;

    TimecodeInfo.NumSamplesPerSecond = static_cast<uint32>(SampleRate);
    TimecodeInfo.NumSamplesSinceMidnight =
        static_cast<uint64>(StartSeconds * static_cast<double>(SampleRate));
    TimecodeInfo.TimecodeRate = InTimecodeRate;
    TimecodeInfo.bTimecodeIsDropFrame = InStartTimecode.bDropFrameFormat;
    SoundWave->SetTimecodeInfo(TimecodeInfo);

    Task->RemoveFromRoot();

    Imported = SoundWave;
    return SoundWave;
}

void FMetaHumanFaceAudioImporter::Release() {
    Imported = nullptr;
}
