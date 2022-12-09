#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Doodle/SourceControlProvider.h"

// #include "DoodleSourceControl.generated.h"

/// 这个类是模块的开始, 注册我们自己的源码控制
class FDoodleSourceControlModule : public IModuleInterface {
 public:
  /** IModuleInterface implementation */
  /**
   * 模块注册
   */
  virtual void StartupModule() override;
  /**
   * @brief 取消注册
   *
   */
  virtual void ShutdownModule() override;

  
  FDoodleSourceControlProvider& GetProvider();

 private:
  /// @brief 实现类
  FDoodleSourceControlProvider SourceControlProvider;
};
