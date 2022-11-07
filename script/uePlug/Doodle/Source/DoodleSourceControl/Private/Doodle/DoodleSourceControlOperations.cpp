#include "DoodleSourceControlOperations.h"
#include "DoodleSourceControlCommand.h"
#include "SourceControlOperations.h"
#include "DoodleSourceControl.h"
#include "Doodle/DoodleSourceControlState.h"

#define LOCTEXT_NAMESPACE "DoodleSourceControl"

namespace doodle_ue4 {
bool UpdateCachedStates(const TArray<FDoodleSourceControlState>& InStates) {
  FDoodleSourceControlModule& SourceControl = FModuleManager::LoadModuleChecked<FDoodleSourceControlModule>("DoodleSourceControl");
  FDoodleSourceControlProvider& Provider    = SourceControl.GetProvider();
  int NbStatesUpdated                       = 0;

  for (const auto& InState : InStates) {
    TSharedRef<FDoodleSourceControlState, ESPMode::ThreadSafe> State = Provider.GetStateInternal(InState.LocalFilename);
    if (State->WorkingCopyState != InState.WorkingCopyState) {
      State->WorkingCopyState         = InState.WorkingCopyState;
      State->PendingMergeBaseFileHash = InState.PendingMergeBaseFileHash;
      //	State->TimeStamp = InState.TimeStamp; // @todo Bug report: Workaround a bug with the Source Control Module not updating file state after a "Save"
      NbStatesUpdated++;
    }
  }

  return (NbStatesUpdated > 0);
}
}  // namespace doodle_ue4

FName FDoodleConnectWorker::GetName() const {
  return "Connect";
}
bool FDoodleConnectWorker::Execute(class FDoodleSourceControlCommand& InCommand) {
  // todo: 实现
  check(InCommand.Operation->GetName() == GetName());

  InCommand.bCommandSuccessful = FPaths::FileExists(InCommand.PathToRepositoryRoot) &&
                                 FPaths::FileExists(InCommand.PathToRepositoryRoot / "Content");
  if (!InCommand.bCommandSuccessful) {
    StaticCastSharedRef<FConnect>(InCommand.Operation)->SetErrorText(LOCTEXT("NotAGitRepository", "Failed to enable Git source control. You need to initialize the project as a Git repository first."));
  }
  /// @brief 开始获取远程目录中的所有文件
  if (InCommand.bCommandSuccessful) {
  }

  return (bool)InCommand.bCommandSuccessful;
}

bool FDoodleConnectWorker::UpdateStates() const {
  // todo: 实现
  return doodle_ue4::UpdateCachedStates(States);
}

#undef LOCTEXT_NAMESPACE