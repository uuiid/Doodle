#pragma once

// clang-format off
#include "UObject/GCObject.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
// clang-format on

// struct FDoodleRemoteRenderUIData {};
//
// class SDoodleRemoteRenderUI : public SCompoundWidget, FGCObject {
//  public:
//   SLATE_BEGIN_ARGS(SDoodleRemoteRenderUI) {}
//   SLATE_END_ARGS()
//
//   // 这里是内容创建函数
//   void Construct(const FArguments& Arg);
//
//   // 垃圾回收
//   virtual void AddReferencedObjects(FReferenceCollector& collector) override;
//
//  private:
//   TSharedPtr<class SListView<TSharedPtr<FDoodleRemoteRenderUIData>>> ListView;
//   TArray<TSharedPtr<FDoodleRemoteRenderUIData>> ListViewData;
// };