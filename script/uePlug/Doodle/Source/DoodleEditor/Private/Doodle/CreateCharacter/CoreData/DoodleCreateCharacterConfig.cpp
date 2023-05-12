#include "DoodleCreateCharacterConfig.h"

int32 UDoodleCreateCharacterConfig::Add_TreeNode(int32 In_Parent) {
  ++TreeIndex;
  ClearNullKeys();
  int32 L_Index             = ListTrees.Emplace();

  ListTrees[L_Index].Parent = In_Parent;
  if (In_Parent != INDEX_NONE) ListTrees[In_Parent].Childs.Add(L_Index);
  ListTrees[L_Index].ShowUIName = {FString{"AddBone"} + FString::FromInt(TreeIndex)};
  MarkPackageDirty();
  return L_Index;
}

TOptional<FString> UDoodleCreateCharacterConfig::Add_ConfigNode(const FName& In_Bone, int32 In_UI_Parent) {
  if (In_UI_Parent == INDEX_NONE)
    return TOptional<FString>{};
  ClearNullKeys();
  FDoodleCreateCharacterConfigUINode& L_UI = ListTrees[In_UI_Parent];

  FString L_Key                            = In_Bone.ToString() + FString::FromInt(In_Bone.GetNumber());

  for (auto i = In_UI_Parent; i != INDEX_NONE && ListTrees[i].Parent != INDEX_NONE; i = ListTrees[i].Parent) {
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

  L_Node.WeightCurve.Resize(4.0f, true, -1.0f, 1.0f);
  L_Node.WeightCurve.TranslationCurve.Name.DisplayName = FName{TEXT("TranslationCurve")};
  L_Node.WeightCurve.TranslationCurve.UpdateOrAddKey(FVector::ZeroVector, -1.f);
  L_Node.WeightCurve.TranslationCurve.UpdateOrAddKey(FVector::ZeroVector, 0.f);
  L_Node.WeightCurve.TranslationCurve.UpdateOrAddKey(FVector::ZeroVector, 1.0f);
  L_Node.WeightCurve.RotationCurve.Name.DisplayName = FName{TEXT("RotationCurve")};
  L_Node.WeightCurve.RotationCurve.UpdateOrAddKey(FVector::ZeroVector, -1.f);
  L_Node.WeightCurve.RotationCurve.UpdateOrAddKey(FVector::ZeroVector, 0.f);
  L_Node.WeightCurve.RotationCurve.UpdateOrAddKey(FVector::ZeroVector, 1.0f);
  L_Node.WeightCurve.ScaleCurve.Name.DisplayName = FName{TEXT("ScaleCurve")};
  L_Node.WeightCurve.ScaleCurve.UpdateOrAddKey(FVector::ZeroVector, -1.f);
  L_Node.WeightCurve.ScaleCurve.UpdateOrAddKey(FVector::ZeroVector, 0.f);
  L_Node.WeightCurve.ScaleCurve.UpdateOrAddKey(FVector::ZeroVector, 1.0f);

  L_Node.BoneName = In_Bone;

  L_UI.Keys.Add(L_Key);
  MarkPackageDirty();
  return L_Key;
}

bool UDoodleCreateCharacterConfig::Has_UI_ShowName(int32 In_Node, const FString& InName) const {
  if (In_Node != INDEX_NONE)
    return true;

  if (ListTrees[In_Node].Parent != INDEX_NONE) {
    for (auto&& i : ListTrees[ListTrees[In_Node].Parent].Childs) {
      if (ListTrees[i].ShowUIName == InName) return true;
    }
  } else {
    for (auto&& i : ListTrees) {
      if (i.Parent == INDEX_NONE && i.ShowUIName == InName) return true;
    }
  }
  return false;
}

void UDoodleCreateCharacterConfig::Rename_UI_ShowName(int32 In_Node, const FString& InName) {
  if (In_Node != INDEX_NONE) {
    ListTrees[In_Node].ShowUIName = InName;
    MarkPackageDirty();
  }
}

bool UDoodleCreateCharacterConfig::Delete_Ui_Node(int32 In_Node) {
  if (!In_Node)
    return false;

  if (In_Node == INDEX_NONE)
    return false;
  ClearNullKeys();

  TFunction<void(int32)> L_Get_Node_Tree_List{};
  TArray<int32> L_Remove_List{};
  L_Get_Node_Tree_List = [&](int32 In_Index) {
    L_Remove_List += ListTrees[In_Index].Childs;
    for (auto&& i : ListTrees[In_Index].Childs) {
      L_Get_Node_Tree_List(i);
    }
  };

  L_Get_Node_Tree_List(In_Node);

  L_Remove_List.Sort();

  for (auto&& i : L_Remove_List) {
    for (auto&& j : ListTrees[i].Keys) {
      ListConfigNode.Remove(j);
    }
  }

  // 去除父引用
  if (ListTrees[In_Node].Parent != INDEX_NONE)
    ListTrees[ListTrees[In_Node].Parent].Childs.Remove(In_Node);

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
  MarkPackageDirty();
  return true;
}

TTuple<FName, FTransform> UDoodleCreateCharacterConfig::Evaluate(const FString& In_BoneName, const float InValue) const {
  if (!ListConfigNode.Contains(In_BoneName)) return MakeTuple(FName{NAME_None}, FTransform::Identity);

  return MakeTuple(ListConfigNode[In_BoneName].BoneName, ListConfigNode[In_BoneName].WeightCurve.Evaluate(InValue, 1.0f));
}

void UDoodleCreateCharacterConfig::ClearNullKeys() {
  TSet<FString> L_All_Key{};
  ListConfigNode.GetKeys(L_All_Key);
  TSet<FString> L_Has_Key{};
  for (auto&& i : ListTrees) {
    for (auto&& j : i.Keys)
      L_Has_Key.Add(j);
  }

  TSet<FString> L_Del_Key = L_All_Key.Difference(L_Has_Key);
  for (auto&& i : L_Del_Key) {
    ListConfigNode.Remove(i);
  }
  MarkPackageDirty();
}
