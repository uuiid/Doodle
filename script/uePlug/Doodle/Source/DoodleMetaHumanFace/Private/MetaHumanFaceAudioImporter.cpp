#include "MetaHumanFaceAudioImporter.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "IAssetTools.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveTimecodeInfo.h"
#include "UObject/Package.h"

namespace
{
    const TCHAR* const FfmpegExecutable = TEXT("ffmpeg");

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

bool FMetaHumanFaceAudioImporter::ExtractAudioFromVideo(
    const FString& InVideoFile, const FString& InWavFile, FString& OutError
) {
    if (!IFileManager::Get().FileExists(*InVideoFile)) {
        OutError = FString::Printf(TEXT("video file does not exist: %s"), *InVideoFile);
        return false;
    }

    const FString WavDirectory = FPaths::GetPath(InWavFile);
    if (!WavDirectory.IsEmpty() && !IFileManager::Get().MakeDirectory(*WavDirectory, true)) {
        OutError = FString::Printf(TEXT("failed to create directory %s"), *WavDirectory);
        return false;
    }

    IFileManager::Get().Delete(*InWavFile, false, true, true);

    const FString Arguments = FString::Printf(
        TEXT("-hide_banner -loglevel error -y -i \"%s\" -vn -acodec pcm_s16le -ar 16000 -ac 1 \"%s\""),
        *FPaths::ConvertRelativePathToFull(InVideoFile), *FPaths::ConvertRelativePathToFull(InWavFile)
    );

    int32 ReturnCode = -1;
    FString StdOut;
    FString StdErr;
    if (!FPlatformProcess::ExecProcess(FfmpegExecutable, *Arguments, &ReturnCode, &StdOut, &StdErr)) {
        OutError = FString::Printf(TEXT("failed to launch ffmpeg: %s"), *StdErr);
        return false;
    }

    if (ReturnCode != 0) {
        OutError = FString::Printf(TEXT("ffmpeg exited with %d: %s"), ReturnCode, *StdErr);
        return false;
    }

    if (!IsWavHeaderValid(InWavFile)) {
        OutError = FString::Printf(TEXT("ffmpeg produced no readable wav at %s"), *InWavFile);
        return false;
    }

    return true;
}

USoundWave* FMetaHumanFaceAudioImporter::ImportWav(
    const FString& InWavFile, const FString& InPackagePath, double InStartSeconds, FString& OutError
) {
    if (!IsWavHeaderValid(InWavFile)) {
        OutError = FString::Printf(TEXT("wav file is missing or invalid: %s"), *InWavFile);
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
    TimecodeInfo.NumSamplesPerSecond = SoundWave->GetSampleRateForCurrentPlatform();
    TimecodeInfo.NumSamplesSinceMidnight =
        static_cast<uint64>(InStartSeconds * static_cast<double>(TimecodeInfo.NumSamplesPerSecond));
    TimecodeInfo.TimecodeRate = FFrameRate(25, 1);
    TimecodeInfo.bTimecodeIsDropFrame = false;
    SoundWave->SetTimecodeInfo(TimecodeInfo);

    Task->RemoveFromRoot();

    Imported = SoundWave;
    return SoundWave;
}

void FMetaHumanFaceAudioImporter::Release() {
    Imported = nullptr;
}
