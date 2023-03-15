#include "DoodleCreateCharacterConfig.h"

FDoodleCreateCharacterConfigUINode* UDoodleCreateCharacterConfig::Add_TreeNode(int32 In_Parent) {
  int32 L_Index             = ListTrees.Emplace();

  ListTrees[L_Index].Parent = In_Parent;
  if (In_Parent != INDEX_NONE) ListTrees[In_Parent].Childs.Add(L_Index);
  return &ListTrees[L_Index];
}

TOptional<FString> UDoodleCreateCharacterConfig::Add_ConfigNode(const FName& In_Bone, int32 In_UI_Parent) {
  if (In_UI_Parent == INDEX_NONE)
    return TOptional<FString>{};

  FDoodleCreateCharacterConfigUINode& L_UI = ListTrees[In_UI_Parent];
  FString L_Key                            = In_Bone.ToString();
  for (auto i = In_UI_Parent; In_UI_Parent != INDEX_NONE; i = ListTrees[i].Parent) {
    L_Key.Append(FString::FromInt(i));
  }

  if (ListConfigNode.Contains(L_Key)) {
    return L_Key;
  }
  auto l_b                                 = L_UI == FDoodleCreateCharacterConfigUINode{};

  FDoodleCreateCharacterConfigNode& L_Node = ListConfigNode.Emplace(In_Bone.ToString());
  L_UI.Keys.Add(L_Key);

  return L_Key;
}

FTransform UDoodleCreateCharacterConfig::Evaluate(const FString& In_BoneName, const float InValue) const {
  const FDoodleCreateCharacterConfigNode* L_Nodel =
      ListConfigNode.Find(In_BoneName);

  if (!L_Nodel) return FTransform::Identity;

  return L_Nodel->WeightCurve.Evaluate(InValue, 1.0f);
}
