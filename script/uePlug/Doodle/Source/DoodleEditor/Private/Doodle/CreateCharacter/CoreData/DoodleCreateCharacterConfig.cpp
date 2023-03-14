#include "DoodleCreateCharacterConfig.h"

FTransform UDoodleCreateCharacterConfig::Evaluate(const FString& In_BoneName, const float InValue) const {
  const FDoodleCreateCharacterConfigNode* L_Nodel =
      ListConfigNode.Find(In_BoneName);

  if (!L_Nodel) return FTransform::Identity;
  return L_Nodel->WeightCurve.Evaluate(InValue, 1.0f);
}
