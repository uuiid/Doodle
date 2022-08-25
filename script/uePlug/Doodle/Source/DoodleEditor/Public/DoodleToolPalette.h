#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
// #include "Misc/NotifyHook.h"
#include "ActorFactories/ActorFactory.h"

class SDoodleToolListItem {
 public:
  UActorFactory* Factory;
  FAssetData AssetData;
  TOptional<int32> SortOrder;

  /** This item's display name */
  FText DisplayName;

  SDoodleToolListItem(UActorFactory* InFactory, const FAssetData& InAssetData, TOptional<int32> InSortOrder = TOptional<int32>())
      : Factory(InFactory), AssetData(InAssetData), SortOrder(InSortOrder) {
    UClass* Class = AssetData.GetClass() == UClass::StaticClass() ? Cast<UClass>(AssetData.GetAsset()) : nullptr;

    if (Factory) {
      DisplayName = Factory->GetDisplayName();
    } else if (Class) {
      DisplayName = Class->GetDisplayNameText();
    } else {
      DisplayName = FText::FromName(AssetData.AssetName);
    }
  };
};

// #include "DoodleToolPalette.h"
// class SDoodleToolListView :public {
//
// };

class SDoodleToolPalette : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SDoodleToolPalette) {}
  SLATE_END_ARGS();

  void Construct(const FArguments& InArgs);

 private:
  // 创建项目函数
  TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<SDoodleToolListItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

  // 拖拽函数
  FReply OnDraggingListViewWidget(const FGeometry& Geo, const FPointerEvent MouseEvent);

  TSharedPtr<SListView<TSharedPtr<SDoodleToolListItem> > > DoodleClassList;
  TArray<TSharedPtr<SDoodleToolListItem> > p_list;
};
