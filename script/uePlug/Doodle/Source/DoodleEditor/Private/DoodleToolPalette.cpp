#include "DoodleToolPalette.h"
#include "SlateBasics.h"

#include "DoodleStyle.h"
// 资源注册表
#include "AssetRegistryModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
// #include "IPlacementModeModule.h"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDoodleToolPalette::Construct(const FArguments& InArgs) {
  p_list.Add(MakeShareable(new SDoodleToolListItem{nullptr, FAssetData{}}));

  SAssignNew(DoodleClassList, SListView<TSharedPtr<SDoodleToolListItem>>)
      .SelectionMode(ESelectionMode::Single)
      .ListItemsSource(&p_list)
      .OnGenerateRow(this, &SDoodleToolPalette::MakeListViewWidget)
      .ItemHeight({35});

  this->ChildSlot[SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(SScrollBorder, DoodleClassList.ToSharedRef())[DoodleClassList.ToSharedRef()]]

  ];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<ITableRow> SDoodleToolPalette::MakeListViewWidget(TSharedPtr<SDoodleToolListItem> Item, const TSharedRef<STableViewBase>& OwnerTable) {
  TSharedRef<STableRow<TSharedPtr<SDoodleToolListItem>>> TableRowWidget =
      SNew(STableRow<TSharedPtr<SDoodleToolListItem>>, OwnerTable);

  TSharedPtr<SHorizontalBox> ContentBox{};

  TSharedRef<SWidget> Content =
      SNew(SBorder)
          .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
          .Padding(0)
          .Cursor(EMouseCursor::GrabHand)
              [SAssignNew(ContentBox, SHorizontalBox)];

  ContentBox->AddSlot()
      .AutoWidth()
          [SNew(SBorder)
               .Padding(4.0f)
               .BorderImage(FdoodleStyle::Create()->GetBrush("doodle.OpenPluginWindow"))];
  ContentBox->AddSlot()
      .HAlign(HAlign_Left)
      .VAlign(VAlign_Center)
          [SNew(STextBlock)
               .Text(Item->DisplayName)];
  TableRowWidget->SetContent(Content);
  return TableRowWidget;
}

FReply SDoodleToolPalette::OnDraggingListViewWidget(const FGeometry& Geo, const FPointerEvent MouseEvent) {
  if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)) {
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(Geo.));
  }

  return FReply::Unhandled();
}
