#pragma once

#include "CoreMinimal.h"
#include "DoodleCreateCharacterConfig.generated.h"

USTRUCT()
struct FDoodleCreateCharacterConfigNode {
  GENERATED_BODY();

  // 权重曲线
  UPROPERTY();
  FTransformCurve WeightCurve{};

  // 目标骨骼名称
  UPROPERTY();
  FName BoneName{};
};

USTRUCT()
struct FDoodleCreateCharacterConfigUINode {
  GENERATED_BODY();

  UPROPERTY();
  int32 Parent{INDEX_NONE};

  UPROPERTY();
  TArray<int32> Childs;

  // 显示ui名称
  UPROPERTY();
  FName ShowUIName{};

  // 树中配置节点id名称
  UPROPERTY();
  TArray<FString> Keys{};

  // 调整最大值
  UPROPERTY()
  float MaxValue{2.0f};
  // 调整最小值
  UPROPERTY()
  float MinValue{-2.0f};

  inline bool operator==(const FDoodleCreateCharacterConfigUINode& In) const {
    return Tie(Parent, Childs, ShowUIName, Keys, MaxValue, MinValue) == Tie(In.Parent, In.Childs, In.ShowUIName, In.Keys, In.MaxValue, In.MinValue);
  }
  inline bool operator!=(const FDoodleCreateCharacterConfigUINode& In) const {
    return !(*this == In);
  }
};

UCLASS()
class UDoodleCreateCharacterConfig : public UObject {
 public:
  GENERATED_BODY()

  // 权重配置
  UPROPERTY();
  TMap<FString, FDoodleCreateCharacterConfigNode> ListConfigNode;

  UPROPERTY();
  TArray<FDoodleCreateCharacterConfigUINode> ListTrees;

  FDoodleCreateCharacterConfigUINode* Add_TreeNode(int32 In_Parent);
  TOptional<FString> Add_ConfigNode(const FName& In_Bone, int32 In_UI_Parent);

  bool Has_UI_ShowName(const FDoodleCreateCharacterConfigUINode* In_Node, const FString& InName) const;
  void Rename_UI_ShowName(const FDoodleCreateCharacterConfigUINode* In_Node, const FString& InName);


  FTransform Evaluate(const FString& In_BoneName, const float InValue) const;

  USkeletalMesh* GetSkeletalMesh() { return SkeletalMesh; }

  void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh) { SkeletalMesh = InSkeletalMesh; }

 private:
  // 骨骼网格体引用
  UPROPERTY();
  TObjectPtr<USkeletalMesh> SkeletalMesh;
};