#include <Doodle/MovieRemoteExecutor.h>
//
#include "FileHelpers.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
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
//
#include "Framework/Notifications/NotificationManager.h"  // 通知消息
#include "Widgets/Notifications/INotificationWidget.h"    // 通知消息继成
#include "Widgets/Notifications/SNotificationList.h"      // 通知消息结构
#include "Widgets/Notifications/SProgressBar.h"           // 进度条
//
#include "Algo/Rotate.h"          // 旋转数组
#include "JsonObjectConverter.h"  // json转换
#define LOCTEXT_NAMESPACE "DoodleMovieRemoteExecutor"

class SDoodleRemoteNotification : public SCompoundWidget, public INotificationWidget {
 public:
  SLATE_BEGIN_ARGS(SDoodleRemoteNotification) {}
  SLATE_END_ARGS()

  void SetOwner(TSharedPtr<SNotificationItem> InOwningNotification) { WeakOwningNotification = InOwningNotification; }

  void Construct(const FArguments&, const TObjectPtr<UDoodleMovieRemoteExecutor>& In_Executor) {
    MovieRemoteExecutor = In_Executor;
    ChildSlot
        // clang-format off
        [
            SNew(SBox)
            .Padding(FMargin{15.f})
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .Padding(FMargin{0,0,0,5.0f})
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font(FCoreStyle::Get().GetFontStyle(TEXT("NotificationList.FontBold")))
					.Text(this, &SDoodleRemoteNotification::GenText)
				]

                + SVerticalBox::Slot()
                .Padding(FMargin{0,0,0,5.0f})
                .AutoHeight()
				// 进度条
				[
					SNew(SProgressBar)
					.RefreshRate(0.1f)
					.Percent(this, &SDoodleRemoteNotification::GetProgress)
				]

                + SVerticalBox::Slot()
                .Padding(FMargin{0,0,0,5.0f})
                .AutoHeight()
				[
					SNew(SHorizontalBox)
					
					+ SHorizontalBox::Slot()
					[
						SAssignNew(Button, SButton)
						.Text(LOCTEXT("StopButton", "Stop"))
						.OnClicked(this, &SDoodleRemoteNotification::ButtonStop)
					]
				]


            ]
        ];
    // clang-format on
  }

 public:
  virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override {
    TSharedPtr<SNotificationItem> Owner = WeakOwningNotification.Pin();
    if (Owner.IsValid()) {
      if (true) {
        Owner->SetFadeOutDuration(0.f);
        Owner->SetExpireDuration(0.f);
      }

      Owner->ExpireAndFadeout();

      // Remove our reference to the owner now that it's fading out
      Owner = nullptr;
    }
  }
  // 设置进度
  TOptional<float> GetProgress() const { return MovieRemoteExecutor->GetProgress(); }

  FText GenText() const {
    return FText::Format(
        LOCTEXT("GenText", "渲染状态 {0}"), UEnum::GetDisplayValueAsText(MovieRemoteExecutor->GetRenderState())
    );
  }

  virtual TSharedRef<SWidget> AsWidget() override { return AsShared(); }

  // Unused
  virtual void OnSetCompletionState(SNotificationItem::ECompletionState InState) override {}

 private:
  FReply ButtonStop() { return FReply::Unhandled(); }

 private:
  TSharedPtr<SWidget> Button, Throbber, Hyperlink;
  TSharedPtr<STextBlock> TextBlock;
  float Progress;
  FText Render_State;

  UFUNCTION()
  TObjectPtr<UDoodleMovieRemoteExecutor> MovieRemoteExecutor;

  TWeakPtr<SNotificationItem> WeakOwningNotification;
};

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
  StartRemoteClientRender();
}

void UDoodleMovieRemoteExecutor::CancelAllJobs_Implementation() {}

DoodleMovieRemoteState UDoodleMovieRemoteExecutor::GetRenderState() {
  // UE_LOG(LogTemp, Warning, TEXT("开始状态查询: %f"), 0.f);
  return DoodleMovieRemoteState::Wait;
}

