
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