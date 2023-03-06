#include "CreateCharacterTree.h"

#include "CharacterEditorViewport.h"
#include "Engine/SkeletalMeshSocket.h"     // 骨骼 Socket
#include "Widgets/Input/SSlider.h"         // 滑动条
#include "SScrubControlPanel.h"            // 时间控制
#include "Framework/Docking/TabManager.h"  // 选项卡布局
#include "Doodle/CreateCharacter/CoreData/DoodleCreateCharacterConfig.h"

class SCreateCharacterConfigTreeItem : public SMultiColumnTableRow<SCreateCharacterTree::TreeVirwWeightItemType> {
 public:
  using Super = SMultiColumnTableRow<SCreateCharacterTree::TreeVirwWeightItemType>;
  SLATE_BEGIN_ARGS(SCreateCharacterConfigTreeItem)
      : _ItemData() {}

  SLATE_ARGUMENT(SCreateCharacterTree::TreeVirwWeightItemType, ItemData)

  SLATE_END_ARGS()

  // 这里是内容创建函数
  void Construct(const FArguments& Arg, const TSharedRef<STableViewBase>& OwnerTableView) {
    ItemData = Arg._ItemData;
    Super::Construct(Super::FArguments{}, OwnerTableView);
  }

  TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override {
    if (InColumnName == "Name") {
      return SNew(STextBlock).Text(FText::FromString(TEXT("name")));
    } else if (InColumnName == "Value") {
      return SNew(SSlider).MaxValue(10.f).MinValue(-10.0f);
    } else {
      return SNew(STextBlock);
    }
  }

 private:
  SCreateCharacterTree::TreeVirwWeightItemType ItemData;
};

const FName SCreateCharacterTree::Name{"Doodle_SCreateCharacterTree"};

void SCreateCharacterTree::Construct(const FArguments& Arg) {
  Super::Construct(
      Super::FArguments{}
          .TreeItemsSource(&CreateCharacterConfigTreeData)
          .OnGenerateRow(TreeVirwWeightType::FOnGenerateRow::CreateSP(this, &SCreateCharacterTree::CreateCharacterConfigTreeData_Row))
          .OnGetChildren(TreeVirwWeightType::FOnGetChildren::CreateSP(this, &SCreateCharacterTree::CreateCharacterConfigTreeData_GetChildren))
          .HeaderRow(
              // clang-format off
              SNew(SHeaderRow) 
            + SHeaderRow::Column(FName{TEXT("Name")}) 
            + SHeaderRow::Column(FName{TEXT("Value")})
              // clang-format on
          )
  );
}

void SCreateCharacterTree::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterTree::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterTree)];  //
}

TSharedRef<class ITableRow> SCreateCharacterTree::CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table) {
  return SNew(SCreateCharacterConfigTreeItem, In_Table).ItemData(In_Value);
}

void SCreateCharacterTree::CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List) {
}

void SCreateCharacterTree::CreateUITree() {
}
