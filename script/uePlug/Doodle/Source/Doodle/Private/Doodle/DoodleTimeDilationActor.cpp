#include "Doodle/DoodleTimeDilationActor.h"

#include "GameFramework/WorldSettings.h"

ADoodleTimeDilation::ADoodleTimeDilation() {
  PrimaryActorTick.bCanEverTick = true;
  TimeDilation                  = 1.0f;
}

void ADoodleTimeDilation::BeginPlay() {
  GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);

  for (auto&& i : ExcludeObjects) {
    if (i) i->CustomTimeDilation = 1 / (TimeDilation ? TimeDilation : 1);
  }
  Super::BeginPlay();
}

void ADoodleTimeDilation::Tick(float DeltaTime) {
  GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);

  for (auto&& i : ExcludeObjects) {
    if (i) i->CustomTimeDilation = 1 / (TimeDilation ? TimeDilation : 1);
  }
}
