#include "DoodleCreateCharacterConfig.h"

FTransform UDoodleCreateCharacterConfig::Evaluate(const FName& In_BoneName, const float InValue) const {
  const FDoodleCreateCharacterConfigNode* L_Nodel =
      ListConfigNode.FindByPredicate([&](const FDoodleCreateCharacterConfigNode& InNode) {
        return InNode.BoneName == In_BoneName;
      });

  if (!L_Nodel) return FTransform::Identity;
  return L_Nodel->WeightCurve.Evaluate(InValue, 1.0f);
}
