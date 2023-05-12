#include "CreateCharacterComponent.h"

UDoodleCreateCharacterComponent::UDoodleCreateCharacterComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
}

void UDoodleCreateCharacterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  FProperty* L_PropertyThatChanged = PropertyChangedEvent.Property;
  if (!L_PropertyThatChanged)
    return;

  if (L_PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, CreateCharacterConfig)) {
    //SetAnimation(CreateCharacterConfig);
    InitAnim(false);
  }
}
