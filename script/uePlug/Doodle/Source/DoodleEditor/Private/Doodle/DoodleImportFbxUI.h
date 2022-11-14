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

struct FAbcImport {
  FAbcImport(){};
  FAbcImport(const FString& InString) : ImportAbcPath(InString) {}
  /// @brief 导入的文件路径
  FString ImportAbcPath;

  int32_t StartTime;
  int32_t EndTime;

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
  /// @brief 导入fbx的列表
  TSharedPtr<class SListView<TSharedPtr<doodle_ue4::FFbxImport>>> ListImportFbx;
  /// @brief 导入fbx的列表数据
  TArray<TSharedPtr<doodle_ue4::FFbxImport>> ListImportFbxData;
  /// @brief 扫描的骨骼数据
  TArray<doodle_ue4::FUSkeletonData> AllSkinObjs;

  /// @brief 导入abc的列表
  TSharedPtr<class SListView<TSharedPtr<doodle_ue4::FAbcImport>>> ListImportAbc;
  /// @brief 导入abc 的列表数据
  TArray<TSharedPtr<doodle_ue4::FAbcImport>> ListImportAbcData;

  /// 导入路径的前缀
  FString Path_Prefix;

  /// @brief 扫描路径(会扫描目录中的fbx和abc文件)
  /// @param in 需要扫描的路径
  void SearchPath(const FString& in);

  /**
   * @brief 获取所有的sk
   *
   */
  void GetAllSkinObjs();
  /**
   * @brief 将sk 和fbx 进行匹配
   *
   */
  void MatchFbx();
  /**
   * @brief 导入fbx
   *
   */
  void ImportFbx();
  /**
   * @brief 导入abc
   *
   */
  void ImportAbc();
  /**
   * @brief 根据导入的路径, 提取信息,  生成要导入ue4的路径
   *
   * @param In_Path 传入的路径(提取信息)
   * @return FString  返回的导入ue4的路径
   */
  FString GetImportPath(const FString& In_Path);
  /**
   * @brief 根据传入的路径生成前缀
   *
   * @param In_Path_Prefix 传入的路径
   */
  void GenPathPrefix(const FString& In_Path_Prefix);

  /**
   * @brief获取文件名称中的开始和结束
   *
   * @param In_Path
   * @return TTuple<int32_t, int32_t>
   */
  TTuple<int32_t, int32_t> GenStartAndEndTime(const FString& In_Path);

  /**
   * @brief 提取所有的标签
   * 暂时是 使用 SK_(\w+)_Skeleton 捕获
   *
   */
  void SetAllSkinTag();
};