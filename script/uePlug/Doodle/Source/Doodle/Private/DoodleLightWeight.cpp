#include "DoodleLightWeight.h"

#include "Components/LightComponent.h"
#include "Engine/Light.h"

FDoodleLightWeight::FDoodleLightWeight() : light(), weight(1) {}

FDoodleLightWeight::FDoodleLightWeight(ULightComponent* in_light, float in_weight)
    : light(in_light), weight(in_weight) {}
