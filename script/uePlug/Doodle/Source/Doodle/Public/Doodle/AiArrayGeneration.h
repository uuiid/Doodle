#pragma once

// DoodleAiArrayGeneration.h

#include "CoreMinimal.h"

// 这个必须最后导入
#include "AiArrayGeneration.generated.h"

class UBrushComponent;

UCLASS()
class DOODLE_API ADoodleAiArrayGeneration : public AActor {
  GENERATED_BODY()
 public:
  ADoodleAiArrayGeneration();

 private:
  UPROPERTY(Category = Collision, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
  TObjectPtr<class UBrushComponent> BrushComponent;

 public:
};