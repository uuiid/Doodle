#pragma once

#include "CoreMinimal.h"

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

    static bool ExtractAudioFromVideo(const FString& InVideoFile, const FString& InWavFile, FString& OutError);

    USoundWave* ImportWav(const FString& InWavFile, const FString& InPackagePath, double InStartSeconds, FString& OutError);

    void Release();

  private:
    USoundWave* Imported = nullptr;
};
