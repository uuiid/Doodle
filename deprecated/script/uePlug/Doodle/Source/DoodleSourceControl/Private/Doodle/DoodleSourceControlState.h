// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlState.h"
#include "ISourceControlRevision.h"

namespace doodle_ue4 {
enum class WorkingCopyState_Type {
  Unknown,        // 未知状态
  Unchanged,      // 未更改状态
  Added,          // 添加后
  Deleted,        // 删除
  Modified,       // 修改
  Renamed,        // 重命名
  Copied,         // 已复制
  Missing,        // 丢失
  Conflicted,     // 冲突
  NotControlled,  // 不受控制
  Ignored,        // 忽略
  NeedDown,       // 需要下载
  
};
}
/// @brief 表示文件状态
/// TODO: 这个类需要写, 完全没写
class FDoodleSourceControlState : public ISourceControlState, public TSharedFromThis<FDoodleSourceControlState, ESPMode::ThreadSafe> {
 public:
  FDoodleSourceControlState(const FString& InLocalFilename)
      : LocalFilename(InLocalFilename),
        WorkingCopyState(doodle_ue4::WorkingCopyState_Type::Unknown),
        TimeStamp(0){};

  /** ISourceControlState interface */
  virtual int32 GetHistorySize() const override;
  virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetHistoryItem(int32 HistoryIndex) const override;
  virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision(int32 RevisionNumber) const override;
  virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FindHistoryRevision(const FString& InRevision) const override;
  /// @brief 在执行三工资合并时获取我们应该用作基础的修订版，不刷新源代码控制状态
  /// @return
  virtual TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> GetBaseRevForMerge() const override;
  virtual FName GetIconName() const override;
  virtual FName GetSmallIconName() const override;
  virtual FText GetDisplayName() const override;
  virtual FText GetDisplayTooltip() const override;
  virtual const FString& GetFilename() const override;
  virtual const FDateTime& GetTimeStamp() const override;
  virtual bool CanCheckIn() const override;
  virtual bool CanCheckout() const override;
  virtual bool IsCheckedOut() const override;
  /// 获取当前分支中的其他人是否签出此文件
  virtual bool IsCheckedOutOther(FString* Who = nullptr) const override;
  /// 获取此文件是否在不同的分支中检出，如果没有指定分支默认为 FEngineVerion 当前分支
  virtual bool IsCheckedOutInOtherBranch(const FString& CurrentBranch = FString()) const override { return false; }
  /// @brief  如果没有指定分支，则获取此文件是否在其他分支中修改，默认值为 FEngineVerion 当前分支
  virtual bool IsModifiedInOtherBranch(const FString& CurrentBranch = FString()) const override { return false; }
  /// @brief 获取此文件是否在其他分支中签出或修改，如果未指定分支，则默认为FEngineVerion当前分支
  virtual bool IsCheckedOutOrModifiedInOtherBranch(const FString& CurrentBranch = FString()) const override { return IsCheckedOutInOtherBranch(CurrentBranch) || IsModifiedInOtherBranch(CurrentBranch); }
  virtual TArray<FString> GetCheckedOutBranches() const override { return TArray<FString>(); }
  virtual FString GetOtherUserBranchCheckedOuts() const override { return FString(); }
  virtual bool GetOtherBranchHeadModification(FString& HeadBranchOut, FString& ActionOut, int32& HeadChangeListOut) const override { return false; }
  /// 获取此文件是否与源代码管理中的版本保持同步
  virtual bool IsCurrent() const override;
  /// @brief 获取此文件是否在源代码管理下
  virtual bool IsSourceControlled() const override;
  /// @brief 获取此文件是否标记为添加
  virtual bool IsAdded() const override;
  /// @brief 获取此文件是否标记为删除
  virtual bool IsDeleted() const override;
  /// @brief 获取源代码管理是否忽略此文件
  virtual bool IsIgnored() const override;
  /// @brief 获取源代码管理是否允许编辑此文件
  virtual bool CanEdit() const override;
  /// @brief 获取我们是否知道有关此文件源代码控制状态的任何信息
  virtual bool IsUnknown() const override;
  /// @brief 获取与我们从源代码管理中获得的版本相比，此文件是否被修改
  virtual bool IsModified() const override;
  /// @brief 获取此文件是否可以添加到源代码管理中（即是当前处于源代码管理下的目录结构的一部分）
  virtual bool CanAdd() const override;
  /// @brief 获取源代码管理是否允许删除此文件。
  virtual bool CanDelete() const override;
  /// @brief 获取此文件是否处于冲突状态
  virtual bool IsConflicted() const override;
  /// @brief 获取此文件是否可以还原，即丢弃其更改并且不再签出该文件。
  virtual bool CanRevert() const override;

 public:
  /** History of the item, if any */
  //   TGitSourceControlHistory History;

  /// @brief 文件路径
  FString LocalFilename;

  /** File Id with which our local revision diverged from the remote revision */
  /// @brief 文件hash
  FString PendingMergeBaseFileHash;

  /// @brief 文件状态
  doodle_ue4::WorkingCopyState_Type WorkingCopyState;

  /// @brief 时间戳
  FDateTime TimeStamp;
};