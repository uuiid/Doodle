
#pragma once

#include "AITypes.h"  // ai类型
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"  // ai类型
//
#include "DoodleAiSplineMoveToCom.generated.h"
class ADoodleAIController;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DOODLE_API UDoodleAiSplineMoveToComponent : public UActorComponent {
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

  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  TObjectPtr<USplineComponent> SplineCurve{};

  void GoToRandomWaypoint();

  void ReplaceSplineCurve(const TObjectPtr<USplineComponent> &In_Tmp, const FTransform &In_Loc);

 private:
  // UFUNCTION(BlueprintCallable)
  // void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type MovementResult);
  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);
  FTimerHandle TimerHandle;
  float TimeToPoint;
  float SpeedMax;

  AActor *Actor;

  ADoodleAIController *AiController;
};
