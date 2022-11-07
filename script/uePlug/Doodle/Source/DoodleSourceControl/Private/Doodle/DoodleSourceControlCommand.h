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
  /// @brief 源代码管理选项
  TSharedRef<class ISourceControlOperation, ESPMode::ThreadSafe> Operation;
  /// @brief 工作线程
  TSharedRef<class IDoodleSourceControlWorker, ESPMode::ThreadSafe> Worker;
  /// @brief 完成后委托
  FSourceControlOperationComplete OperationCompleteDelegate;
  /// @brief 文件列表
  TArray<FString> Files;
  /// @brief 自动删除命令
  bool bAutoDelete;
  /// 由源代码管理线程执行
  volatile int32 bExecuteProcessed;
  /// @brief 命令成功
  int32 bCommandSuccessful;
  /// 信息消息
  TArray<FString> InfoMessages;
  /// 错误消息
  TArray<FString> ErrorMessages;
  /// @brief 运行方式, 同步或者异步
  EConcurrency::Type Concurrency;
  /// @brief 主要的项目根路径
  FString PathToRepositoryRoot;
};