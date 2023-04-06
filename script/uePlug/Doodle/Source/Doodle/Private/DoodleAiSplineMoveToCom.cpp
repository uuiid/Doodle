#include "DoodleAiSplineMoveToCom.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"  // 蓝图ai帮助
#include "AI/NavigationSystemBase.h"
#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "DetourCrowdAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"

#include "DoodleAiCrowd.h"
#include "DoodleAIController.h"
UDoodleAiSplineMoveToComponent::UDoodleAiSplineMoveToComponent() {
  PrimaryComponentTick.bCanEverTick = true;
}

void UDoodleAiSplineMoveToComponent::BeginPlay() {
  Super::BeginPlay();

  TimeToPoint = 1;
  GoToRandomWaypoint();
  // GetDistance() / ;

  SpeedMax = GetOwner<APawn>()->FindComponentByClass<UMovementComponent>()->GetMaxSpeed();
}

void UDoodleAiSplineMoveToComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  TimeToPoint += (DeltaTime * SpeedMax);
  if (TimeToPoint >= GetSplineLength()) TimeToPoint -= GetSplineLength();
}

void UDoodleAiSplineMoveToComponent::OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type MovementResult) {
  FVector Result;
  if (!GetRandomPointInRadius(Actor->GetActorLocation(), Result)) {
    return;
  }
  if (MovementResult == EPathFollowingResult::Success)
    ++TimeToPoint;

  if (TimeToPoint >= GetNumberOfSplinePoints()) TimeToPoint -= GetNumberOfSplinePoints();

  FAIMoveRequest AIMoveRequest{Result};
  AIMoveRequest.SetAcceptanceRadius(AcceptanceRadius);
  AIMoveRequest.SetAllowPartialPath(true);

  FPathFollowingRequestResult L_R = AiController->MoveTo(AIMoveRequest);
}

void UDoodleAiSplineMoveToComponent::GoToRandomWaypoint() {
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
  UWorld *L_World = GetWorld();
  if (!L_World)
    return;

  FVector Result;
  ;
  if (!GetRandomPointInRadius(Actor->GetActorLocation(), Result)) {
    // L_World->GetTimerManager().SetTimer(
    //     TimerHandle, this, &UDoodleAiSplineMoveToComponent::GoToRandomWaypoint, 0.1f, false
    //);
    return;
  }
  FAIMoveRequest AIMoveRequest{Result};
  AIMoveRequest.SetAcceptanceRadius(AcceptanceRadius);
  AIMoveRequest.SetAllowPartialPath(true);
  // UAIBlueprintHelperLibrary::SimpleMoveToLocation();
  FPathFollowingRequestResult L_R = AiController->MoveTo(AIMoveRequest);

  switch (L_R.Code) {
    case EPathFollowingRequestResult::AlreadyAtGoal:
      ++TimeToPoint;
      L_World->GetTimerManager().SetTimer(
          TimerHandle, this, &UDoodleAiSplineMoveToComponent::GoToRandomWaypoint, 0.1f, false
      );
      break;
    case EPathFollowingRequestResult::Failed:
      L_World->GetTimerManager().SetTimer(
          TimerHandle, this, &UDoodleAiSplineMoveToComponent::GoToRandomWaypoint, 0.15f + FMath::RandRange(-0.1f, 0.2f), false
      );
      break;
    case EPathFollowingRequestResult::RequestSuccessful:
      L_World->GetTimerManager().SetTimer(
          TimerHandle, this, &UDoodleAiSplineMoveToComponent::GoToRandomWaypoint, 0.15f + FMath::RandRange(-0.1f, 0.2f), false
      );
      // AiController->ReceiveMoveCompleted.AddDynamic(this, &UDoodleAiSplineMoveToComponent::OnMoveCompleted);
      break;
  }
}

bool UDoodleAiSplineMoveToComponent::GetRandomPointInRadius(const FVector &Origin, FVector &OutResult) {
  UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
  if (!NavSys) {
    return false;
  }
  FVector L_Forward = Actor->GetActorForwardVector();
  L_Forward *= (SpeedMax * 3.f);

  L_Forward = Actor->GetActorLocation() + L_Forward;
  // FVector L_point = GetLocationAtSplinePoint(TimeToPoint, ESplineCoordinateSpace::World);
  // UE_LOG(LogTemp, Log, TEXT("index  %s"), *L_Forward.ToString());
  //  FNavLocation Result;
  //  bool bSuccess = NavSys->GetRandomReachablePointInRadius(L_point, RandomRadius, Result);
  //   Out
  OutResult = FindLocationClosestToWorldLocation(L_Forward, ESplineCoordinateSpace::World);
  // DrawDebugSphere(GetWorld(), L_Forward, 10.f, 3, FColor{255, 0, 0});

  return true;
}
