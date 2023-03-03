#include "CreateCharacterMianUI.h"

#include "CharacterEditorViewport.h"
#include "Engine/SkeletalMeshSocket.h"  // 骨骼 Socket
#include "Widgets/Input/SSlider.h"      // 滑动条
#include "SScrubControlPanel.h"         // 时间控制

class SCreateCharacterConfigTreeItem : public SMultiColumnTableRow<SCreateCharacterMianUI::TreeVirwWeightItemType> {
 public:
  using Super = SMultiColumnTableRow<SCreateCharacterMianUI::TreeVirwWeightItemType>;
  SLATE_BEGIN_ARGS(SCreateCharacterConfigTreeItem)
      : _ItemData() {}

  SLATE_ARGUMENT(SCreateCharacterMianUI::TreeVirwWeightItemType, ItemData)

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
  SCreateCharacterMianUI::TreeVirwWeightItemType ItemData;
};

const FName SCreateCharacterMianUI::Name{"Doodle_CreateCharacterMianUI"};

void SCreateCharacterMianUI::Construct(const FArguments& Arg) {
  // clang-format off
  CharacterEditorViewport = SNew(SCharacterEditorViewport)
    .ToolTipText(FText::FromString(TEXT("render windwos")));
  // clang-format on

  // clang-format off
  ChildSlot
  [
    SNew(SBorder)
    .BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 0.0f))
    .BorderImage(new FSlateBrush())
    .HAlign(HAlign_Fill)
    [
      SNew(SHorizontalBox)
      // 渲染槽
      + SHorizontalBox::Slot()
      .FillWidth(1.0f)
      [
        CharacterEditorViewport.ToSharedRef()
      ]

      // 树小部件
      + SHorizontalBox::Slot()
      .FillWidth(1.0f)
      [
        SAssignNew(CreateCharacterConfigTree, TreeVirwWeightType)
        .TreeItemsSource(&CreateCharacterConfigTreeData)
        .OnGenerateRow(TreeVirwWeightType::FOnGenerateRow::CreateSP(this, &SCreateCharacterMianUI::CreateCharacterConfigTreeData_Row))
        .OnGetChildren(TreeVirwWeightType::FOnGetChildren::CreateSP(this, &SCreateCharacterMianUI::CreateCharacterConfigTreeData_GetChildren))
        .HeaderRow
        (
          SNew(SHeaderRow)
          + SHeaderRow::Column(FName{TEXT("Name")})
          + SHeaderRow::Column(FName{TEXT("Value")})
        )
      ]
    ]
  ];
  // clang-format on
}

void SCreateCharacterMianUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SCreateCharacterMianUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SCreateCharacterMianUI)];  //
}

TSharedRef<class ITableRow> SCreateCharacterMianUI::CreateCharacterConfigTreeData_Row(TreeVirwWeightItemType In_Value, const TSharedRef<class STableViewBase>& In_Table) {
  return SNew(SCreateCharacterConfigTreeItem, In_Table).ItemData(In_Value);
}

void SCreateCharacterMianUI::CreateCharacterConfigTreeData_GetChildren(TreeVirwWeightItemType In_Value, TreeVirwWeightDataType& In_List) {
}
