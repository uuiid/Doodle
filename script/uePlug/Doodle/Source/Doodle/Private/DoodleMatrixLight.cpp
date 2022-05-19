// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleMatrixLight.h"

ADoodleMatrixLight::ADoodleMatrixLight()
{
    PrimaryActorTick.bCanEverTick = true;
    CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot");
}

void ADoodleMatrixLight::CreateLightSqueue()
{

    const double ang{360 / (double)LightSamplesQueue};

    for (auto i = 0; i < LightSamplesQueue; ++i)
    {
        USceneComponent *LSceneComponent =
            NewObject<USceneComponent>(GetRootComponent());
        FTransform l_tran{};
        FVector LLocal{0, 0, GetActorLocation().Z};
        LSceneComponent->SetWorldLocation(LLocal);
        SceneComponentList_.Add(LSceneComponent);
        CreateLightSquare(i);
    }
}

void ADoodleMatrixLight::CreateLightSquare(
    int InSceneComponentIndex)
{
    for (auto i = 0; i < LightSamplesSquared; ++i)
    {
        USceneComponent *LSceneComponent = SceneComponentList_[i];
        USpotLightComponent *LSpotLightComponent =
            NewObject<USpotLightComponent>(LSceneComponent);
    }
}

void ADoodleMatrixLight::SetLightAttr()
{
}

void ADoodleMatrixLight::TEST()
{

    FTransform l_lo{GetActorTransform()};
    FTransform l_tran{
        FQuat{},
        l_lo.GetTranslation() + FVector{0, 100, 0}};
    l_lo.DebugPrint();
    l_tran.DebugPrint();
    l_lo *= l_tran;
    l_lo.DebugPrint();
    SetActorTransform(l_lo);
}