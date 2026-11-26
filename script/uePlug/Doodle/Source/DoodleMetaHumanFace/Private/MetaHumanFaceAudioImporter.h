#pragma once

#include "CoreMinimal.h"
#include "Misc/FrameRate.h"
#include "Misc/Timecode.h"

class USoundWave;

class FMetaHumanFaceAudioImporter
{
  public:
    FMetaHumanFaceAudioImporter();
    ~FMetaHumanFaceAudioImporter();

    FMetaHumanFaceAudioImporter(const FMetaHumanFaceAudioImporter&) = delete;
    FMetaHumanFaceAudioImporter(FMetaHumanFaceAudioImporter&&) = delete;
    FMetaHumanFaceAudioImporter& operator=(const FMetaHumanFaceAudioImporter&) = delete;
    FMetaHumanFaceAudioImporter& operator=(FMetaHumanFaceAudioImporter&&) = delete;

    USoundWave* ImportWav(
        const FString& InWavFile, const FString& InPackagePath, const FTimecode& InStartTimecode,
        const FFrameRate& InTimecodeRate, FString& OutError
    );

    void Release();

  private:
    USoundWave* Imported = nullptr;
};
