#include "UDoodleImportUilt.h"

/// UDoodleImportUilt::add_movie_scene_track
#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"

UDoodleImportUilt *UDoodleImportUilt::Get() {
  TArray<UClass *> ImportUiltClasses;
  GetDerivedClasses(UDoodleImportUilt::StaticClass(), ImportUiltClasses);
  int32 NumClasses = ImportUiltClasses.Num();

  for (auto &&i : ImportUiltClasses) {
    // i->GetName();
    UE_LOG(LogTemp, Log, TEXT("get class name %s"), *i->GetName());
    if (i->GetName() == "doodleInportimpl_C")
      return Cast<UDoodleImportUilt>(i->GetDefaultObject());
  }
  if (NumClasses > 0) {
    return Cast<UDoodleImportUilt>(ImportUiltClasses[0]->GetDefaultObject());
  }
  return nullptr;
}
