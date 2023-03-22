
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
  FVector Direction;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "随机范围")
  float RandomRadius{100};
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "目标接受范围")
  float AcceptanceRadius{50};

 private:
  void GoToRandomWaypoint();
  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);
  FTimerHandle TimerHandle;

  AActor *Actor;

  ADoodleAIController *AiController;
};
