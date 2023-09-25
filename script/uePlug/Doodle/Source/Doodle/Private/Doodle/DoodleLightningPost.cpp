#include "Doodle/DoodleLightningPost.h"

#include "Components/DirectionalLightComponent.h"

ADoodleLightingPost::ADoodleLightingPost() {
  LightComponent = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("LightComponent"));
  LightComponent->SetupAttachment(RootComponent);
  LightComponent->SetMobility(EComponentMobility::Movable);

  IntensityMultiplier                = 1.0f;
  SaturationMultiplier               = 1.0f;
  ContrastMultiplier                 = 1.0f;

  Settings.bOverride_ColorSaturation = true;
  Settings.bOverride_ColorContrast   = true;
}
