#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/GCObject.h"

class DoodleCopyMat : public SCompoundWidget, public FGCObject {
 public:
  SLATE_BEGIN_ARGS(DoodleCopyMat) {}
  SLATE_END_ARGS()
  // 这里是内容创建函数
  void Construct(const FArguments& Arg);
  // 知道了原因, 感觉用不到(垃圾回收)
  virtual void AddReferencedObjects(FReferenceCollector& collector) override;

 private:
  FReply getSelect();
  FReply CopyMateral();

  FReply BathImport();
  FReply BathReameAss();

  bool bEnableSeparateTranslucency;
  FReply set_marteral_deep();

  TArray<FString> OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes);
  FString OpenDirDialog(const FString& DialogTitle, const FString& DefaultPath);
  void set_material_attr(UMaterialInterface* in_mat, const FString& in_SlotName);

 private:
  USkeletalMesh* copySoureSkinObj;
  UObject* copySoureGeoCache;
};
