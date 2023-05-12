
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "CreateCharacterActor.generated.h"

class UDoodleCreateCharacterComponent;

UCLASS(ClassGroup = ISkeletalMeshes, Blueprintable, ComponentWrapperClass, ConversionRoot, meta = (ChildCanTick))
class ADoodleCreateCharacterActor : public AActor {
 public:
  GENERATED_BODY()
  ADoodleCreateCharacterActor();

 private:
  UPROPERTY(Category = CreateCharacter, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Components|SkeletalMesh,Physics", AllowPrivateAccess = "true"))
  TObjectPtr<UDoodleCreateCharacterComponent> SkeletalMeshCom;
};