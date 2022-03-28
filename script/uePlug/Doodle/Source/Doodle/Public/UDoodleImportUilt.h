#pragma once

#include "CoreMinimal.h"

//这个必须在最后面
#include "UDoodleImportUilt.generated.h"

class ULevelSequence;
class ACineCameraActor;
class UWorld;
class UMovieSceneTrack;
class UGeometryCache;
class UAnimSequence;

UCLASS(Blueprintable)
class DOODLE_API UDoodleImportUilt : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static UDoodleImportUilt *Get();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, meta = (DefaultToSelf = "Target", HideSelfPin = "Target"))
    void create_camera(
        const ULevelSequence *in_level,
        const ACineCameraActor *in_camera) const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, meta = (DefaultToSelf = "Target", HideSelfPin = "Target"))
    void add_movie_scene_track(
        const UMovieSceneTrack *in_track,
        FName in_name,
        const FString &in_path) const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, meta = (DefaultToSelf = "Target", HideSelfPin = "Target"))
    bool add_skin_scene(const UAnimSequence *in_anim);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, meta = (DefaultToSelf = "Target", HideSelfPin = "Target"))
    bool add_geo_cache_scene(const UAnimSequence *in_anim);
};