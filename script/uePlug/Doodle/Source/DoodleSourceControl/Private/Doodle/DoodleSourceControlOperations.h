
#pragma once

#include "CoreMinimal.h"
#include "IDoodleSourceControlWorker.h"
class FDoodleSourceControlCommand;
class FDoodleSourceControlState;

class FDoodleConnectWorker : public IDoodleSourceControlWorker {
 public:
  virtual ~FDoodleConnectWorker() = default;

  virtual FName GetName() const override;
  virtual bool Execute(class FDoodleSourceControlCommand& InCommand) override;

  virtual bool UpdateStates() const override;

 public:
  /// 结果
  TArray<FDoodleSourceControlState> States;
};

/// @todo 还需要实现
class FDoodleCheckInWorker : public IDoodleSourceControlWorker {
 public:
  virtual ~FDoodleCheckInWorker() = default;

  virtual FName GetName() const override;
  virtual bool Execute(class FDoodleSourceControlCommand& InCommand) override;

  virtual bool UpdateStates() const override;

 public:
  /// 结果
  TArray<FDoodleSourceControlState> States;
};

/**
 * @brief 更新文件状态
 *
 */
class FDoodleUpdateStatusWorker : public IDoodleSourceControlWorker {
 public:
  virtual ~FDoodleUpdateStatusWorker() = default;

  virtual FName GetName() const override;
  virtual bool Execute(class FDoodleSourceControlCommand& InCommand) override;

  virtual bool UpdateStates() const override;

 public:
  /// 结果
  TArray<FDoodleSourceControlState> States;
};