// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlState.h"
#include "ISourceControlRevision.h"

namespace doodle_ue4 {
enum class WorkingCopyState_Type {
  Unknown,
  Unchanged,  // called "clean" in SVN, "Pristine" in Perforce
  Added,
  Deleted,
  Modified,
  Renamed,
  Copied,
  Missing,
  Conflicted,
  NotControlled,
  Ignored,
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
  virtual bool IsCheckedOutOther(FString* Who = nullptr) const override;
  virtual bool IsCheckedOutInOtherBranch(const FString& CurrentBranch = FString()) const override { return false; }
  virtual bool IsModifiedInOtherBranch(const FString& CurrentBranch = FString()) const override { return false; }
  virtual bool IsCheckedOutOrModifiedInOtherBranch(const FString& CurrentBranch = FString()) const override { return IsCheckedOutInOtherBranch(CurrentBranch) || IsModifiedInOtherBranch(CurrentBranch); }
  virtual TArray<FString> GetCheckedOutBranches() const override { return TArray<FString>(); }
  virtual FString GetOtherUserBranchCheckedOuts() const override { return FString(); }
  virtual bool GetOtherBranchHeadModification(FString& HeadBranchOut, FString& ActionOut, int32& HeadChangeListOut) const override { return false; }
  virtual bool IsCurrent() const override;
  virtual bool IsSourceControlled() const override;
  virtual bool IsAdded() const override;
  virtual bool IsDeleted() const override;
  virtual bool IsIgnored() const override;
  virtual bool CanEdit() const override;
  virtual bool IsUnknown() const override;
  virtual bool IsModified() const override;
  virtual bool CanAdd() const override;
  virtual bool CanDelete() const override;
  virtual bool IsConflicted() const override;
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