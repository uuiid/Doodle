#include "Doodle/AiArrayGeneration.h"

#include "Components/BrushComponent.h"

ADoodleAiArrayGeneration::ADoodleAiArrayGeneration() {
  BrushComponent           = CreateDefaultSubobject<UBrushComponent>(TEXT("BrushComponent"));
  BrushComponent->Mobility = EComponentMobility::Static;
  BrushComponent->SetGenerateOverlapEvents(false);
  BrushComponent->SetCanEverAffectNavigation(false);

  RootComponent = BrushComponent;
  SetHidden(true);
  SetCanBeDamaged(false);
  bCollideWhenPlacing          = true;
  SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
}