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

TOptional<FGuid> UDoodleCreateCharacterConfig::Add_ConfigNode(const FName& In_Bone, int32 In_UI_Parent) {
  if (In_UI_Parent == INDEX_NONE)
    return TOptional<FGuid>{};
  ClearNullKeys();
  FDoodleCreateCharacterConfigUINode& L_UI = ListTrees[In_UI_Parent];

  for (auto&& L_K : L_UI.Keys) {
    if (ListConfigNode[L_K].BoneName == In_Bone) {
      return L_K;
    }
  }

  FGuid L_Key{FGuid::NewGuid()};

  FDoodleCreateCharacterConfigNode& L_Node = ListConfigNode.Emplace(L_Key);

  L_Node.WeightCurve.Resize(4.0f, true, -1.0f, 1.0f);
  L_Node.WeightCurve.TranslationCurve.Name.DisplayName = FName{TEXT("TranslationCurve")};
  L_Node.WeightCurve.RotationCurve.Name.DisplayName    = FName{TEXT("RotationCurve")};
  L_Node.WeightCurve.ScaleCurve.Name.DisplayName       = FName{TEXT("ScaleCurve")};
  L_Node.WeightCurve.UpdateOrAddKey(FTransform::Identity, -1.f);
  L_Node.WeightCurve.UpdateOrAddKey(FTransform::Identity, 0.f);
  L_Node.WeightCurve.UpdateOrAddKey(FTransform::Identity, 1.f);

  L_Node.BoneName = In_Bone;

  L_UI.Keys.Add(L_Key);
  TestCache();
  if (!ListConfigNode_Cache.Contains(In_Bone))
    ListConfigNode_Cache.Emplace(In_Bone);
  ListConfigNode_Cache[In_Bone].Emplace(L_Key, In_UI_Parent);
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
  else
    ListTrees.RemoveAt(In_Node);

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
  FillCache();
  MarkPackageDirty();
  return true;
}

TArray<TTuple<FName, FTransform>> UDoodleCreateCharacterConfig::Evaluate(int32_t In_Index) const {
  if (!ListTrees.IsValidIndex(In_Index)) return {};
  const FDoodleCreateCharacterConfigUINode& L_Bone = ListTrees[In_Index];

  TArray<TTuple<FName, FTransform>> L_Out{};
  TestCache();
  if (ListConfigNode_Cache.IsEmpty())
    return L_Out;
  for (auto&& L_Key : L_Bone.Keys) {
    FTransform L_Tran = FTransform::Identity;
    for (auto&& [guid, id] : ListConfigNode_Cache[ListConfigNode[L_Key].BoneName]) {
      L_Tran *= ListConfigNode[guid].WeightCurve.Evaluate(ListTrees[id].Value, 1.0f);
    }
    L_Out.Emplace(ListConfigNode[L_Key].BoneName, L_Tran);
  }

  return L_Out;
}

void UDoodleCreateCharacterConfig::ClearNullKeys() {
  TSet<FGuid> L_All_Key{};
  ListConfigNode.GetKeys(L_All_Key);
  TSet<FGuid> L_Has_Key{};
  for (auto&& i : ListTrees) {
    for (auto&& j : i.Keys)
      L_Has_Key.Add(j);
  }

  TSet<FGuid> L_Del_Key = L_All_Key.Difference(L_Has_Key);
  for (auto&& i : L_Del_Key) {
    ListConfigNode.Remove(i);
  }

  MarkPackageDirty();
}

void UDoodleCreateCharacterConfig::FillCache() const {
  ListConfigNode_Cache.Empty();

  for (auto i = 0; i < ListTrees.Num(); ++i) {
    auto& L_UI = ListTrees[i];
    for (auto&& L_Key : L_UI.Keys) {
      if (!ListConfigNode.Contains(L_Key))
        continue;
      auto& bone = ListConfigNode[L_Key];
      if (!ListConfigNode_Cache.Contains(bone.BoneName))
        ListConfigNode_Cache.Emplace(bone.BoneName);
      ListConfigNode_Cache[bone.BoneName].Emplace(L_Key, i);
    }
  }
}

void UDoodleCreateCharacterConfig::TestCache() const {
  if (ListConfigNode_Cache.IsEmpty())
    FillCache();
}
