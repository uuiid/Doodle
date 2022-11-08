#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/GCObject.h"

namespace doodle_ue4 {
struct FFbxImport {
 public:
  FString ImportFbxPath;
  USkeleton* SkinObj;
};

}  // namespace doodle_ue4

class SDoodleImportFbxUI : public SCompoundWidget, FGCObject {
 public:
  SLATE_BEGIN_ARGS(SDoodleImportFbxUI) {}
  SLATE_END_ARGS()

  // 这里是内容创建函数
  void Construct(const FArguments& Arg);

  // 知道了原因, 感觉用不到(垃圾回收)
  virtual void AddReferencedObjects(FReferenceCollector& collector) override;

  const static FName Name;

  static void CreateDoodleUI(FMenuBuilder& MenuBuilder);
  static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

 private:
  TSharedPtr<class SListView<TSharedPtr<doodle_ue4::FFbxImport>>> ListImportFbx;
};