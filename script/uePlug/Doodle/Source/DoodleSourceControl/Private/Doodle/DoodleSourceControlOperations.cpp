#include "DoodleSourceControlOperations.h"
#include "DoodleSourceControlCommand.h"
#include "SourceControlOperations.h"
#include "DoodleSourceControl.h"
#include "Doodle/DoodleSourceControlState.h"

// 文件信息平台包装
#include "HAL/FileManager.h"

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
      // State->TimeStamp                = InState.TimeStamp;
      NbStatesUpdated++;
    }
  }

  return (NbStatesUpdated > 0);
}

bool CompareFiles(const FString& InLocal, const FString& InOrigin, FDoodleSourceControlState& OutState) {
  /**
   *           |   本地     |    远端   |  本地时间戳  |     远端时间戳   |
   *-----------------------------------------------------------------------------------------
   * 文件状态  |   不存在   |   存在    |     无       |        有        |      需要下载(丢失)
   * 文件状态  |   存在     |   不存在  |     有       |        无        |      需要上传(添加后)
   *
   * 文件状态  |   存在     |   存在    |     新       |        旧        |      需要上传(修改)
   * 文件状态  |   存在     |   存在    |     旧       |        新        |      需要下载
   *
   */
  bool LL_Exit = FPaths::FileExists(InLocal);
  bool LO_Exit = FPaths::FileExists(InOrigin);

  if (LL_Exit && !LO_Exit) {
    OutState.WorkingCopyState = doodle_ue4::WorkingCopyState_Type::NeedDown;
    return true;
  } else if (!LL_Exit && LO_Exit) {
    OutState.WorkingCopyState = doodle_ue4::WorkingCopyState_Type::Added;
    return true;
  }

  if (LL_Exit && LO_Exit) {
    FDateTime LL_Time = IFileManager::Get().GetTimeStamp(*InLocal);
    FDateTime LO_Time = IFileManager::Get().GetTimeStamp(*InOrigin);

    if (LL_Time == LO_Time) {
      OutState.WorkingCopyState = doodle_ue4::WorkingCopyState_Type::Unchanged;
    } else if (LL_Time < LO_Time) {
      OutState.WorkingCopyState = doodle_ue4::WorkingCopyState_Type::NeedDown;
    } else {
      OutState.WorkingCopyState = doodle_ue4::WorkingCopyState_Type::Modified;
    }
  }
  return true;
};

}  // namespace doodle_ue4

FName FDoodleConnectWorker::GetName() const {
  return "Connect";
}
bool FDoodleConnectWorker::Execute(class FDoodleSourceControlCommand& InCommand) {
  check(InCommand.Operation->GetName() == GetName());

  InCommand.bCommandSuccessful = !InCommand.PathToRepositoryRoot.IsEmpty() &&
                                 IFileManager::Get().DirectoryExists(*InCommand.PathToRepositoryRoot) &&
                                 IFileManager::Get().DirectoryExists(*(InCommand.PathToRepositoryRoot / "Content"));

  if (!InCommand.bCommandSuccessful) {
    StaticCastSharedRef<FConnect>(InCommand.Operation)->SetErrorText(LOCTEXT("NotUe4Project", "The content folder for the target item was not found"));
  }
  /// @brief 开始获取远程目录中的所有文件
  /// todo: 这里要获取远程的所有文件状态
  if (InCommand.bCommandSuccessful) {
  }

  return (bool)InCommand.bCommandSuccessful;
}

bool FDoodleConnectWorker::UpdateStates() const {
  return doodle_ue4::UpdateCachedStates(States);
}

FName FDoodleCheckInWorker::GetName() const {
  return "CheckIn";
}
bool FDoodleCheckInWorker::Execute(class FDoodleSourceControlCommand& InCommand) {
  check(InCommand.Operation->GetName() == GetName());

  TSharedRef<FCheckIn, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FCheckIn>(InCommand.Operation);

  /// todo: 需要实现
  return {};
}
bool FDoodleCheckInWorker::UpdateStates() const {
  return doodle_ue4::UpdateCachedStates(States);
}

FName FDoodleUpdateStatusWorker::GetName() const {
  return "UpdateStatus";
}
bool FDoodleUpdateStatusWorker::Execute(class FDoodleSourceControlCommand& InCommand) {
  check(InCommand.Operation->GetName() == GetName());

  TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FUpdateStatus>(InCommand.Operation);

  /// todo: 实现
  if (InCommand.Files.Num() > 0) {
    FString LProject = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

    for (const FString& LFile : InCommand.Files) {
      FString L_Dis = LFile;
      L_Dis.RemoveFromStart(LProject);
      L_Dis                             = InCommand.PathToRepositoryRoot / L_Dis;
      FDoodleSourceControlState& LState = States.Emplace_GetRef(FDoodleSourceControlState{LFile});
      doodle_ue4::CompareFiles(LFile, L_Dis, LState);
    }
  } else {
  }
  return {};
}
bool FDoodleUpdateStatusWorker::UpdateStates() const {
  return doodle_ue4::UpdateCachedStates(States);
}

#undef LOCTEXT_NAMESPACE