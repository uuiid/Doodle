#include "SourceControlProvider.h"

#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "Logging/MessageLog.h"
#include "ScopedSourceControlProgress.h"

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
  /// 同步执行命令

  ECommandResult::Type Result = ECommandResult::Failed;

  // 提供了字符串则显示进度
  {
    FScopedSourceControlProgress Progress(Task);

    // 异步发出命令
    IssueCommand(InCommand);

    // 等待完成, 使同步
    while (!InCommand.bExecuteProcessed) {
      // 更新进度
      Tick();

      Progress.Tick();

      // 停止一会
      FPlatformProcess::Sleep(0.01f);
    }

    // 最后执行一次确保清理
    Tick();

    if (InCommand.bCommandSuccessful) {
      Result = ECommandResult::Succeeded;
    }
  }

  // 立即删除命令, 异步的话在 task 中删除
  check(!InCommand.bAutoDelete);

  // 确认删除的命令没有在队列中
  if (CommandQueue.Contains(&InCommand)) {
    CommandQueue.Remove(&InCommand);
  }
  delete &InCommand;

  return Result;
}

ECommandResult::Type FDoodleSourceControlProvider::IssueCommand(FDoodleSourceControlCommand &InCommand) {
  // 发布命令

  if (GThreadPool != nullptr) {
    // 排队到全局线程池中开始运行
    GThreadPool->AddQueuedWork(&InCommand);
    CommandQueue.Add(&InCommand);
    return ECommandResult::Succeeded;
  } else {
    FText Message(LOCTEXT("NoSCCThreads", "There are no threads available to process the source control command."));

    FMessageLog("SourceControl").Error(Message);
    InCommand.bCommandSuccessful = false;
    InCommand.Operation->AddErrorMessge(Message);

    return InCommand.ReturnResults();
  }
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

  bool bStatesUpdated = false;
  for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex) {
    FDoodleSourceControlCommand &Command = *CommandQueue[CommandIndex];
    if (Command.bExecuteProcessed) {
      // 移出队列
      CommandQueue.RemoveAt(CommandIndex);

      // 更新文件状态
      bStatesUpdated |= Command.Worker->UpdateStates();

      // dump any messages to output log
      // OutputCommandMessages(Command);

      Command.ReturnResults();

      // commands that are left in the array during a tick need to be deleted
      if (Command.bAutoDelete) {
        // Only delete commands that are not running 'synchronously'
        delete &Command;
      }

      // only do one command per tick loop, as we dont want concurrent modification
      // of the command queue (which can happen in the completion delegate)
      break;
    }
  }

  if (bStatesUpdated) {
    OnSourceControlStateChanged.Broadcast();
  }
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
