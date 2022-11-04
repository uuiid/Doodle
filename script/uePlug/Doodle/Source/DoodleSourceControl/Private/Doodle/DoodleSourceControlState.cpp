#include "DoodleSourceControlState.h"

int32 FDoodleSourceControlState::GetHistorySize() const {}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::GetHistoryItem(int32 HistoryIndex) const {}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::FindHistoryRevision(int32 RevisionNumber) const {}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::FindHistoryRevision(const FString& InRevision) const {}
TSharedPtr<class ISourceControlRevision, ESPMode::ThreadSafe> FDoodleSourceControlState::GetBaseRevForMerge() const {}
FName FDoodleSourceControlState::GetIconName() const {}
FName FDoodleSourceControlState::GetSmallIconName() const {}
FText FDoodleSourceControlState::GetDisplayName() const {}
FText FDoodleSourceControlState::GetDisplayTooltip() const {}
const FString& FDoodleSourceControlState::GetFilename() const {}
const FDateTime& FDoodleSourceControlState::GetTimeStamp() const {}
bool FDoodleSourceControlState::CanCheckIn() const {}
bool FDoodleSourceControlState::CanCheckout() const {}
bool FDoodleSourceControlState::IsCheckedOut() const {}
bool FDoodleSourceControlState::IsCheckedOutOther(FString* Who = nullptr) const {}
bool FDoodleSourceControlState::IsCurrent() const {}
bool FDoodleSourceControlState::IsSourceControlled() const {}
bool FDoodleSourceControlState::IsAdded() const {}
bool FDoodleSourceControlState::IsDeleted() const {}
bool FDoodleSourceControlState::IsIgnored() const {}
bool FDoodleSourceControlState::CanEdit() const {}
bool FDoodleSourceControlState::IsUnknown() const {}
bool FDoodleSourceControlState::IsModified() const {}
bool FDoodleSourceControlState::CanAdd() const {}
bool FDoodleSourceControlState::CanDelete() const {}
bool FDoodleSourceControlState::IsConflicted() const {}
bool FDoodleSourceControlState::CanRevert() const {}