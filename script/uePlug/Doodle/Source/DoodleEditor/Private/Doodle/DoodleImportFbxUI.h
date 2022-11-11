#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/GCObject.h"

namespace doodle_ue4 {
struct FFbxImport {
 public:
  FFbxImport(){};
  FFbxImport(const FString& InString) : ImportFbxPath(InString), SkinObj() {}
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

  // 垃圾回收
  virtual void AddReferencedObjects(FReferenceCollector& collector) override;

  const static FName Name;

  static TSharedRef<SDockTab> OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs);

 private:
  TSharedPtr<class SListView<TSharedPtr<doodle_ue4::FFbxImport>>> ListImportFbx;
  TArray<TSharedPtr<doodle_ue4::FFbxImport>> ListImportFbxData;

  TArray<USkeleton*> AllSkinObjs;
  TMap<FString, USkeleton*> AllSkinObjs_Map;

  void SearchPath(const FString& in);

  void GetAllSkinObjs();
  void MatchFbx();
};