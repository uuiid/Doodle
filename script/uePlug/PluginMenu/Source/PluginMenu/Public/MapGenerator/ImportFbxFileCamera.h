#pragma once
#include "CoreMinimal.h"
#include "Sequencer/Public/ISequencer.h"

class FImportFbxFileCamera
{
public:
	static bool ImportCameraFromFbxFile(FString& ImportFilename, UMovieScene *MovieScene, ISequencer *Sequencer, TMap<FGuid, FString>& ObjectBindingMap);
};