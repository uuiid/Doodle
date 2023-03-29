
#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "DoodleAiMoveToComponent.generated.h"
class ADoodleAIController;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DOODLE_API UDoodleAiMoveToComponent : public UActorComponent {
 public:
  GENERATED_BODY()
  UDoodleAiMoveToComponent(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

  virtual void BeginPlay() override;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "方向")
  FVector Direction{1000, 0, 0};

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "随机范围")
  float RandomRadius{500};
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "目标接受范围")
  float AcceptanceRadius{266};

 private:
  void GoToRandomWaypoint();
  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);
  FTimerHandle TimerHandle;

  AActor *Actor;

  ADoodleAIController *AiController;
};
