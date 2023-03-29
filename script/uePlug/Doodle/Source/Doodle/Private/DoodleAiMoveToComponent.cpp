#include "DoodleAiMoveToComponent.h"
#include "AI/NavigationSystemBase.h"
#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "DetourCrowdAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"

#include "DoodleAiCrowd.h"
#include "DoodleAIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

UDoodleAiMoveToComponent::UDoodleAiMoveToComponent(const FObjectInitializer &ObjectInitializer) : UActorComponent(ObjectInitializer) {
}

void UDoodleAiMoveToComponent::BeginPlay() {
  Super::BeginPlay();

  GoToRandomWaypoint();
}
void UDoodleAiMoveToComponent::GoToRandomWaypoint() {
  if (UWorld *L_World = GetWorld()) {
    L_World->GetTimerManager().SetTimer(
        TimerHandle, this, &UDoodleAiMoveToComponent::GoToRandomWaypoint, 5.0f + FMath::RandRange(-3.0f, 2.0f), false
    );
  }

  if (!Actor || !AiController) {
    Actor = GetOwner<AActor>();
    if (!Actor) {
      UE_LOG(LogTemp, Log, TEXT("return Actor"));
      return;
    }
    AiController = Cast<ADoodleAIController>(UAIBlueprintHelperLibrary::GetAIController(Actor));
  }
  if (!Actor || !AiController) {
    UE_LOG(LogTemp, Log, TEXT("return Actor AiController"));
    return;
  }

  FVector Result;
  if (!GetRandomPointInRadius(Actor->GetActorLocation() + Direction, Result)) {
    UE_LOG(LogTemp, Log, TEXT("GetRandomPointInRadius(Actor->GetActorLocation() + Direction, Result)"));
    return;
  }
  FAIMoveRequest AIMoveRequest{Result};
  AIMoveRequest.SetAcceptanceRadius(AcceptanceRadius);
  AIMoveRequest.SetAllowPartialPath(true);

  AiController->MoveTo(AIMoveRequest);
}

bool UDoodleAiMoveToComponent::GetRandomPointInRadius(const FVector &Origin, FVector &OutResult) {
  UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (!NavSys) {
    return false;
  }

  FNavLocation Result;
  bool bSuccess = NavSys->GetRandomReachablePointInRadius(Origin, RandomRadius, Result);
  // Out
  OutResult     = Result;

  return bSuccess;
}