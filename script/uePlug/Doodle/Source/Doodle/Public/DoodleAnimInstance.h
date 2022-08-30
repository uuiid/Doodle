
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h" //定时器
#include "Animation/AnimInstance.h"
// 这个必须最后导入
#include "DoodleAnimInstance.generated.h"

UCLASS()
class DOODLE_API UDoodleAnimInstance : public UAnimInstance
{
public:
    GENERATED_BODY()
    UDoodleAnimInstance();

    void DoodleCalculateSpeed();

    void DoodleLookAtObject(const AActor *InActor);

protected:
    float VelocityAttr;
    float DirectionAttr;
};