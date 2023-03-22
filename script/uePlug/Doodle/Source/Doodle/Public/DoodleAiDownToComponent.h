
#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "DoodleAiDownToComponent.generated.h"

class UDoodleAnimInstance;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DOODLE_API UDoodleAiDownToComponent : public UActorComponent {
 public:
  GENERATED_BODY()
  UDoodleAiDownToComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  virtual void BeginPlay() override;

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction
  ) override;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "开始时间")
  float CrouchBegineTime{1.5};
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "随机范围")
  float RandomRadius{0.2};

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doodle", DisplayName = "缓冲曲线")
  FRuntimeFloatCurve CrouchCurve{};

 private:
  void On_Crouch();

  UDoodleAnimInstance* DoodleAnimInstance_P;
  FTimerHandle TimerHandle;

  bool Begine_Crouch_B{};
  float Buffer_Time;
};
