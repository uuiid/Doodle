#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SCharacterEditorViewport;
struct FDoodleCreateCharacterConfigNode;

class ITableRow;
class STableViewBase;

class SCreateCharacterMianUI : public SCompoundWidget, FGCObject {
 public:
  SLATE_BEGIN_ARGS(SCreateCharacterMianUI) {}
  SLATE_END_ARGS()

  using TreeVirwWeightItemType = TSharedPtr<FDoodleCreateCharacterConfigNode>;
  using TreeVirwWeightType     = STreeView<TreeVirwWeightItemType>;
  using TreeVirwWeightDataType = TArray<TreeVirwWeightItemType>;

  // 这里是内容创建函数
  void Construct(const FArguments& Arg);

  // 垃圾回收
  virtual void AddReferencedObjects(FReferenceCollector& collector) override;

  const static FName Name;

  static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

 private:
  TSharedRef<class ITableRow> CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table);
  void CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List);
  TSharedPtr<SCharacterEditorViewport> CharacterEditorViewport;

  // 树部件引用
  TSharedPtr<TreeVirwWeightType> CreateCharacterConfigTree{};
  TreeVirwWeightDataType CreateCharacterConfigTreeData{};
};