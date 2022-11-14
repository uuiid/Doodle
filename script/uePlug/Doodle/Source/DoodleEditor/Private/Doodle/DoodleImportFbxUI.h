#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/GCObject.h"

namespace doodle_ue4 {
struct FFbxImport {
 public:
  FFbxImport(){};
  FFbxImport(const FString& InString) : ImportFbxPath(InString), SkinObj() {}
  /// @brief 导入的文件路径
  FString ImportFbxPath;
  /// @brief fbx文件中包含的节点
  TSet<FString> FbxNodeNames;
  /// @brief 寻找到的骨骼
  USkeleton* SkinObj;
  /// @brief fbx 导入后的路径
  FString ImportPathDir;
};

struct FUSkeletonData {
 public:
  FUSkeletonData(){};
  FUSkeletonData(const TSet<FString>& InString, USkeleton* InSkin)
      : BoneNames(InString), SkinObj(InSkin) {}
  TSet<FString> BoneNames;
  USkeleton* SkinObj;
  FString SkinTag;
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

  TArray<doodle_ue4::FUSkeletonData> AllSkinObjs;
  void SearchPath(const FString& in);

  void GetAllSkinObjs();
  void MatchFbx();
  void ImportFbx();
  FString GetImportPath(const FString& In_Path);
  void GenPathPrefix(const FString& In_Path_Prefix);

  /**
   * @brief 提取所有的标签
   * 暂时是 使用 SK_(\w+)_Skeleton 捕获
   *
   */
  void SetAllSkinTag();

  /// 导入路径的前缀
  FString Path_Prefix;
};