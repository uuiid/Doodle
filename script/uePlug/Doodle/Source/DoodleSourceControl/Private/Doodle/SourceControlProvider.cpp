#include "SourceControlProvider.h"

#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "Logging/MessageLog.h"

#include "Doodle/SDoodleSourceControlSettings.h"
#include "Doodle/DoodleSourceControlState.h"
#include "Doodle/DoodleSourceControlCommand.h"

#define LOCTEXT_NAMESPACE "DoodleSourceControl"

FDoodleSourceControlProvider::FDoodleSourceControlProvider() {
  NameAttr = TEXT("doodle");
}

void FDoodleSourceControlProvider::Init(bool bForceConnection) {
  /// 这里初始化一些同步目录,查看使用存在
  this->bAvailable = true;
}

void FDoodleSourceControlProvider::Close() {
  /// 这里我们是吗都不需要做
}

FText FDoodleSourceControlProvider::GetStatusText() const {
  /// 直接返回一个字符串即可
  return FText::FromString(TEXT("doodle file"));
}

bool FDoodleSourceControlProvider::IsEnabled() const {
  return bAvailable;
}

bool FDoodleSourceControlProvider::IsAvailable() const {
  return bAvailable;
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
  if (!IsEnabled()) {
    return ECommandResult::Failed;
  }

  TArray<FString> AbsoluteFiles = SourceControlHelpers::AbsoluteFilenames(InFiles);
  /// 强制上传文件
  if (InStateCacheUsage == EStateCacheUsage::ForceUpdate) {
    Execute(ISourceControlOperation::Create<FUpdateStatus>(), AbsoluteFiles);
  }

  for (const auto &Files : AbsoluteFiles) {
    OutState.Add(GetStateInternal(Files));
  }

  return ECommandResult::Type::Failed;
}
TSharedRef<FDoodleSourceControlState, ESPMode::ThreadSafe> FDoodleSourceControlProvider::GetStateInternal(const FString &Filename) {
  TSharedRef<FDoodleSourceControlState, ESPMode::ThreadSafe> *State = StateCache.Find(Filename);
  if (State != NULL) {
    // found cached item
    return (*State);
  } else {
    // cache an unknown state for this item
    TSharedRef<FDoodleSourceControlState, ESPMode::ThreadSafe> NewState = MakeShareable(new FDoodleSourceControlState(Filename));
    StateCache.Add(Filename, NewState);
    return NewState;
  }
}

TArray<FSourceControlStateRef> FDoodleSourceControlProvider::GetCachedStateByPredicate(
    TFunctionRef<bool(const FSourceControlStateRef &)> Predicate
) const {
  TArray<FSourceControlStateRef> Result;
  for (const auto &CacheItem : StateCache) {
    FSourceControlStateRef State = CacheItem.Value;
    if (Predicate(State)) {
      Result.Add(State);
    }
  }
  return Result;
}

FDelegateHandle FDoodleSourceControlProvider::RegisterSourceControlStateChanged_Handle(
    const FSourceControlStateChanged::FDelegate &SourceControlStateChanged
) {
  return OnSourceControlStateChanged.Add(SourceControlStateChanged);
}

void FDoodleSourceControlProvider::UnregisterSourceControlStateChanged_Handle(
    FDelegateHandle Handle
) {
  OnSourceControlStateChanged.Remove(Handle);
}

ECommandResult::Type FDoodleSourceControlProvider::Execute(
    const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation,
    const TArray<FString> &InFiles,
    EConcurrency::Type InConcurrency,
    const FSourceControlOperationComplete &InOperationCompleteDelegate
) {
  /// 如果有链接则先失败
  if (!IsEnabled() && !(InOperation->GetName() == "Connect")) {
    // 这个代码永远不会运行
    InOperationCompleteDelegate.ExecuteIfBound(InOperation, ECommandResult::Failed);
    return ECommandResult::Failed;
  }

  TArray<FString> AbsoluteFiles                                      = SourceControlHelpers::AbsoluteFilenames(InFiles);

  // Query to see if we allow this operation
  TSharedPtr<IDoodleSourceControlWorker, ESPMode::ThreadSafe> Worker = CreateWorker(InOperation->GetName());
  if (!Worker.IsValid()) {
    // this operation is unsupported by this source control provider
    FFormatNamedArguments Arguments;
    Arguments.Add(TEXT("OperationName"), FText::FromName(InOperation->GetName()));
    Arguments.Add(TEXT("ProviderName"), FText::FromName(GetName()));
    FText Message(FText::Format(LOCTEXT("UnsupportedOperation", "Operation '{OperationName}' not supported by source control provider '{ProviderName}'"), Arguments));
    FMessageLog("SourceControl").Error(Message);
    InOperation->AddErrorMessge(Message);

    InOperationCompleteDelegate.ExecuteIfBound(InOperation, ECommandResult::Failed);
    return ECommandResult::Failed;
  }

  FDoodleSourceControlCommand *Command = new FDoodleSourceControlCommand(InOperation, Worker.ToSharedRef());
  Command->Files                       = AbsoluteFiles;
  Command->OperationCompleteDelegate   = InOperationCompleteDelegate;

  // fire off operation
  if (InConcurrency == EConcurrency::Synchronous) {
    Command->bAutoDelete = false;
    return ExecuteSynchronousCommand(*Command, InOperation->GetInProgressString());
  } else {
    Command->bAutoDelete = true;
    return IssueCommand(*Command);
  }
}

ECommandResult::Type FDoodleSourceControlProvider::ExecuteSynchronousCommand(
    FDoodleSourceControlCommand &InCommand, const FText &Task
) {
  // TODO: 同步执行命令
}

ECommandResult::Type FDoodleSourceControlProvider::IssueCommand(FDoodleSourceControlCommand &InCommand) {
  // TODO: 发布命令
}

TSharedPtr<IDoodleSourceControlWorker, ESPMode::ThreadSafe> FDoodleSourceControlProvider::CreateWorker(
    const FName &InOperationName
) const {
  /// TODO: 这里我们需呀根据传入的操作名称进行创建

  return nullptr;
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
  ///  TODO: 这里要实现
}

TArray<TSharedRef<class ISourceControlLabel>> FDoodleSourceControlProvider::GetLabels(
    const FString &InMatchingSpec
) const {
  TArray<TSharedRef<ISourceControlLabel>> Tags;

  return Tags;
}

#if SOURCE_CONTROL_WITH_SLATE
TSharedRef<class SWidget> FDoodleSourceControlProvider::MakeSettingsWidget() const {
  return SNew(SDoodleSourceControlSettings);
}

#endif

#undef LOCTEXT_NAMESPACE
