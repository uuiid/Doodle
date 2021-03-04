#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/GCObject.h"

class DoodleCopyMat : public SCompoundWidget, public FGCObject {
 public:
  SLATE_BEGIN_ARGS(DoodleCopyMat) {}
  SLATE_END_ARGS()
  //这里是内容创建函数
  void Construct(const FArguments& Arg);
  //这个暂时不知道什么意思,但是不加就几把编译不过去
  virtual void AddReferencedObjects(FReferenceCollector& collector) override;

 private:
  FReply getSelect();
  FReply CopyMateral();

  FReply BathImport();
  FReply BathReameAss();
  FReply importAbcFile();

  TArray<FString> OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes);
  FString OpenDirDialog(const FString& DialogTitle,
                        const FString& DefaultPath);

 private:
  USkeletalMesh* copySoureSkinObj;
  UObject* copySoureGeoCache;
};