float UDoodleMovieRemoteExecutor::GetProgress() {
  UE_LOG(LogTemp, Warning, TEXT("开始进度查询: %f"), 0.f);

  return 0.0f;
}

void UDoodleMovieRemoteExecutor::CheckForProcessFinished() {}

bool UDoodleMovieRemoteExecutor::UploadFiles() {
  TArray<FString> L_StrList{};
#if 1
  static auto G_Config{TEXT("//192.168.20.59/UE_Config/Remote_Debug.txt")};
#else
  static auto G_Config{TEXT("//192.168.20.59/UE_Config/Remote.txt")};
#endif  // 1

  TSharedPtr<FScopedSlowTask> L_Task_Scoped_Ptr =
      MakeShared<FScopedSlowTask>(0, LOCTEXT("DoingSlowWork2", "复制文件中..."));
  L_Task_Scoped_Ptr->MakeDialog();

  FFileHelper::LoadFileToStringArray(L_StrList, G_Config);

  IPlatformFile& L_PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

  FString L_Prj_Name            = FPaths::GetBaseFilename(FPaths::GetProjectFilePath());
  FString L_R_Prj               = L_StrList[0] / L_Prj_Name / FPaths::GetCleanFilename(FPaths::GetProjectFilePath());

  {  // 复制项目
    if (L_PlatformFile.FileExists(*L_R_Prj)) {
      L_PlatformFile.DeleteFile(*L_R_Prj);
    }
    L_PlatformFile.CreateDirectoryTree(*(L_StrList[0] / L_Prj_Name));
    L_Task_Scoped_Ptr->EnterProgressFrame(
        0.0F, FText::FromString(FString::Printf(TEXT("复制项目 %s -> %s"), *L_R_Prj, *(FPaths::GetProjectFilePath())))
    );
    if (!L_PlatformFile.CopyFile(*L_R_Prj, *(FPaths::GetProjectFilePath()))) return false;
    for (auto&& i : RemoteRenderJobArgs) i.ProjectPath = L_R_Prj;
  }

  {
    // 复制配置
    FString L_Config = FPaths::ProjectConfigDir();
    FPaths::NormalizeDirectoryName(L_Config);
    L_Task_Scoped_Ptr->EnterProgressFrame(
        0.0F, FText::FromString(FString::Printf(
                  TEXT("复制配置 %s -> %s"), *(L_StrList[0] / L_Prj_Name / FPaths::GetBaseFilename(L_Config)), *L_Config
              ))
    );
    L_PlatformFile.CopyDirectoryTree(*(L_StrList[0] / L_Prj_Name / FPaths::GetBaseFilename(L_Config)), *L_Config, true);
  }
  // 复制内容
  FString SourceDir(FPaths::ProjectContentDir());
  FPaths::NormalizeDirectoryName(SourceDir);

  FString DestDir(L_StrList[0] / L_Prj_Name / FPaths::GetBaseFilename(SourceDir));
  FPaths::NormalizeDirectoryName(DestDir);

  // Does Source dir exist?
  if (!L_PlatformFile.DirectoryExists(*SourceDir)) {
    return false;
  }

  // Destination directory exists already or can be created ?
  if (!L_PlatformFile.DirectoryExists(*DestDir) && !L_PlatformFile.CreateDirectoryTree(*DestDir)) {
    return false;
  }

  // Copy all files and directories
  struct FCopyFilesAndDirs : public IPlatformFile::FDirectoryVisitor {
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    const TCHAR* SourceRoot;
    const TCHAR* DestRoot;
    TSharedPtr<FScopedSlowTask> Task_Scoped{};

    FCopyFilesAndDirs(const TCHAR* InSourceRoot, const TCHAR* InDestRoot, TSharedPtr<FScopedSlowTask> In_Task_Scoped)
        : SourceRoot(InSourceRoot), DestRoot(InDestRoot), Task_Scoped(MoveTemp(In_Task_Scoped)) {}

    virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
      FString NewName(FilenameOrDirectory);
      // change the root
      NewName = NewName.Replace(SourceRoot, DestRoot);
      if (Task_Scoped->ShouldCancel()) return false;

      if (bIsDirectory) {
        // create new directory structure
        if (!PlatformFile.CreateDirectoryTree(*NewName) && !PlatformFile.DirectoryExists(*NewName)) {
          return false;
        }
      } else {
        // Delete destination file if it exists and we are overwriting
        if (PlatformFile.FileExists(*NewName)) {
          if (PlatformFile.FileSize(*NewName) == PlatformFile.FileSize(FilenameOrDirectory) &&
              PlatformFile.GetTimeStamp(*NewName) == PlatformFile.GetTimeStamp(FilenameOrDirectory)) {
            Task_Scoped->EnterProgressFrame(
                0.0F, FText::FromString(FString::Printf(TEXT("跳过复制 %s -> %s"), FilenameOrDirectory, *NewName))
            );
            // 继续
            return true;
          } else {
            Task_Scoped->EnterProgressFrame(0.0F, FText::FromString(FString::Printf(TEXT("删除 %s"), *NewName)));
            PlatformFile.DeleteFile(*NewName);
          }
        }
        Task_Scoped->EnterProgressFrame(
            0.0F, FText::FromString(FString::Printf(TEXT("开始复制 %s -> %s"), FilenameOrDirectory, *NewName))
        );

        // Copy file from source
        if (!PlatformFile.CopyFile(*NewName, FilenameOrDirectory)) {
          // Not all files could be copied
          return false;
        }
        PlatformFile.SetTimeStamp(*NewName, PlatformFile.GetTimeStamp(FilenameOrDirectory));
        Task_Scoped->EnterProgressFrame(
            0.0F, FText::FromString(FString::Printf(TEXT("完成复制 %s -> %s"), FilenameOrDirectory, *NewName))
        );
      }
      return true;  // continue searching
    }
  };

  // copy files and directories visitor
  FCopyFilesAndDirs CopyFilesAndDirs{*SourceDir, *DestDir, L_Task_Scoped_Ptr};

  // create all files subdirectories and files in subdirectories!
  return L_PlatformFile.IterateDirectoryRecursively(*SourceDir, CopyFilesAndDirs);
}

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

  for (auto&& i : RemoteRenderJobArgs)
    i.ProjectPath = UMoviePipelineEditorBlueprintLibrary::ConvertManifestFileToString(ManifestFilePath);

  // 循环查看任务中的设置，让他们修改命令行参数。因为我们可能有多个任务，所以我们会遍历所有任务和所有设置，希望用户的设置不会发生冲突。
  for (const UMoviePipelineExecutorJob* Job : DuplicatedQueue->GetJobs()) {
    FDoodleRemoteRenderJobArg& L_Arg = RemoteRenderJobArgs.Emplace_GetRef();
    L_Arg.Args                       = CommandLineArgs;

    TMap<FString, FStringFormatArg> NamedArguments;

    NamedArguments.Add(
        TEXT("PlayWorld"), TEXT("MoviePipelineEntryMap")
    );  // Boot up on an empty map, the executor will immediately transition to the correct one.
    NamedArguments.Add(TEXT("UnrealURL"), UnrealURLParams);  // Pass the command line arguments for this job
    NamedArguments.Add(TEXT("SubprocessCommandLine"), FCommandLine::GetSubprocessCommandline());
    TArray<FString> OutUrlParams;
    TArray<FString> OutCommandLineArgs;
    TArray<FString> OutDeviceProfileCVars;
    TArray<FString> OutExecCmds;

    Job->GetConfiguration()->InitializeTransientSettings();
    for (const UMoviePipelineSetting* Setting : Job->GetConfiguration()->GetAllSettings()) {
      Setting->BuildNewProcessCommandLineArgs(OutUrlParams, OutCommandLineArgs, OutDeviceProfileCVars, OutExecCmds);
    }

    for (const FString& UrlParam : OutUrlParams) {
      UnrealURLParams += UrlParam;
    }

    L_Arg.Args += TEXT(" ");
    for (const FString& Arg : OutCommandLineArgs) {
      L_Arg.Args += FString::Printf(TEXT(" %s"), *Arg);
    }

    if (OutDeviceProfileCVars.Num() > 0) {
      L_Arg.Args += TEXT(" -dpcvars=\"");
      for (const FString& Cvar : OutDeviceProfileCVars) {
        L_Arg.Args += FString::Printf(TEXT("%s,"), *Cvar);
      }
      L_Arg.Args += TEXT("\"");
    }

    if (OutExecCmds.Num() > 0) {
      L_Arg.Args += TEXT(" -execcmds=\"");
      for (const FString& Cmd : OutExecCmds) {
        L_Arg.Args += FString::Printf(TEXT("%s,"), *Cmd);
      }
      L_Arg.Args += TEXT("\"");
    }
    NamedArguments.Add(TEXT("CommandLineParams"), L_Arg.Args);

    L_Arg.Args = FString::Format(
        TEXT("{PlayWorld}{UnrealURL} -game {SubprocessCommandLine} {CommandLineParams}"), NamedArguments
    );
  }

  UE_LOG(LogMovieRenderPipeline, Log, TEXT("Launching a new process to render with the following command line:"));
  for (auto&& i : RemoteRenderJobArgs) UE_LOG(LogMovieRenderPipeline, Log, TEXT("%s"), *i.Args);
}

