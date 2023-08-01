#include "FileHelpers.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "MoviePipelineEditorBlueprintLibrary.h"
#include "MoviePipelineGameMode.h"
#include "MoviePipelineInProcessExecutorSettings.h"
#include "MoviePipelineNewProcessExecutor.h"
#include "MoviePipelineQueue.h"
#include "MovieRenderPipelineCoreModule.h"
#include "MovieRenderPipelineSettings.h"
#include "ObjectTools.h"
#include "PackageHelperFunctions.h"
#include "Serialization/Formatters/JsonArchiveOutputFormatter.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/StructuredArchive.h"
#include "UObject/Package.h"
#include "UnrealEdMisc.h"
#include <Doodle/MovieRemoteExecutor.h>

void UDoodleMovieRemoteExecutor::Execute_Implementation(UMoviePipelineQueue* InPipelineQueue) {
  if (InPipelineQueue->GetJobs().Num() == 0) {
    OnExecutorFinishedImpl();
    return;
  }

  // Because it's run in a new process it will load packages from disk, thus we have to save changes to see them.
  bool bPromptUserToSave    = true;
  bool bSaveMapPackages     = true;
  bool bSaveContentPackages = true;
  if (!FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages)) {
    OnExecutorFinishedImpl();
    return;
  }

  // Make sure all of the maps in the queue exist on disk somewhere, otherwise the remote process boots up and then
  // fails.
  bool bHasValidMap = UMoviePipelineEditorBlueprintLibrary::IsMapValidForRemoteRender(InPipelineQueue->GetJobs());
  if (!bHasValidMap) {
    UMoviePipelineEditorBlueprintLibrary::WarnUserOfUnsavedMap();
    OnExecutorFinishedImpl();
    return;
  }
  GenerateCommandLineArguments(InPipelineQueue);
  FindRemoteClient();
  return;
}

void UDoodleMovieRemoteExecutor::CancelAllJobs_Implementation() {}

void UDoodleMovieRemoteExecutor::CheckForProcessFinished() {}

void UDoodleMovieRemoteExecutor::GenerateCommandLineArguments(UMoviePipelineQueue* InPipelineQueue) {
  // Arguments to pass to the executable. This can be modified by settings in the event a setting needs to be applied
  // early. In the format of -foo -bar
  FString CommandLineArgs;
  // In the format of ?arg=1?arg=2. This is appended after the the map name.
  FString UnrealURLParams;

  // Append all of our inherited command line arguments from the editor.
  const UMoviePipelineInProcessExecutorSettings* ExecutorSettings =
      GetDefault<UMoviePipelineInProcessExecutorSettings>();

  CommandLineArgs += ExecutorSettings->InheritedCommandLineArguments;
  CommandLineArgs += TEXT(" ") + ExecutorSettings->AdditionalCommandLineArguments;

  // Provide our own default arguments
  CommandLineArgs += FString::Printf(TEXT(" -messaging -SessionName=\"%s\""), TEXT("NewProcess Movie Render"));
  CommandLineArgs += TEXT(" -nohmd");
  CommandLineArgs += TEXT(" -windowed");
  CommandLineArgs += FString::Printf(TEXT(" -ResX=%d -ResY=%d"), 1280, 720);

  // Place the Queue in a package and serialize it to disk so we can pass their dynamic object
  // to another process without having to save/check in/etc.
  FString ManifestFilePath;
  UMoviePipelineQueue* DuplicatedQueue =
      UMoviePipelineEditorBlueprintLibrary::SaveQueueToManifestFile(InPipelineQueue, ManifestFilePath);
  if (!DuplicatedQueue) {
    UE_LOG(LogMovieRenderPipeline, Error, TEXT("Could not save manifest package to disk. Path: %s"), *ManifestFilePath);
    OnExecutorFinishedImpl();
    return;
  }

  // Boot into our custom Game Mode (to go with our custom map). Once booted the in-process Executor will load the
  // correct map with correct gamemode.
  UnrealURLParams += FString::Printf(TEXT("?game=%s"), *AMoviePipelineGameMode::StaticClass()->GetPathName());

  // Loop through our settings in the job and let them modify the command line arguments/params. Because we could have
  // multiple jobs, we go through all jobs and all settings and hope the user doesn't have conflicting settings.
  TArray<FString> OutUrlParams;
  TArray<FString> OutCommandLineArgs;
  TArray<FString> OutDeviceProfileCVars;
  TArray<FString> OutExecCmds;
  for (const UMoviePipelineExecutorJob* Job : DuplicatedQueue->GetJobs()) {
    Job->GetConfiguration()->InitializeTransientSettings();
    for (const UMoviePipelineSetting* Setting : Job->GetConfiguration()->GetAllSettings()) {
      Setting->BuildNewProcessCommandLineArgs(OutUrlParams, OutCommandLineArgs, OutDeviceProfileCVars, OutExecCmds);
    }
  }

  for (const FString& UrlParam : OutUrlParams) {
    UnrealURLParams += UrlParam;
  }

  CommandLineArgs += TEXT(" ");
  for (const FString& Arg : OutCommandLineArgs) {
    CommandLineArgs += FString::Printf(TEXT(" %s"), *Arg);
  }

  if (OutDeviceProfileCVars.Num() > 0) {
    CommandLineArgs += TEXT(" -dpcvars=\"");
    for (const FString& Cvar : OutDeviceProfileCVars) {
      CommandLineArgs += FString::Printf(TEXT("%s,"), *Cvar);
    }
    CommandLineArgs += TEXT("\"");
  }

  if (OutExecCmds.Num() > 0) {
    CommandLineArgs += TEXT(" -execcmds=\"");
    for (const FString& Cmd : OutExecCmds) {
      CommandLineArgs += FString::Printf(TEXT("%s,"), *Cmd);
    }
    CommandLineArgs += TEXT("\"");
  }

  if (FPaths::IsProjectFilePathSet()) {
    RemoteRenderJobArg.ProjectPath = FString::Printf(TEXT("\"%s\""), *FPaths::GetProjectFilePath());
  } else {
    RemoteRenderJobArg.ProjectPath = FApp::GetProjectName();
  }

  RemoteRenderJobArg.ManifestValue =
      UMoviePipelineEditorBlueprintLibrary::ConvertManifestFileToString(ManifestFilePath);

  TMap<FString, FStringFormatArg> NamedArguments;

  NamedArguments.Add(
      TEXT("PlayWorld"), TEXT("MoviePipelineEntryMap")
  );  // Boot up on an empty map, the executor will immediately transition to the correct one.
  NamedArguments.Add(TEXT("UnrealURL"), UnrealURLParams);  // Pass the command line arguments for this job
  NamedArguments.Add(TEXT("SubprocessCommandLine"), FCommandLine::GetSubprocessCommandline());
  NamedArguments.Add(TEXT("CommandLineParams"), CommandLineArgs);

  RemoteRenderJobArg.Args = FString::Format(
      TEXT("{PlayWorld}{UnrealURL} -game {SubprocessCommandLine} {CommandLineParams} "), NamedArguments
  );

  // Prefer the -Cmd version of the executable if possible, gracefully falling back to the normal one. This is to help
  // with user education about the -Cmd version which allows piping the output log to the cmd window that launched it.
  const FString ExecutablePath   = FUnrealEdMisc::Get().GetExecutableForCommandlets();

  uint32 ProcessID               = 0;
  const bool bLaunchDetatched    = true;
  const bool bLaunchMinimized    = false;
  const bool bLaunchWindowHidden = false;
  const uint32 PriorityModifier  = 0;

  UE_LOG(LogMovieRenderPipeline, Log, TEXT("Launching a new process to render with the following command line:"));
  UE_LOG(LogMovieRenderPipeline, Log, TEXT("%s %s"), *ExecutablePath, *RemoteRenderJobArg.Args);
}

