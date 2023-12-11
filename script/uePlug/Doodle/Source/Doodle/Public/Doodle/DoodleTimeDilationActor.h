#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

// 这个必须最后导入
#include "DoodleTimeDilationActor.generated.h"

UCLASS()
class DOODLE_API ADoodleTimeDilation : public AActor {
  GENERATED_BODY()
 public:
  ADoodleTimeDilation();
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Interp, Category = Doodle, DisplayName = "时间膨胀")
  float TimeDilation;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Doodle, DisplayName = "排除对象")
  TArray<TObjectPtr<AActor>> ExcludeObjects;

  virtual void BeginPlay() override;

  virtual void Tick(float DeltaTime) override;
};