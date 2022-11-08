#include "DoodleSourceControlCommand.h"

#include "DoodleSourceControl.h"
#include "DoodleSourceControl.h"

FDoodleSourceControlCommand::FDoodleSourceControlCommand(
    const TSharedRef<class ISourceControlOperation, ESPMode::ThreadSafe>& InOperation,
    const TSharedRef<class IDoodleSourceControlWorker, ESPMode::ThreadSafe>& InWorker,
    const FSourceControlOperationComplete& InOperationCompleteDelegate
) : Operation(InOperation), Worker(InWorker), OperationCompleteDelegate(InOperationCompleteDelegate) {
  check(IsInGameThread());
  FDoodleSourceControlModule& LModle = FModuleManager::LoadModuleChecked<FDoodleSourceControlModule>("DoodleSourceControl");
  PathToRepositoryRoot               = LModle.GetProvider().GetPathToRepositoryRoot();
  ;
}

bool FDoodleSourceControlCommand::DoWork() {
  bCommandSuccessful = Worker->Execute(*this);
  /// 这个要原子更新, 防止多线程
  FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);
  return {};
}

void FDoodleSourceControlCommand::Abandon() {
  FPlatformAtomics::InterlockedExchange(&bExecuteProcessed, 1);
}

void FDoodleSourceControlCommand::DoThreadedWork() {
  Concurrency = EConcurrency::Asynchronous;
  DoWork();
}

ECommandResult::Type FDoodleSourceControlCommand::ReturnResults() {
  // 转发所有的消息
  for (FString& String : InfoMessages) {
    Operation->AddInfoMessge(FText::FromString(String));
  }
  for (FString& String : ErrorMessages) {
    Operation->AddErrorMessge(FText::FromString(String));
  }

  // 确认是否完成
  ECommandResult::Type Result = bCommandSuccessful ? ECommandResult::Succeeded : ECommandResult::Failed;
  // 完成委托
  OperationCompleteDelegate.ExecuteIfBound(Operation, Result);
  return Result;
}