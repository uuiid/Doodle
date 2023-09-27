// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DoodleVariantObject.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "DoodleVariantFactory.generated.h"

UCLASS()
class DOODLEEDITOR_API UDoodleVariantFactory : public UFactory
{
    GENERATED_BODY()
public:
    UDoodleVariantFactory() {
        bCreateNew = true;
        bEditAfterNew = true;
        SupportedClass = UDoodleVariantObject::StaticClass();
    }

    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ConfigureProperties() override;

    FAssetData meshAssetData;
};
