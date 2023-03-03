#include "CreateCharacter_Factory.h"

#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"
UCreateCharacter::UCreateCharacter(const FObjectInitializer& ObjectInitializer) {
  bCreateNew     = true;
  bEditAfterNew  = true;
  SupportedClass = UDoodleCreateCharacterConfig::StaticClass();
}

bool UCreateCharacter::ConfigureProperties() { return true; }

UObject* UCreateCharacter::FactoryCreateNew(
    UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn,
    FName CallingContext
) {
  UDoodleCreateCharacterConfig* L_Ass = NewObject<UDoodleCreateCharacterConfig>(InParent, Class, Name, Flags | RF_Transactional);

  if (L_Ass && SkeletalMesh) {
    L_Ass->SetSkeletalMesh(SkeletalMesh);
    return L_Ass;
  }

  return nullptr;
}

UObject* UCreateCharacter::FactoryCreateNew(
    UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
) {
  return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}
