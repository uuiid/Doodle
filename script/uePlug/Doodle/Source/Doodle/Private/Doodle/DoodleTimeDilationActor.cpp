#include "Doodle/DoodleTimeDilationActor.h"

#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"

ADoodleTimeDilation::ADoodleTimeDilation() {
  PrimaryActorTick.bCanEverTick = true;
  TimeDilation                  = 1.0f;
}

void ADoodleTimeDilation::BeginPlay() {
  // GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);

  for (TActorIterator<AActor> L_ActorItr{GetWorld()}; L_ActorItr; ++L_ActorItr) {
    L_ActorItr->CustomTimeDilation = TimeDilation;
  }

  for (auto&& i : ExcludeObjects) {
    if (i) i->CustomTimeDilation = 1;
  }
  Super::BeginPlay();
}

void ADoodleTimeDilation::Tick(float DeltaTime) {
  // GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);
  UE_LOG(LogTemp, Warning, TEXT("ADoodleTimeDilation::Tick %f"), TimeDilation);
  for (TActorIterator<AActor> L_ActorItr{GetWorld()}; L_ActorItr; ++L_ActorItr) {
    L_ActorItr->CustomTimeDilation = TimeDilation;
  }
  for (auto&& i : ExcludeObjects) {
    if (i) i->CustomTimeDilation = 1;
  }
}
