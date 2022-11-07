#include "DoodleSourceControlState.h"

#define LOCTEXT_NAMESPACE "DoodleSourceControl.State"

int32 FDoodleSourceControlState::GetHistorySize() const {
  return 0;
}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::GetHistoryItem(int32 HistoryIndex) const {
  return {};
}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::FindHistoryRevision(int32 RevisionNumber) const {
  return {};
}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::FindHistoryRevision(const FString& InRevision) const {
  return {};
}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::GetBaseRevForMerge() const {
  return {};
}
FName FDoodleSourceControlState::GetIconName() const {
  switch (WorkingCopyState) {
    case doodle_ue4::WorkingCopyState_Type::Modified:
      return FName("Subversion.CheckedOut");
    case doodle_ue4::WorkingCopyState_Type::Added:
      return FName("Subversion.OpenForAdd");
    case doodle_ue4::WorkingCopyState_Type::Renamed:
    case doodle_ue4::WorkingCopyState_Type::Copied:
      return FName("Subversion.Branched");
    case doodle_ue4::WorkingCopyState_Type::Deleted:  // Deleted & Missing files does not show in Content Browser
    case doodle_ue4::WorkingCopyState_Type::Missing:
      return FName("Subversion.MarkedForDelete");
    case doodle_ue4::WorkingCopyState_Type::Conflicted:
      return FName("Subversion.NotAtHeadRevision");
    case doodle_ue4::WorkingCopyState_Type::NotControlled:
      return FName("Subversion.NotInDepot");
    case doodle_ue4::WorkingCopyState_Type::Unknown:
    case doodle_ue4::WorkingCopyState_Type::Unchanged:  // Unchanged is the same as "Pristine" (not checked out) for Perforce, ie no icon
    case doodle_ue4::WorkingCopyState_Type::Ignored:
    default:
      return NAME_None;
  }

  return NAME_None;
}
FName FDoodleSourceControlState::GetSmallIconName() const {
  switch (WorkingCopyState) {
    case doodle_ue4::WorkingCopyState_Type::Modified:
      return FName("Subversion.CheckedOut_Small");
    case doodle_ue4::WorkingCopyState_Type::Added:
      return FName("Subversion.OpenForAdd_Small");
    case doodle_ue4::WorkingCopyState_Type::Renamed:
    case doodle_ue4::WorkingCopyState_Type::Copied:
      return FName("Subversion.Branched_Small");
    case doodle_ue4::WorkingCopyState_Type::Deleted:  // Deleted & Missing files can appear in the Submit to Source Control window
    case doodle_ue4::WorkingCopyState_Type::Missing:
      return FName("Subversion.MarkedForDelete_Small");
    case doodle_ue4::WorkingCopyState_Type::Conflicted:
      return FName("Subversion.NotAtHeadRevision_Small");
    case doodle_ue4::WorkingCopyState_Type::NotControlled:
      return FName("Subversion.NotInDepot_Small");
    case doodle_ue4::WorkingCopyState_Type::Unknown:
    case doodle_ue4::WorkingCopyState_Type::Unchanged:  // Unchanged is the same as "Pristine" (not checked out) for Perforce, ie no icon
    case doodle_ue4::WorkingCopyState_Type::Ignored:
    default:
      return NAME_None;
  }

  return NAME_None;
}
FText FDoodleSourceControlState::GetDisplayName() const {
  switch (WorkingCopyState) {
    case doodle_ue4::WorkingCopyState_Type::Unknown:
      return LOCTEXT("Unknown", "Unknown");
    case doodle_ue4::WorkingCopyState_Type::Unchanged:
      return LOCTEXT("Unchanged", "Unchanged");
    case doodle_ue4::WorkingCopyState_Type::Added:
      return LOCTEXT("Added", "Added");
    case doodle_ue4::WorkingCopyState_Type::Deleted:
      return LOCTEXT("Deleted", "Deleted");
    case doodle_ue4::WorkingCopyState_Type::Modified:
      return LOCTEXT("Modified", "Modified");
    case doodle_ue4::WorkingCopyState_Type::Renamed:
      return LOCTEXT("Renamed", "Renamed");
    case doodle_ue4::WorkingCopyState_Type::Copied:
      return LOCTEXT("Copied", "Copied");
    case doodle_ue4::WorkingCopyState_Type::Conflicted:
      return LOCTEXT("ContentsConflict", "Contents Conflict");
    case doodle_ue4::WorkingCopyState_Type::Ignored:
      return LOCTEXT("Ignored", "Ignored");
    case doodle_ue4::WorkingCopyState_Type::NotControlled:
      return LOCTEXT("NotControlled", "Not Under Source Control");
    case doodle_ue4::WorkingCopyState_Type::Missing:
      return LOCTEXT("Missing", "Missing");
  }

  return FText();
}
FText FDoodleSourceControlState::GetDisplayTooltip() const {
  switch (WorkingCopyState) {
    case doodle_ue4::WorkingCopyState_Type::Unknown:
      return LOCTEXT("Unknown_Tooltip", "Unknown source control state");
    case doodle_ue4::WorkingCopyState_Type::Unchanged:
      return LOCTEXT("Pristine_Tooltip", "There are no modifications");
    case doodle_ue4::WorkingCopyState_Type::Added:
      return LOCTEXT("Added_Tooltip", "Item is scheduled for addition");
    case doodle_ue4::WorkingCopyState_Type::Deleted:
      return LOCTEXT("Deleted_Tooltip", "Item is scheduled for deletion");
    case doodle_ue4::WorkingCopyState_Type::Modified:
      return LOCTEXT("Modified_Tooltip", "Item has been modified");
    case doodle_ue4::WorkingCopyState_Type::Renamed:
      return LOCTEXT("Renamed_Tooltip", "Item has been renamed");
    case doodle_ue4::WorkingCopyState_Type::Copied:
      return LOCTEXT("Copied_Tooltip", "Item has been copied");
    case doodle_ue4::WorkingCopyState_Type::Conflicted:
      return LOCTEXT("ContentsConflict_Tooltip", "The contents of the item conflict with updates received from the repository.");
    case doodle_ue4::WorkingCopyState_Type::Ignored:
      return LOCTEXT("Ignored_Tooltip", "Item is being ignored.");
    case doodle_ue4::WorkingCopyState_Type::NotControlled:
      return LOCTEXT("NotControlled_Tooltip", "Item is not under version control.");
    case doodle_ue4::WorkingCopyState_Type::Missing:
      return LOCTEXT("Missing_Tooltip", "Item is missing (e.g., you moved or deleted it without using Git). This also indicates that a directory is incomplete (a checkout or update was interrupted).");
  }

  return FText();
}
const FString& FDoodleSourceControlState::GetFilename() const {
  return LocalFilename;
}
const FDateTime& FDoodleSourceControlState::GetTimeStamp() const {
  return TimeStamp;
}

