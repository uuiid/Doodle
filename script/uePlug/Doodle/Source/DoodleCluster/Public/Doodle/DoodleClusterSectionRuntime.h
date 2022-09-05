#pragma once

#include "MovieSceneSection.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "DoodleClusterSectionRuntime.generated.h"

class UMovieSceneSignedObject;

class UMovieSceneEntitySystemLinker;
class FEntityImportParams;
class FImportedEntity;

UCLASS()
class DOODLECLUSTER_API UDoodleClusterSection
    : public UMovieSceneSection
//   public IMovieSceneEntityProvider
{
    GENERATED_BODY()

public:
    // virtual void ImportEntityImpl(
    //     UMovieSceneEntitySystemLinker *EntityLinker,
    //     const FEntityImportParams &Params,
    //     FImportedEntity *OutImportedEntity) override;
};