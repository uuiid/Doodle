// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "MoviePipelineLinearExecutor.h"
#include "MovieRemoteExecutor.generated.h"

USTRUCT()
struct FDoodleRemoteRenderJobArg {
  GENERATED_BODY();

  UPROPERTY()
  FString ProjectPath;

  UPROPERTY()
  FString Args;

  UPROPERTY()
  FString ManifestValue;

  UPROPERTY()
  FString OutFilePath;
};

// struct FDoodleRemoteRenderState {
//
//
// };

UENUM()
enum class DoodleMovieRemoteState {
  Wait UMETA(DisplayName = "Wait"),
  Render UMETA(DisplayName = "Render"),
  Success UMETA(DisplayName = "Success"),
  Fail UMETA(DisplayName = "Fail"),
};

/**
 * This is the implementation responsible for executing the rendering of
 * multiple movie pipelines on the local machine in an external process.
 * This simply handles launching and managing the external processes and
 * acts as a proxy to them where possible. This internally uses the
 * UMoviePipelineInProcessExecutor on the launched instances.
 */
UCLASS(Blueprintable)
class UDoodleMovieRemoteExecutor : public UMoviePipelineExecutorBase {
  GENERATED_BODY()

  // UMoviePipelineExecutorBase Interface
  virtual void Execute_Implementation(UMoviePipelineQueue* InPipelineQueue) override;
  virtual bool IsRendering_Implementation() const override { return ProcessHandle.IsValid(); }

  // Canceling current job is equivalent to canceling all jobs for this executor
  virtual void CancelCurrentJob_Implementation() override { CancelAllJobs_Implementation(); }
  virtual void CancelAllJobs_Implementation() override;
  // ~UMoviePipelineExecutorBase Interface
 public:
  DoodleMovieRemoteState GetRenderState();
  float GetProgress();

 protected:
  void CheckForProcessFinished();
  // 上传文件
  bool UploadFiles();

 protected:
  /** A handle to the currently running process (if any) for the active job. */
  FProcHandle ProcessHandle;

 private:
  TSet<int32> Render_IDs{};
  int32 GetRepository_ID{};

  FString Remote_Repository;
  TArray<FDoodleRemoteRenderJobArg> RemoteRenderJobArgs;
  TWeakPtr<class SWindow> WeakCustomWindow;

  // 生成命令行
  void GenerateCommandLineArguments(UMoviePipelineQueue* InPipelineQueue);
  TArray<TObjectPtr<UMoviePipelineQueue>> GetQueuesToRender(UMoviePipelineQueue* InPipelineQueue);

  void StartRemoteClientRender();

  inline int32 GetProt() { return 50021; }

  void FindRemoteClient();
  UFUNCTION()
  void HttpRemoteClient(int32 RequestIndex, int32 ResponseCode, const FString& Message);
};