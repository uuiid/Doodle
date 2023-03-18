#include "DoodleCreateCharacterConfig.h"

FDoodleCreateCharacterConfigUINode* UDoodleCreateCharacterConfig::Add_TreeNode(int32 In_Parent) {
  int32 L_Index             = ListTrees.Emplace();

  ListTrees[L_Index].Parent = In_Parent;
  if (In_Parent != INDEX_NONE) ListTrees[In_Parent].Childs.Add(L_Index);
  ListTrees[L_Index].ShowUIName = {"Add_Bone"};
  return &ListTrees[L_Index];
}

TOptional<FString> UDoodleCreateCharacterConfig::Add_ConfigNode(const FName& In_Bone, int32 In_UI_Parent) {
  if (In_UI_Parent == INDEX_NONE)
    return TOptional<FString>{};

  FDoodleCreateCharacterConfigUINode& L_UI = ListTrees[In_UI_Parent];
  FString L_Key                            = In_Bone.ToString();
  for (auto i = In_UI_Parent; In_UI_Parent == INDEX_NONE; i = ListTrees[i].Parent) {
    L_Key.Append(FString::FromInt(i));
  }

  if (ListConfigNode.Contains(L_Key)) {
    return L_Key;
  }

  FDoodleCreateCharacterConfigNode& L_Node = ListConfigNode.Emplace(In_Bone.ToString());

  L_Node.WeightCurve.Resize(4.0f, true, -2.0f, 2.0f);
  L_Node.WeightCurve.TranslationCurve.Name.DisplayName = FName{TEXT("TranslationCurve")};
  L_Node.WeightCurve.RotationCurve.Name.DisplayName    = FName{TEXT("RotationCurve")};
  L_Node.WeightCurve.ScaleCurve.Name.DisplayName       = FName{TEXT("ScaleCurve")};

  L_UI.Keys.Add(L_Key);

  return L_Key;
}

bool UDoodleCreateCharacterConfig::Has_UI_ShowName(const FDoodleCreateCharacterConfigUINode* In_Node, const FString& InName) const {
  if (!In_Node)
    return true;

  if (In_Node->Parent != INDEX_NONE) {
    for (auto&& i : ListTrees[In_Node->Parent].Childs) {
      if (ListTrees[i].ShowUIName.ToString() == InName) return true;
    }
  } else {
    for (auto&& i : ListTrees) {
      if (i.Parent == INDEX_NONE && i.ShowUIName.ToString() == InName) return true;
    }
  }
  return false;
}

void UDoodleCreateCharacterConfig::Rename_UI_ShowName(const FDoodleCreateCharacterConfigUINode* In_Node, const FName& InName) {
  if (!In_Node)
    return;

  if (auto L_Index = ListTrees.Find(*In_Node)) {
    ListTrees[L_Index].ShowUIName = InName;
  }
}

FTransform UDoodleCreateCharacterConfig::Evaluate(const FString& In_BoneName, const float InValue) const {
  const FDoodleCreateCharacterConfigNode* L_Nodel =
      ListConfigNode.Find(In_BoneName);

  if (!L_Nodel) return FTransform::Identity;

  return L_Nodel->WeightCurve.Evaluate(InValue, 1.0f);
}