void UDoodleMovieRemoteExecutor::StartRemoteClientRender() {
  const UMoviePipelineInProcessExecutorSettings* ExecutorSettings =
      GetDefault<UMoviePipelineInProcessExecutorSettings>();
  if (RemoteClients.IsEmpty()) {
    FNotificationInfo L_Info{FText::FromString(TEXT("无法找到远程机器..."))};
    FSlateNotificationManager::Get().AddNotification(L_Info);
    OnExecutorFinishedImpl();
  }

  if (!UploadFiles()) {
    OnExecutorFinishedImpl();
    return;
  }
  FNotificationInfo L_Info{FText::FromString(TEXT("正在渲染..."))};
  // L_Info.ContentWidget        = SNew(SDoodleRemoteNotification, this);
  L_Info.bFireAndForget       = true;  // 自动取消
  L_Info.FadeInDuration       = 1.0f;  // 淡入淡出时间
  // 造型设计补充信息
  L_Info.WidthOverride        = 400.0f;
  L_Info.bUseLargeFont        = false;
  L_Info.bUseThrobber         = false;
  L_Info.bUseSuccessFailIcons = false;
  // 显示与默认警告不同的信息图标
  L_Info.Image                = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Warning"));

  L_Info.Text                 = FText::FromString(TEXT("提交渲染完成"));
  FSlateNotificationManager::Get().AddNotification(L_Info);
  static FString L_Sub_URL{TEXT("v1/render_frame/submit_job")};
  for (auto&& i : RemoteRenderJobArgs) {
    FString L_Url = FString::Printf(TEXT("http://%s:%d/%s"), *RemoteClients[0], GetProt(), *L_Sub_URL);
    Algo::Rotate(RemoteClients, 1);
    FString L_MSg{};
    FJsonObjectConverter::UStructToJsonObjectString(i, L_MSg);
    int32 L_Id = SendHTTPRequest(
        L_Url, TEXT("POSt"), L_MSg,
        TMap<FString, FString>{
            {TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent")}, {TEXT("Content-Type"), TEXT("application/json")}}
    );
    Render_IDs.Add(L_Id);
  }

  if (ExecutorSettings->bCloseEditor) {
    FPlatformMisc::RequestExit(false);
  }
  OnExecutorFinishedImpl();
}

void UDoodleMovieRemoteExecutor::FindRemoteClient() {
  static auto G_Config{TEXT("//192.168.20.59/UE_Config/Client.txt")};
  // static FString L_Sub_URL{TEXT("v1/AtWork")};

  if (FPlatformFileManager::Get().GetPlatformFile().FileExists(G_Config)) {
    TArray<FString> L_StrList{};
    FFileHelper::LoadFileToStringArray(L_StrList, G_Config);

    for (auto&& i : L_StrList) {
      RemoteClients.AddUnique(i);
    }
  }
}

void UDoodleMovieRemoteExecutor::HttpRemoteClient(int32 RequestIndex, int32 ResponseCode, const FString& Message) {}

#undef LOCTEXT_NAMESPACE