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
  
  FString L_Key                            = In_Bone.ToString() + FString::FromInt(In_Bone.GetNumber());

  for (auto i = In_UI_Parent; In_UI_Parent == INDEX_NONE; i = ListTrees[i].Parent) {
    L_Key.Append(FString::FromInt(i));
  }

  if (ListConfigNode.Contains(L_Key)) {
    return L_Key;
  }
  // 防止添加重叠key
  for (auto&& i : ListConfigNode) {
    if (i.Value.BoneName == In_Bone)
      return i.Key;
  }

  FDoodleCreateCharacterConfigNode& L_Node = ListConfigNode.Emplace(L_Key);

  L_Node.WeightCurve.Resize(4.0f, true, -2.0f, 2.0f);
  L_Node.WeightCurve.TranslationCurve.Name.DisplayName = FName{TEXT("TranslationCurve")};
  L_Node.WeightCurve.RotationCurve.Name.DisplayName    = FName{TEXT("RotationCurve")};
  L_Node.WeightCurve.ScaleCurve.Name.DisplayName       = FName{TEXT("ScaleCurve")};
  L_Node.BoneName                                      = In_Bone;

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

bool UDoodleCreateCharacterConfig::Delete_Ui_Node(const FDoodleCreateCharacterConfigUINode* In_Node) {
  if (!In_Node)
    return false;

  int32 L_Index = ListTrees.Find(*In_Node);

  if (L_Index == INDEX_NONE)
    return false;

  TFunction<void(int32)> L_Get_Node_Tree_List{};
  TArray<int32> L_Remove_List{};
  L_Get_Node_Tree_List = [&](int32 In_Index) {
    L_Remove_List += ListTrees[In_Index].Childs;
    for (auto&& i : ListTrees[In_Index].Childs) {
      L_Get_Node_Tree_List(i);
    }
  };

  L_Get_Node_Tree_List(L_Index);

  L_Remove_List.Sort();

  for (auto&& i : L_Remove_List) {
    for (auto&& j : ListTrees[i].Keys) {
      ListConfigNode.Remove(j);
    }
  }

  // 去除父引用
  if (In_Node->Parent != INDEX_NONE)
    ListTrees[In_Node->Parent].Childs.Remove(L_Index);

  TFunction<void(int32, int32)> L_Build_Tree{};
  TArray<FDoodleCreateCharacterConfigUINode> L_ListTree;
  L_Build_Tree = [&](int32 In_Index, int32 In_New_Parent_Index) {
    for (auto&& i : ListTrees[In_Index].Childs) {
      auto&& L_C        = ListTrees[i];
      auto L_Index_Temp = L_ListTree.Emplace(In_New_Parent_Index, L_C.ShowUIName, L_C.Keys, L_C.MaxValue, L_C.MinValue, L_C.Value);
      L_ListTree[In_New_Parent_Index].Childs.Emplace(L_Index_Temp);

      L_Build_Tree(i, L_Index_Temp);
    }
  };

  for (auto i = 0; i < ListTrees.Num(); ++i) {
    if (ListTrees[i].Parent == INDEX_NONE) {
      auto&& L_C        = ListTrees[i];
      auto L_Index_Temp = L_ListTree.Emplace(INDEX_NONE, L_C.ShowUIName, L_C.Keys, L_C.MaxValue, L_C.MinValue, L_C.Value);
      L_Build_Tree(i, L_Index_Temp);
    }
  }
  ListTrees = L_ListTree;
  return true;
}

TTuple<FName, FTransform> UDoodleCreateCharacterConfig::Evaluate(const FString& In_BoneName, const float InValue) const {
  if (!ListConfigNode.Contains(In_BoneName)) return MakeTuple(FName{NAME_None}, FTransform::Identity);

  return MakeTuple(ListConfigNode[In_BoneName].BoneName, ListConfigNode[In_BoneName].WeightCurve.Evaluate(InValue, 1.0f));
}
