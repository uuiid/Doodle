#include "CreateCharacterActor_Customization.h"
#include "DetailLayoutBuilder.h"
TSharedRef<IDetailCustomization> CreateCharacterActor_Customization::MakeInstance() {
  return MakeShared<CreateCharacterActor_Customization>();
  // return TSharedRef<CreateCharacterActor_Customization>();
}

void CreateCharacterActor_Customization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) {
  DetailBuilder.HideCategory("Animation");
}