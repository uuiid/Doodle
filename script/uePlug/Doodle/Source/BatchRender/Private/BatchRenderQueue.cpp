#include "BatchRenderQueue.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelineConfigBase.h"
#include "MovieRenderPipelineSettings.h"
#include "MoviePipelineDeferredPasses.h"
#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelineImageSequenceOutput.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelinePIEExecutor.h"
#include <BatchRenderQueueEngineSubsystem.h>
#include "MoviePipelineConsoleVariableSetting.h"
#include <FieldNotification/FieldMulticastDelegate.h>

#define LOCTEXT_NAMESPACE "UBatchRenderQueue"
void UBatchRenderQueue::RenderMovies()
{

    PipelineSubsystem = GEngine->GetEngineSubsystem<UBatchRenderQueueEngineSubsystem>();
    PipelineSubsystem->OnBatchRenderFinished.BindUObject(this, &UBatchRenderQueue::OnQueueFinished);
    PipelineSubsystem->OnBatchRenderFinished2.BindUObject(this, &UBatchRenderQueue::OnQueueFinished);
	UMoviePipelineQueue* RenderQueue = PipelineSubsystem->GetQueue();
    TArray<UMoviePipelineExecutorJob*> ExistingJobs = RenderQueue->GetJobs();
    for (UMoviePipelineExecutorJob* Job : ExistingJobs)
    {
        RenderQueue->DeleteJob(Job);
    }
    for (const URenderJobInfo& JobInfo : BatchJobs)
    {
        UMoviePipelineExecutorJob* Job = RenderQueue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
        Job->Map = FSoftObjectPath(JobInfo.Map);
        UE_LOG(LogTemp, Display, TEXT("Map: %s"),*JobInfo.Map);
        Job->Sequence = FSoftObjectPath(JobInfo.Sequence);
        TArray<FString> SplitedStr;
        JobInfo.Sequence.ParseIntoArray(SplitedStr, TEXT("/"), false);

        UMoviePipelineOutputSetting* OutputSetting = Cast<UMoviePipelineOutputSetting>(Job->GetConfiguration()->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
        OutputSetting->OutputResolution = FIntPoint(3840, 3840);
        OutputSetting->OutputDirectory.Path = JobInfo.OutputDir;
        OutputSetting->FileNameFormat = SplitedStr[SplitedStr.Num()-1] + TEXT(".{frame_number}");


        UMoviePipelineImageSequenceOutput_PNG* ImageOutput = Cast<UMoviePipelineImageSequenceOutput_PNG>(Job->GetConfiguration()->FindOrAddSettingByClass(UMoviePipelineImageSequenceOutput_PNG::StaticClass()));
        Job->GetConfiguration()->FindOrAddSettingByClass(UMoviePipelineDeferredPassBase::StaticClass());


        UMoviePipelineAntiAliasingSetting* AntiAliasingSetting = Cast<UMoviePipelineAntiAliasingSetting>(Job->GetConfiguration()->FindOrAddSettingByClass(UMoviePipelineAntiAliasingSetting::StaticClass()));
        AntiAliasingSetting->bRenderWarmUpFrames = true;
        AntiAliasingSetting->bOverrideAntiAliasing = true;
        AntiAliasingSetting->AntiAliasingMethod = EAntiAliasingMethod::AAM_TSR;
        AntiAliasingSetting->EngineWarmUpCount = 32;
     

        //UMoviePipelineConsoleVariableSetting* ConsoleVariableSetting = Cast<UMoviePipelineConsoleVariableSetting>(Job->GetConfiguration()->FindOrAddSettingByClass(UMoviePipelineConsoleVariableSetting::StaticClass()));
        //ConsoleVariableSetting->AddOrUpdateConsoleVariable(TEXT("r.Tonemapper.Sharpen"), 2.0);
        //ConsoleVariableSetting->AddOrUpdateConsoleVariable(TEXT("r.RayTracing.Shadows.SamplesPerPixel"), 8.0);
        //Job->GetConfiguration()->InitializeTransientSettings();
    }
 

    // 渲染队列并注册回调函数
    //const UMovieRenderPipelineProjectSettings* ProjectSettings = GetDefault<UMovieRenderPipelineProjectSettings>();
    //TSubclassOf<UMoviePipelineExecutorBase> ExecutorClass = ProjectSettings->DefaultLocalExecutor.TryLoadClass<UMoviePipelineExecutorBase>();
    //Executor = Cast<UMoviePipelinePIEExecutor>(PipelineSubsystem->RenderQueueWithExecutor(UMoviePipelinePIEExecutor::StaticClass()));
    //Executor = NewObject<UMoviePipelinePIEExecutor>(this);
    //UE::MoviePipeline::FViewportArgs InitArgs;
    //InitArgs.bRenderViewport = true;
    //InitArgs.DebugWidgetClass = Executor->DebugWidgetClass;
    //Executor->SetViewportInitArgs(InitArgs);
    //Executor- OnExecutorFinishedDelegateNative.//().AddUObject(this, &UBatchRenderQueue::OnQueueFinished);
    //Executor->OnIndividualJobFinished().AddUObject(this, &UBatchRenderQueue::OnIndividualJobFinished);
    //Executor->Execute(RenderQueue);
    //FDelegate NewDelegate;
    
    PipelineSubsystem->RenderQueueWithExecutor(UMoviePipelinePIEExecutor::StaticClass());
}

void UBatchRenderQueue::OnQueueFinished(TArray<UMoviePipelineExecutorJob*> Jobs)
{

    for (UMoviePipelineExecutorJob* Job : Jobs)
    {
        FString Map = Job->Map.ToString();
        //UMoviePipelineOutputSetting* OutputSetting = Job->GetConfiguration()->FindSettingByClass(UMoviePipelineOutputSetting::StaticClass())
        UMoviePipelineOutputSetting* OutputSetting = Cast<UMoviePipelineOutputSetting>(Job->GetConfiguration()->FindSettingByClass(UMoviePipelineOutputSetting::StaticClass()));
        FString OutputDirectory = OutputSetting->OutputDirectory.Path;
        //DeleteFrame(OutputDirectory);
        FString FFMpegPath;
        FPackageName::TryConvertGameRelativePackagePathToLocalPath(TEXT("/Doodle/ThirdParty/ffmpeg.exe"), FFMpegPath);
        FFMpegPath = FPaths::ConvertRelativePathToFull(FFMpegPath.Replace(TEXT("Content/"), TEXT("")));
        // encode mp4

        FString InputPattern = OutputDirectory + TEXT("/Sequence.%04d.png");
        FDateTime Now = FDateTime::Now();
        FString DefaultTimeString = Now.ToString();
        FString OutputFilePath = FPaths::GetPath(OutputDirectory)/ DefaultTimeString + TEXT(".mp4");
        FTimerHandle handle;
        FTimerDelegate RigDelegate = FTimerDelegate::CreateLambda([=,this]()
            {
                // 批量导入Rig
                FScopedSlowTask ScopedSlow{6.0f, LOCTEXT("ConvertPNGSequenceToVideo", "正在合成视频 ...")};
                ScopedSlow.MakeDialog();
                ScopedSlow.EnterProgressFrame(0.5f, LOCTEXT("ConvertPNGSequenceToVideo", "正在合成视频 ..."));
                ConvertPNGSequenceToVideo(FFMpegPath, InputPattern, OutputFilePath, 25);
                ScopedSlow.EnterProgressFrame(0.5f, LOCTEXT("ConvertPNGSequenceToVideo", "正在合成视频 ..."));
                DeleteFrame(OutputDirectory);
                FPlatformProcess::ExploreFolder(*OutputFilePath);
                ScopedSlow.EnterProgressFrame(1.0f, LOCTEXT("ConvertPNGSequenceToVideo", "正在合成视频 ..."));
            });
        //delay for director watcher finding the file changes
        GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(handle, RigDelegate, 0.5f, false);

    }
    //OnExecutorFinishedDelegateNative.Broadcast(TEXT("Msg"),true);
    //OnExecutorFinishedDelegate.Broadcast(this, !bAnyJobHadFatalError);
    //UE_LOG(LogTemp, Log, TEXT("===================Success======================%s"),*Map);
}

bool UBatchRenderQueue::DeleteFrame(FString Dir)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    TArray<FString> Files;
    bool IsAddTask = false;
    PlatformFile.FindFiles(Files, *Dir, nullptr);
    for (const FString File : Files)
    {
        FString FullFilePath = File;//FPaths::Combine(RenderFilePath, File);
        //UE_LOG(LogTemp, Log, TEXT("Found file: %s"), *FullFilePath);
        if (!File.EndsWith(TEXT(".png"))) {
            continue;
        }
        //TArray<FString> SpFullFPS;
        //FullFilePath.ParseIntoArray(SpFullFPS, TEXT("."), false);

        //FString FrameNum = SpFullFPS[SpFullFPS.Num()-2];
        //UE_LOG(LogTemp, Log, TEXT("FrameNum: %s"), *FrameNum);
        //int32 Num = FCString::Atoi(*FrameNum);

        //FString DebugNum = Num < 200 ? TEXT("true") : TEXT("false");
        //UE_LOG(LogTemp, Log, TEXT("DebugNum: %s"), *DebugNum);
        if (PlatformFile.DeleteFile(*FullFilePath))
        {
            UE_LOG(LogTemp, Log, TEXT("Deleted file: %s"), *FullFilePath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to delete file: %s"), *FullFilePath);
        }
        IsAddTask = true;
        

    }
    PlatformFile.DeleteDirectory(*Dir);
    return IsAddTask;
}

void UBatchRenderQueue::ConvertPNGSequenceToVideo(const FString& FFmpegPath, const FString& InputPattern, const FString& OutputFilePath, int32 FrameRate)
{
    FString CommandLineParams = FString::Printf(TEXT("-framerate %d -start_number 1001 -i %s -c:v libx264 -pix_fmt yuv420p %s"), FrameRate, *InputPattern, *OutputFilePath);

    // 执行 FFmpeg
    FString OutStdOut;
    FString OutStdErr;
    int32 ReturnCode = 0;

    FPlatformProcess::ExecProcess(*FFmpegPath, *CommandLineParams, &ReturnCode, &OutStdOut, &OutStdErr);

    if (ReturnCode == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("FFmpeg executed successfully!"));
        UE_LOG(LogTemp, Log, TEXT("Output: %s"), *OutStdOut);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FFmpeg execution failed!"));
        UE_LOG(LogTemp, Error, TEXT("Error: %s"), *OutStdErr);
    }

}

void UBatchRenderQueue::OnQueueFinished()
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    /*if(OnExecutorFinishedDelegate.IsBound())
    {
        OnExecutorFinishedDelegate.Execute();
    }
    else {
        UE_LOG(LogTemp, Log, TEXT("Individual job completed. Success: %d"), OnExecutorFinishedDelegate.IsBound());

    }*/
    

    //OnExecutorFinishedDelegate.Broadcast(this, !bAnyJobHadFatalError);
    //for (URenderJobInfo BatchJob : BatchJobs)
    //{
    //    FString RenderFilePath = BatchJob.OutputDir;
    //    if (!PlatformFile.DirectoryExists(*RenderFilePath))
    //    {
    //        //UE_LOG(LogCGTAssetImport, Log, TEXT("Directory does not exist: %s"), *RenderFilePath);
    //        //AddTaskError(TEXT("删除"), FString::Printf(TEXT("(%s)不存在渲染文件夹"), *Item->ShotNum));
    //        continue;
    //    }
    //    DeleteFrame(RenderFilePath);

    //    
    //}
}

void UBatchRenderQueue::OnIndividualJobFinished(UMoviePipelineExecutorJob* InJob, bool bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("Individual job completed. Success: %d"), bSuccess);
}

