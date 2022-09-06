#include "Doodle/DoodleClusterSectionRuntime.h"
#include "EntitySystem/MovieSceneEntitySystemLinker.h"
#include "EntitySystem/MovieSceneEntityBuilder.h"
#include "EntitySystem/BuiltInComponentTypes.h"

#include "Doodle/MovieSceneDoodleClusterTrack.h"

void UDoodleClusterSection::ImportEntityImpl(
    UMovieSceneEntitySystemLinker *EntityLinker,
    const FEntityImportParams &Params,
    FImportedEntity *OutImportedEntity)
{
    //     using namespace UE::MovieScene;
    //     UMovieSceneDoodleClusterTrack *CustomTrack = GetTypedOuter<UMovieSceneDoodleClusterTrack>();
    //     FBuiltInComponentTypes *BuiltInComponents = FBuiltInComponentTypes::Get();
    //     // if (CustomTrack->TrackInstanceType.Get())
    //     // {
    //     //     if (CustomTrack->TrackType == ETrackType::ObjectTrack)
    //     //     {
    //     //         FGuid ObjectBindingID = Params.GetObjectBindingID();
    //     //         OutImportedEntity->AddBuilder(
    //     //             FEntityBuilder()
    //     //                 .Add(BuiltInComponents->TrackInstance,
    //     //                      FMovieSceneTrackInstanceComponent{this, CustomTrack->TrackInstanceType})
    //     //                 .AddConditional(BuiltInComponents->GenericObjectBinding,
    //     //                                 ObjectBindingID, ObjectBindingID.IsValid()));
    //     //     }
    //     //     else
    //     //     {
    //     //         // 当master track 时 AddTag(FBuiltInComponentTypes::Get()->Tags.Master) 打标
    //     //         OutImportedEntity->AddBuilder(
    //     //             FEntityBuilder()
    //     //                 .AddTag(FBuiltInComponentTypes::Get()->Tags.Master)
    //     //                 .Add(BuiltInComponents->TrackInstance,
    //     //                      FMovieSceneTrackInstanceComponent{this, CustomTrack->TrackInstanceType}));
    //     //     }
    //     // }

    //     FGuid ObjectBindingID = Params.GetObjectBindingID();

    //     OutImportedEntity->AddBuilder(
    //         FEntityBuilder()
    //             .Add(BuiltInComponents->TrackInstance,
    //                  FMovieSceneTrackInstanceComponent{this,
    //                                                    CustomTrack->TrackInstanceType})
    //             .AddConditional(BuiltInComponents->GenericObjectBinding,
    //                             ObjectBindingID, ObjectBindingID.IsValid()));

    using namespace UE::MovieScene;

    // // FMovieSceneTrackInstanceComponent TrackInstance { this, UMovieSceneCameraCutTrackInstance::StaticClass() };

    // OutImportedEntity->AddBuilder(
    //     FEntityBuilder()
    //         .AddTag(FBuiltInComponentTypes::Get()->Tags.Master)
    //         .Add(FBuiltInComponentTypes::Get()->TrackInstance, DoodleLockAtObject));
}