#pragma once

#include "CoreMinimal.h"
#include "ISourceControlOperation.h"
#include "ISourceControlState.h"
#include "ISourceControlProvider.h"
#include "IDoodleSourceControlWorker.h"

class FDoodleSourceControlState;
class FDoodleSourceControlCommand;

DECLARE_DELEGATE_RetVal(FDoodleSourceControlWorkerRef, FGetDoodleSourceControlWorker)
    /**
     * @brief 这个类是源代码控制的接口
     *
     */
    class FDoodleSourceControlProvider : public ISourceControlProvider {
 public:
  FDoodleSourceControlProvider();
  /* ISourceControlProvider implementation */
  /// 初始化
  virtual void Init(bool bForceConnection = true) override;
  /// @brief 关闭
  virtual void Close() override;
  /// @brief 获取状态 text
  /// @return
  virtual FText GetStatusText() const override;
  /// @brief 是否启用了源码控制
  /// @return
  virtual bool IsEnabled() const override;
  /// @brief 是否激活
  /// @return
  virtual bool IsAvailable() const override;
  virtual const FName &GetName(void) const override;
  virtual bool QueryStateBranchConfig(const FString &ConfigSrc, const FString &ConfigDest) override;
  virtual void RegisterStateBranches(const TArray<FString> &BranchNames, const FString &ContentRoot) override;
  virtual int32 GetStateBranchIndex(const FString &InBranchName) const override;
  virtual ECommandResult::Type GetState(const TArray<FString> &InFiles, TArray<TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> &OutState, EStateCacheUsage::Type InStateCacheUsage) override;
  virtual TArray<FSourceControlStateRef> GetCachedStateByPredicate(TFunctionRef<bool(const FSourceControlStateRef &)> Predicate) const override;
  virtual FDelegateHandle RegisterSourceControlStateChanged_Handle(const FSourceControlStateChanged::FDelegate &SourceControlStateChanged) override;
  virtual void UnregisterSourceControlStateChanged_Handle(FDelegateHandle Handle) override;
  virtual ECommandResult::Type Execute(const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation, const TArray<FString> &InFiles, EConcurrency::Type InConcurrency = EConcurrency::Synchronous, const FSourceControlOperationComplete &InOperationCompleteDelegate = FSourceControlOperationComplete()) override;
  virtual bool CanCancelOperation(const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation) const override;
  virtual void CancelOperation(const TSharedRef<ISourceControlOperation, ESPMode::ThreadSafe> &InOperation) override;
  virtual bool UsesLocalReadOnlyState() const override;
  virtual bool UsesChangelists() const override;
  virtual bool UsesCheckout() const override;
  virtual void Tick() override;
  virtual TArray<TSharedRef<class ISourceControlLabel>> GetLabels(const FString &InMatchingSpec) const override;
#if SOURCE_CONTROL_WITH_SLATE
  virtual TSharedRef<class SWidget> MakeSettingsWidget() const override;
#endif

  /// @brief 获取缓存状态
  /// @param Filename
  /// @return 缓存状态共享指针
  TSharedRef<FDoodleSourceControlState, ESPMode::ThreadSafe> GetStateInternal(const FString &Filename);

  /**
   * @brief 获取同步的根目录
   *
   * @return const FString&
   */
  inline const FString &GetPathToRepositoryRoot() const {
    return PathToRepositoryRoot;
  }

 private:
  /**
   * @brief 创建工作
   *
   * @param InOperationName 传入的操作名称
   * @return TSharedPtr<IGitSourceControlWorker, ESPMode::ThreadSafe>返回工作指针
   */
  TSharedPtr<IDoodleSourceControlWorker, ESPMode::ThreadSafe> CreateWorker(const FName &InOperationName) const;

  /**
   * @brief 执行同步命令
   *
   * @param InCommand 传入的命令
   * @param Task task
   * @return ECommandResult::Type
   */
  ECommandResult::Type ExecuteSynchronousCommand(
      FDoodleSourceControlCommand &InCommand, const FText &Task
  );
  /**
   * @brief 发布命令
   *
   * @param InCommand 传入的命令
   * @return ECommandResult::Type
   */
  ECommandResult::Type IssueCommand(FDoodleSourceControlCommand &InCommand);

  /**
   * @brief 打印log日志
   *
   * @param InCommand
   */
  void OutputCommandMessages(const FDoodleSourceControlCommand &InCommand) const;

 private:
  friend class SDoodleSourceControlSettings;

  FName NameAttr;
  bool bAvailable;
  friend class FDoodleSourceControlModule;

  /// @brief 缓存的文件状态
  TMap<FString, TSharedRef<class FDoodleSourceControlState, ESPMode::ThreadSafe>> StateCache;
  /** For notifying when the source control states in the cache have changed */
  FSourceControlStateChanged OnSourceControlStateChanged;

  /// @brief 命令队列
  TArray<FDoodleSourceControlCommand *> CommandQueue;

  /** The currently registered source control operations */
  TMap<FName, FGetDoodleSourceControlWorker> WorkersMap;
  FString PathToRepositoryRoot;
};