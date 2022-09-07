#include "SourceControlProvider.h"

#include "Doodle/SDoodleSourceControlSettings.h"

FDoodleSourceControlProvider::FDoodleSourceControlProvider() {
  NameAttr = TEXT("doodle");
}

void FDoodleSourceControlProvider::Init(bool bForceConnection) {
}

void FDoodleSourceControlProvider::Close() {
}

FText FDoodleSourceControlProvider::GetStatusText() const {
  return FText::FromString(TEXT("doodle file"));
}

bool FDoodleSourceControlProvider::IsEnabled() const {
  return false;
}

bool FDoodleSourceControlProvider::IsAvailable() const {
  return false;
}

const FName &FDoodleSourceControlProvider::GetName(void) const {
  return NameAttr;
}

bool FDoodleSourceControlProvider::QueryStateBranchConfig(
    const FString &ConfigSrc,
    const FString &ConfigDest
) {
  return false;
}

void FDoodleSourceControlProvider::RegisterStateBranches(
    const TArray<FString> &BranchNames,
    const FString &ContentRoot
) {
}

int32 FDoodleSourceControlProvider::GetStateBranchIndex(
    const FString &InBranchName
) const {
  return INDEX_NONE;
}

ECommandResult::Type FDoodleSourceControlProvider::GetState(
    const TArray<FString> &InFiles,
    TArray<TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> &OutState,
    EStateCacheUsage::Type InStateCacheUsage
) {
  return ECommandResult::Type::Failed;
}

TArray<FSourceControlStateRef> FDoodleSourceControlProvider::GetCachedStateByPredicate(
    TFunctionRef<bool(const FSourceControlStateRef &)> Predicate
) const {
  return {};
}

FDelegateHandle FDoodleSourceControlProvider::RegisterSourceControlStateChanged_Handle(
    const FSourceControlStateChanged::FDelegate &SourceControlStateChanged
) {
  return {};
}

void FDoodleSourceControlProvider::UnregisterSourceControlStateChanged_Handle(
    FDelegateHandle Handle
) {
}

ECommandResult::Type FDoodleSourceControlProvider::Execute(
    const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation,
    const TArray<FString> &InFiles,
    EConcurrency::Type InConcurrency,
    const FSourceControlOperationComplete &InOperationCompleteDelegate
) {
  return ECommandResult::Type::Failed;
}

bool FDoodleSourceControlProvider::CanCancelOperation(
    const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation
)
    const {
  return false;
}

void FDoodleSourceControlProvider::CancelOperation(
    const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation
) {
}

bool FDoodleSourceControlProvider::UsesLocalReadOnlyState() const {
  return false;
}

bool FDoodleSourceControlProvider::UsesChangelists() const {
  return false;
}

bool FDoodleSourceControlProvider::UsesCheckout() const {
  return false;
}

void FDoodleSourceControlProvider::Tick() {
}

TArray<TSharedRef<class ISourceControlLabel>> FDoodleSourceControlProvider::GetLabels(
    const FString &InMatchingSpec
) const {
  return {};
}

#if SOURCE_CONTROL_WITH_SLATE
TSharedRef<class SWidget> FDoodleSourceControlProvider::MakeSettingsWidget() const {
  return SNew(SDoodleSourceControlSettings);
}

#endif