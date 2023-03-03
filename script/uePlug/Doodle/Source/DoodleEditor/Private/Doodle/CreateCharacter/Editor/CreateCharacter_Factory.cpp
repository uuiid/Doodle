#include "CreateCharacter_Factory.h"

#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterAnimAsset.h"
UCreateCharacter::UCreateCharacter(const FObjectInitializer& ObjectInitializer) {
  bCreateNew     = true;
  bEditAfterNew  = true;
  SupportedClass = UDoodleCreateCharacterAnimAsset::StaticClass();
}

bool UCreateCharacter::ConfigureProperties() { return false; }

UObject* UCreateCharacter::FactoryCreateNew(
    UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn,
    FName CallingContext
) {
  return nullptr;
}

UObject* UCreateCharacter::FactoryCreateNew(
    UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
) {
  return nullptr;
}
