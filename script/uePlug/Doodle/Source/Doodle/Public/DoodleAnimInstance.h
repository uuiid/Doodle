
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"  //定时器
#include "Animation/AnimInstance.h"
// 这个必须最后导入
#include "DoodleAnimInstance.generated.h"

UCLASS(Blueprintable)
class DOODLE_API UDoodleAnimInstance : public UAnimInstance {
 public:
  GENERATED_BODY()
  UDoodleAnimInstance();
  UFUNCTION(BlueprintCallable, Category = "Doodle")
  void DoodleCalculateSpeed();

  UFUNCTION(BlueprintCallable, Category = "Doodle")
  void DoodleLookAtObject(const AActor *InActor);

  void DoodleRandom();

  void NativeBeginPlay() override;

 protected:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle")
  float VelocityAttr{0.f};
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle")
  float DirectionAttrXY{0.f};
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doodle")
  float DirectionAttrZ{0.f};
  UPROPERTY(EditAnywhere, Interp, BlueprintReadOnly, Category = "Doodle")
  int RandomAttr{};

  UPROPERTY(EditAnywhere, Interp, BlueprintReadOnly, Category = "Doodle", Meta = (ClampMin = 0))
  int RandomAttrMin{0};
  UPROPERTY(EditAnywhere, Interp, BlueprintReadOnly, Category = "Doodle", Meta = (ClampMin = 0))
  int RandomAttrMax{6};

  float RandomAttr_InstallValue;
};