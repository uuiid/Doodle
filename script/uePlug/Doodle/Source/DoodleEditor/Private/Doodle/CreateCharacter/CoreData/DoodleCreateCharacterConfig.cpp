#include "DoodleCreateCharacterConfig.h"

FVector UDoodleCreateCharacterConfig::Evaluate(const FName& In_BoneName, const float InValue) const {
  const FDoodleCreateCharacterConfigNode* L_Nodel =
      ListConfigNode.FindByPredicate([&](const FDoodleCreateCharacterConfigNode& InNode) {
        return InNode.BoneName == In_BoneName;
      });

  if (!L_Nodel) return FVector::OneVector * InValue;
  return L_Nodel->WeightCurve.Evaluate(InValue, 1.0f);
}
