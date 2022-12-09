// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IDoodleSourceControlWorker {
 public:
  /**
   * 描述这个名称, 工厂方法使用
   */
  virtual FName GetName() const                                      = 0;

  /**
   * 工作函数, 在后台线程执行
   */
  virtual bool Execute(class FDoodleSourceControlCommand& InCommand) = 0;

  /**
   * 更新项目状态, 在主线程中执行
   * @return true 完成更新
   * @return false 未完成更新
   */
  virtual bool UpdateStates() const                                  = 0;
};

typedef TSharedRef<IDoodleSourceControlWorker, ESPMode::ThreadSafe> FDoodleSourceControlWorkerRef;
