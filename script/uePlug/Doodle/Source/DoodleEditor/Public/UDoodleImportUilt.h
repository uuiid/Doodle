#pragma once

#include "CoreMinimal.h"

//这个必须在最后面
#include "UDoodleImportUilt.generated.h"

class ULevelSequence;
class ACineCameraActor;
class UWorld;
class UMovieSceneTrack;

UCLASS(Blueprintable)
class DOODLEEDITOR_API UDoodleImportUilt : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static UDoodleImportUilt *Get();

    UFUNCTION(BlueprintImplementableEvent)
    void create_camera(
        const ULevelSequence *in_level,
        const UWorld *in_world,
        const ACineCameraActor *in_camera) const;

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void add_movie_scene_track(
        const UMovieSceneTrack *in_track,
        FName in_name,
        const FString &in_path) const;
};