void UDoodleMovieRemoteExecutor::StartRemoteClientRender() {
  const UMoviePipelineInProcessExecutorSettings* ExecutorSettings =
      GetDefault<UMoviePipelineInProcessExecutorSettings>();
  // todo: 上传文件

  if (ExecutorSettings->bCloseEditor) {
    FPlatformMisc::RequestExit(false);
  } else {
    // Register a tick handler to listen every frame to see if the process shut down gracefully, we'll use return codes
    // to tell success vs cancel.
    FCoreDelegates::OnBeginFrame.AddUObject(this, &UDoodleMovieRemoteExecutor::CheckForProcessFinished);
  }
}

void UDoodleMovieRemoteExecutor::FindRemoteClient() {
  static auto G_Config{TEXT("//192.168.20.59/UE_Config/Client.txt")};
  static FString L_Sub_URL{TEXT("v1/AtWork")};
  HTTPResponseRecievedDelegate.AddDynamic(this, &UDoodleMovieRemoteExecutor::HttpRemoteClient);
  if (FPlatformFileManager::Get().GetPlatformFile().FileExists(G_Config)) {
    TArray<FString> L_StrList{};
    FFileHelper::LoadFileToStringArray(L_StrList, G_Config);

    for (auto&& i : L_StrList) {
      FString L_Url = FString::Printf(TEXT("http://%s:%d/%s/"), *i, 50021, *L_Sub_URL);
      // int32 L_Id    = SendHTTPRequest(L_Url, TEXT("POST"), {}, TMap<FString, FString>{{TEXT("Content-Type"),
      // TEXT("text")}});
      int32 L_Id    = SendHTTPRequest(
          L_Url, TEXT("GET"), {}, TMap<FString, FString>{{TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent")}}
      );
      RemoteClientMap.Add(L_Id) = i;
    }
  }
}

void UDoodleMovieRemoteExecutor::HttpRemoteClient(int32 RequestIndex, int32 ResponseCode, const FString& Message) {
  if (ResponseCode == 200) {
    RemoteClientUrl = RemoteClientMap[RequestIndex];
    UE_LOG(LogTemp, Warning, TEXT("使用工作机: %s"), *RemoteClientUrl);
    HTTPResponseRecievedDelegate.RemoveAll(this);
    StartRemoteClientRender();
  } else {
    RemoteClientMap.Remove(RequestIndex);
    UE_LOG(LogTemp, Warning, TEXT("错误的返回代码: %d"), ResponseCode);
  }

  if (RemoteClientMap.IsEmpty()) {
    UE_LOG(LogTemp, Warning, TEXT("全部客户端被占用"));
    HTTPResponseRecievedDelegate.RemoveAll(this);
    OnExecutorFinishedImpl();
  }
}
