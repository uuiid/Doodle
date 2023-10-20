// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DoodleVariantObject.h"
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "DoodleVariantFactory.generated.h"

UCLASS()
class DOODLEEDITOR_API UDoodleVariantFactory : public UFactory {
  GENERATED_BODY()

 public:
  UDoodleVariantFactory();
 protected:
  virtual UObject* FactoryCreateNew(
      UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
  ) override;

  virtual bool ConfigureProperties() override;
  FString GetDefaultNewAssetName() const override;
private:
  FAssetData MeshAssetData;
  FString NewVaraint{"NewVariant"};
};
