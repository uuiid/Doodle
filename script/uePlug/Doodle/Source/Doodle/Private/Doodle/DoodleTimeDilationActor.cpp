#include "Doodle/DoodleTimeDilationActor.h"

#include "GameFramework/WorldSettings.h"

ADoodleTimeDilation::ADoodleTimeDilation() { TimeDilation = 1.0f; }

void ADoodleTimeDilation::BeginPlay() {
  GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);

  for (auto&& i : ExcludeObjects) {
    if (i) i->CustomTimeDilation = 1 / (TimeDilation ? TimeDilation : 1);
  }
}

void ADoodleTimeDilation::Tick(float DeltaTime) {
  GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);

  for (auto&& i : ExcludeObjects) {
    if (i) i->CustomTimeDilation = 1 / (TimeDilation ? TimeDilation : 1);
  }
}
