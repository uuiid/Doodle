#pragma once

#include "CoreMinimal.h"

// 这个必须在最后面
#include "UDoodleImportUilt.generated.h"

class ULevelSequence;
class ACineCameraActor;
class UWorld;
class UMovieSceneTrack;
class UGeometryCache;
class UAnimSequence;
class AGeometryCacheActor;
class ASkeletalMeshActor;
class UMovieSceneSection;

UCLASS(Blueprintable)
class DOODLE_API UDoodleImportUilt : public UObject {
  GENERATED_BODY()
 public:
  UFUNCTION(BlueprintCallable, Category = "Doodle Editor")
  static UDoodleImportUilt *Get();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Doodle", meta = (DefaultToSelf, HideSelfPin))
  void create_camera(
      const ULevelSequence *in_level,
      const ACineCameraActor *in_camera
  ) const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Doodle", meta = (DefaultToSelf, HideSelfPin))
  void add_movie_scene_track(
      const UMovieSceneTrack *in_track,
      FName in_name,
      const FString &in_path
  ) const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Doodle", meta = (DefaultToSelf, HideSelfPin))
  UMovieSceneSection *add_skin_scene(
      const ULevelSequence *in_level,
      const ASkeletalMeshActor *in_anim,
      const UAnimSequence *in_anim_sequence
  );

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Doodle", meta = (DefaultToSelf, HideSelfPin))
  UMovieSceneSection *add_geo_cache_scene(
      const ULevelSequence *in_level,
      const AGeometryCacheActor *in_geo
  );
};
