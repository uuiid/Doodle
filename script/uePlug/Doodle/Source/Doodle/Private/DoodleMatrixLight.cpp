// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleMatrixLight.h"
#include "Components/SpotLightComponent.h"

//添加编辑器显示图标
#include "Components/ArrowComponent.h"

ADoodleMatrixLight::ADoodleMatrixLight()
{
    PrimaryActorTick.bCanEverTick = true;
    USceneComponent *LRoot = CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
    SetRootComponent(LRoot);
}

void ADoodleMatrixLight::CreateLightSqueue()
{

    const float ang{360 / (float)LightSamplesQueue};

    for (auto i = 0; i < LightSamplesQueue; ++i)
    {
        USceneComponent *LSceneComponent =
            NewObject<USceneComponent>(GetRootComponent());

        /// 设置旋转方式
        FTransform l_tran{};
        FVector LLocal{FRotator{0, 0, ang * i}.Vector() * 100};
        LSceneComponent->AddLocalOffset(LLocal);

        /// 添加灯光场景根组件
        // LSceneComponent->SetWorldLocation(LLocal);
        LSceneComponent->AttachToComponent(
            RootComponent,
            FAttachmentTransformRules::KeepRelativeTransform);
        LSceneComponent->RegisterComponent();

        /// 添加箭头指示
        UArrowComponent *LArrowComponent = NewObject<UArrowComponent>(LSceneComponent);
        LArrowComponent->AttachToComponent(
            LSceneComponent,
            FAttachmentTransformRules::KeepRelativeTransform);
        LArrowComponent->RegisterComponent();

        /// 旋转组件指向圆心
        // FRotator LRotator = FRotationMatrix::MakeFromX(
        //                         LSceneComponent->GetComponentLocation() - GetActorLocation())
        //                         .Rotator();
        // LSceneComponent->SetWorldRotation(LRotator);

        SceneComponentList_.Add(LSceneComponent);
        ArrowList_.Add(LArrowComponent);

        CreateLightSquare(i);
    }
}

void ADoodleMatrixLight::CreateLightSquare(
    int InSceneComponentIndex)
{
    for (auto i = 0; i < LightSamplesSquared; ++i)
    {
        for (auto j = 0; j < LightSamplesSquared; ++j)
        {
            USceneComponent *LSceneComponent = SceneComponentList_[InSceneComponentIndex];
            USpotLightComponent *LSpotLightComponent =
                NewObject<USpotLightComponent>(LSceneComponent);
            LSpotLightComponent->AttachToComponent(LSceneComponent,
                                                   FAttachmentTransformRules::KeepRelativeTransform);
            LSpotLightComponent->RegisterComponent();
            LightList_.Add(LSpotLightComponent);
        }
    }
}

void ADoodleMatrixLight::SetLightAttr()
{
}

void ADoodleMatrixLight::TEST()
{
    for (auto *l_un_reg : SceneComponentList_)
    {
        l_un_reg->UnregisterComponent();
    }
    for (auto *l_un_reg : ArrowList_)
    {
        l_un_reg->UnregisterComponent();
    }
    for (auto *l_un_reg : LightList_)
    {
        l_un_reg->UnregisterComponent();
    }
    SceneComponentList_.Empty();
    ArrowList_.Empty();
    LightList_.Empty();
    CreateLightSqueue();
}