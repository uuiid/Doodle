#include "DoodleAnimInstance.h"

UDoodleAnimInstance::UDoodleAnimInstance()
    : Super()
{
}

void UDoodleAnimInstance::DoodleCalculateSpeed()
{

    APawn *LPawn = TryGetPawnOwner();

    if (LPawn)
    {
        FVector LVelocity = LPawn->GetVelocity();
        DirectionAttrXY = CalculateDirection(
            LVelocity,
            LPawn->GetBaseAimRotation());
    }
}
void UDoodleAnimInstance::DoodleLookAtObject(const AActor *InActor)
{
    // const APawn *LInPawn = Cast<const APawn>(InActor);
    APawn *LSelfPawn = TryGetPawnOwner();
    // AActor *LSelfPawn = GetOwningActor();
    if (LSelfPawn)
    {
        FVector LInVector = InActor->GetActorLocation();
        FVector LSelfVector = LSelfPawn->GetActorLocation();

        FVector LVector = LSelfVector - LInVector;
        // LVector.RotateAngleAxis()
        // UE_LOG(LogTemp, Log, TEXT("LVector: %s"), *(LVector.ToString()));
        if (!LVector.IsNearlyZero())
        {
            // 此处代码是根据 CalculateDirection 函数来的
            FMatrix LSelfRot = FRotationMatrix{
                LSelfPawn->GetBaseAimRotation()};

            FVector LXYNormal = LVector.GetSafeNormal2D();
            // 获取向前矢量和速度(XY平面) 的cos -> 弧度值
            float ForwardCosAngle = FVector::DotProduct(LSelfRot.GetScaledAxis(EAxis::X), LXYNormal);
            // 将弧度值转换为度
            DirectionAttrXY = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

            // 判断向前轴和速度方向, 根据结果进行翻转
            float RightCosAngle = FVector::DotProduct(LSelfRot.GetScaledAxis(EAxis::Y), LXYNormal);
            if (RightCosAngle < 0)
            {
                DirectionAttrXY *= -1;
            }

            // 这下面是上下角度的代码
            FVector2D LZNormal2D{LVector.X, LVector.Z};
            LZNormal2D.Normalize();
            FVector LZNormal{LZNormal2D.X, 0.0f, LZNormal2D.Y};

            // 获取向前矢量和速度(XY平面) 的cos -> 弧度值
            ForwardCosAngle = FVector::DotProduct(LSelfRot.GetScaledAxis(EAxis::X), LZNormal);
            // 将弧度值转换为度
            DirectionAttrZ = FMath::RadiansToDegrees(FMath::Acos(ForwardCosAngle));

            // 判断向前轴和速度方向, 根据结果进行翻转
            RightCosAngle = FVector::DotProduct(LSelfRot.GetScaledAxis(EAxis::Z), LZNormal);
            if (RightCosAngle < 0)
            {
                DirectionAttrZ *= -1;
            }
            UE_LOG(LogTemp, Log, TEXT("DirectionAttrXY %f DirectionAttrZ: %f"), DirectionAttrXY, DirectionAttrZ);
        }
    }
}