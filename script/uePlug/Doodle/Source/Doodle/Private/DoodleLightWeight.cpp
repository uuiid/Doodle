#include "DoodleLightWeight.h"
#include "Engine/Light.h"
FDoodleLightWeightWeak::FDoodleLightWeightWeak() : light(), weight(1) {}

FDoodleLightWeightWeak::FDoodleLightWeightWeak(ALight* in_light,
                                               float in_weight)

    : light(in_light), weight(in_weight) {}
bool FDoodleLightWeightWeak::operator==(
    const FDoodleLightWeightWeak& in_l) const {
  return light == in_l.light;
}
bool FDoodleLightWeightWeak::operator!=(
    const FDoodleLightWeightWeak& in_l) const {
  return !(*this == in_l);
};

FDoodleLightWeight::FDoodleLightWeight() : light(), weight(1) {}

FDoodleLightWeight::FDoodleLightWeight(ALight* in_light, float in_weight)
    : light(in_light), weight(in_weight) {}
