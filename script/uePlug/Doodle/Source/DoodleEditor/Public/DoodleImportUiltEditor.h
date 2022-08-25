#pragma once

#include "CoreMinimal.h"

// 这个必须在最后面
#include "DoodleImportUiltEditor.generated.h"

class ULevelSequence;
class ACineCameraActor;
class UWorld;
class UMovieSceneTrack;
class UGeometryCache;
class UAnimSequence;
class AGeometryCacheActor;
class ASkeletalMeshActor;
class UMovieSceneSection;
class UMovieSceneUserImportFBXSettings;

UCLASS(Blueprintable)
class DOODLEEDITOR_API UDoodleImportUiltEditor : public UObject {
  GENERATED_BODY()
 public:
  UFUNCTION(BlueprintCallable, Category = "Doodle Editor")
  static UDoodleImportUiltEditor *Get();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Doodle Editor", meta = (DefaultToSelf, HideSelfPin))
  UMovieSceneSection *add_camera_fbx_scene(
      UWorld *InWorld,
      const ULevelSequence *in_level,
      const UMovieSceneUserImportFBXSettings *in_setting,
      const FString &InImportFilename,
      const FGuid &in_camera
  );
};
