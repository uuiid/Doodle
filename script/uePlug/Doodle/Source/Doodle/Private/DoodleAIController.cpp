#include "DoodleAIController.h"

#include "AI/NavigationSystemBase.h"
#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "DetourCrowdAIController.h"
#include "DoodleAiCrowd.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
///@brief Actor子类AI控制器
ADoodleAIController::ADoodleAIController(
    const FObjectInitializer &ObjectInitializer
)
    : AAIController(
          ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(
              TEXT("PathFollowingComponent")
          )
      ) {
  if (UCrowdFollowingComponent *CrowdComp = Cast<UCrowdFollowingComponent>(GetComponentByClass(UCrowdFollowingComponent::StaticClass()))) {
    CrowdComp->SuspendCrowdSteering(true);
  }
  // bAllowStrafe = true;
}
///@brief Actor子类控制器中的开始部件
//void ADoodleAIController::BeginPlay() {
//  CastChecked<UCrowdFollowingComponent>(GetPathFollowingComponent())->SetCrowdAvoidanceRangeMultiplier(1);
//  ADoodleAiCrowd *DoodleCurveCrowd = Cast<ADoodleAiCrowd>(GetPawn());
//  if (DoodleCurveCrowd && DoodleCurveCrowd->MoveTo) {
//    GoToRandomWaypoint();
//  }
//}


//void ADoodleAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult &Result) {
//  Super::OnMoveCompleted(RequestID, Result);
//  //  UAIBlueprintHelperLibrary::
//  // UE_LOG(LogTemp, Warning, TEXT("ADoodleAIController::OnMoveCompleted"));
//  // GoToRandomWaypoint();
//}
