#include "DoodleAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "AI/NavigationSystemBase.h"
#include "NavigationSystem.h"

ADoodleAIController::ADoodleAIController(
    const FObjectInitializer &ObjectInitializer)
    : AAIController(
          ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(
              TEXT("PathFollowingComponent"))) {}

void ADoodleAIController::BeginPlay()
{
    GetPathFollowingComponent()->OnRequestFinished.AddLambda([this](FAIRequestID RequestID,
                                                                    const FPathFollowingResult &Result)
                                                             { GoToRandomWaypoint(); });
    GoToRandomWaypoint();
}

void ADoodleAIController::GoToRandomWaypoint()
{
    FVector Result;
    if (GetRandomPointInRadius(GetPawn()->GetActorLocation(), 600, Result))
    {
        FAIMoveRequest AIMoveRequest{Result};
        AIMoveRequest.SetAcceptanceRadius(50);
        AIMoveRequest.SetAllowPartialPath(true);

        if (MoveTo(AIMoveRequest).Code != EPathFollowingRequestResult::Type::RequestSuccessful)
            GoToRandomWaypoint();
    }
}

bool ADoodleAIController::GetRandomPointInRadius(const FVector &Origin, float Radius, FVector &OutResult)
{
    UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return false;
    }

    FNavLocation Result;
    bool bSuccess = NavSys->GetRandomReachablePointInRadius(Origin, 600, Result);

    // Out
    OutResult = Result;
    return bSuccess;
}
void ADoodleAIController::OnMoveCompleted(FAIRequestID RequestID,
                                          const FPathFollowingResult &Result)
{
    Super::OnMoveCompleted(RequestID, Result);
    //  UAIBlueprintHelperLibrary::
    UE_LOG(LogTemp, Warning, TEXT("ADoodleAIController::OnMoveCompleted"));
    // GoToRandomWaypoint();
}