#pragma once

#include "CoreMinimal.h"
#include "CreateCharacter_Factory.generated.h"
#include "Factories/Factory.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"

UCLASS()
class UCreateCharacter : public UFactory {
  GENERATED_BODY()

 public:
  UCreateCharacter(const FObjectInitializer& ObjectInitializer);
  //~MotionField_Factory();

  UPROPERTY(EditAnywhere, Category = CreateCharacter)
  TObjectPtr<USkeletalMesh> SkeletalMesh;

  //~ Begin UFactory Interface
  virtual bool ConfigureProperties() override;
  virtual UObject* FactoryCreateNew(
      UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn,
      FName CallingContext
  ) override;
  virtual UObject* FactoryCreateNew(
      UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
  ) override;
  //~ Begin UFactory Interface
};
