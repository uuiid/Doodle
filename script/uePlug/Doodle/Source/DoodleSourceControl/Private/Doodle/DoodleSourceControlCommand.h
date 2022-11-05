// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISourceControlProvider.h"
#include "Misc/IQueuedWork.h"

#include "IDoodleSourceControlWorker.h"
/**
 * @brief 只要多线程进行同步
 * TODO: 这个类需要实现
 */
class FDoodleSourceControlCommand : public IQueuedWork {
 public:
  FDoodleSourceControlCommand(
      const TSharedRef<class ISourceControlOperation, ESPMode::ThreadSafe>& InOperation,
      const TSharedRef<class IDoodleSourceControlWorker, ESPMode::ThreadSafe>& InWorker,
      const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete()
  );

  /**
   * @brief 这里开始工作
   *
   * @return true
   * @return false
   */
  bool DoWork();

  /**
   * @brief 停止工作时调用
   *
   */
  virtual void Abandon() override;

  /**
   * @brief 清理工作, 如果运行不要进行清理
   *
   */
  virtual void DoThreadedWork() override;

  /**
   * @brief 保存结果发出回调
   *
   * @return ECommandResult::Type
   */
  ECommandResult::Type ReturnResults();

 public:
  TSharedRef<class ISourceControlOperation, ESPMode::ThreadSafe> Operation;

  TSharedRef<class IDoodleSourceControlWorker, ESPMode::ThreadSafe> Worker;
  FSourceControlOperationComplete OperationCompleteDelegate;

  TArray<FString> Files;
  /// @brief 自动删除命令
  bool bAutoDelete;

  /// 已经在执行
  bool bExecuteProcessed;

  /// 执行成功?
  bool bCommandSuccessful;
};