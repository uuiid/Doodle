#include "Doodle/DoodleClusterSectionRuntime.h"
#include "EntitySystem/MovieSceneEntitySystemLinker.h"
#include "EntitySystem/MovieSceneEntityBuilder.h"
#include "EntitySystem/BuiltInComponentTypes.h"

void UDoodleClusterSection::ImportEntityImpl(
    UMovieSceneEntitySystemLinker *EntityLinker,
    const FEntityImportParams &Params,
    FImportedEntity *OutImportedEntity)
{
    using namespace UE::MovieScene;

    // USequencerTrackBP *CustomTrack = GetTypedOuter<USequencerTrackBP>();
    // if (CustomTrack->TrackInstanceType.Get())
    // {
    //     FBuiltInComponentTypes *BuiltInComponents = FBuiltInComponentTypes::Get();
    //     if (CustomTrack->TrackType == ETrackType::ObjectTrack)
    //     {
    //         FGuid ObjectBindingID = Params.GetObjectBindingID();
    //         OutImportedEntity->AddBuilder(
    //             FEntityBuilder()
    //                 .Add(BuiltInComponents->TrackInstance, FMovieSceneTrackInstanceComponent{this, CustomTrack->TrackInstanceType})
    //                 .AddConditional(BuiltInComponents->GenericObjectBinding, ObjectBindingID, ObjectBindingID.IsValid()));
    //     }
    //     else
    //     {
    //         // 当master track 时 AddTag(FBuiltInComponentTypes::Get()->Tags.Master) 打标
    //         OutImportedEntity->AddBuilder(
    //             FEntityBuilder()
    //                 .AddTag(FBuiltInComponentTypes::Get()->Tags.Master)
    //                 .Add(BuiltInComponents->TrackInstance, FMovieSceneTrackInstanceComponent{this, CustomTrack->TrackInstanceType}));
    //     }
    // }
}