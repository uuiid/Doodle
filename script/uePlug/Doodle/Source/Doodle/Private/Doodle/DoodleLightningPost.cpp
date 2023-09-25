#include "Doodle/DoodleLightningPost.h"

#include "Components/DirectionalLightComponent.h"
ADoodleLightingPost::ADoodleLightingPost() {
  LightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("LightComponent"));
  LightComponent->SetupAttachment(RootComponent);
  LightComponent->SetMobility(EComponentMobility::Static);
}