
#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CreateCharacterComponent.generated.h"

class UDoodleCreateCharacterConfig;

UCLASS(Blueprintable, ClassGroup = (Rendering, Common), hidecategories = (Object, "Mesh|SkeletalAsset"), config = Engine, editinlinenew, meta = (BlueprintSpawnableComponent))
class UDoodleCreateCharacterComponent : public USkeletalMeshComponent {
 public:
  GENERATED_BODY()
  UDoodleCreateCharacterComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

 private:
  UPROPERTY(Category = CreateCharacter, EditAnywhere, Transient)
  TObjectPtr<UDoodleCreateCharacterConfig> CreateCharacterConfig;

  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};