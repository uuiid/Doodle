#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"

// 这个必须最后导入
#include "AiArrayGeneration.generated.h"

class USplineComponent;

UCLASS()
class DOODLE_API ADoodleAiArrayGeneration : public AActor {
  GENERATED_BODY()
 public:
  ADoodleAiArrayGeneration();

 private:
  UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
  TObjectPtr<class USplineComponent> SplineComponent;

 public:
  FRandomStream RandomStream;
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "行", meta = (ClampMin = 1, ClampMax = 1000)
  )
  int32 Row;
  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "列", meta = (ClampMin = 1, ClampMax = 1000)
  )
  int32 Column;

  UPROPERTY(
      EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "随机范围",
      meta = (ClampMin = -1000.0, ClampMax = 1000)
  )
  float RandomRadius;

  virtual void Tick(float DeltaTime) override;

 private:
  UPROPERTY()
  TArray<FVector> Points;
  void GenPoint();
  bool GetRandomPointInRadius(const FVector &Origin, FVector &OutResult);

  virtual void OnConstruction(const FTransform &Transform) override;
};