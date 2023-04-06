
#pragma once

#include "CoreMinimal.h"

#include "Components/SplineComponent.h"
#include "AITypes.h"                            // ai类型
#include "Navigation/PathFollowingComponent.h"  // ai类型

#include "DoodleAiSplineMoveToCom.generated.h"
class ADoodleAIController;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DOODLE_API UDoodleAiSplineMoveToComponent : public USplineComponent {
 public:
  GENERATED_BODY()
  UDoodleAiSplineMoveToComponent();

  virtual void BeginPlay() override;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "目标接受范围")
  float AcceptanceRadius{266};

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction *ThisTickFunction
  ) override;

 private:
  UFUNCTION(BlueprintCallable)
  void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type MovementResult);
  void GoToRandomWaypoint();
  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);
  FTimerHandle TimerHandle;
  float TimeToPoint;
  float SpeedMax;

  AActor *Actor;

  ADoodleAIController *AiController;
};