// 已删除和丢失的资产不能出现在内容浏览器中，但要在“将文件提交到源代码管理”窗口中！
bool FDoodleSourceControlState::CanCheckIn() const {
  return WorkingCopyState == doodle_ue4::WorkingCopyState_Type::Added ||
         WorkingCopyState == doodle_ue4::WorkingCopyState_Type::Deleted ||
         WorkingCopyState == doodle_ue4::WorkingCopyState_Type::Missing ||
         WorkingCopyState == doodle_ue4::WorkingCopyState_Type::Modified ||
         WorkingCopyState == doodle_ue4::WorkingCopyState_Type::Renamed;
}
// 这里我们是所有的都是未迁出
bool FDoodleSourceControlState::CanCheckout() const {
  // TODO: 实现
  return {};
}

bool FDoodleSourceControlState::IsCheckedOut() const {
  // TODO: 实现
  return {};
}

bool FDoodleSourceControlState::IsCheckedOutOther(FString* Who) const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsCurrent() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsSourceControlled() const {
  // TODO: 实现
  return true;
}
bool FDoodleSourceControlState::IsAdded() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsDeleted() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsIgnored() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::CanEdit() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsUnknown() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsModified() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::CanAdd() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::CanDelete() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::IsConflicted() const {
  // TODO: 实现
  return {};
}
bool FDoodleSourceControlState::CanRevert() const {
  return CanCheckIn();
}

#undef LOCTEXT_NAMESPACE