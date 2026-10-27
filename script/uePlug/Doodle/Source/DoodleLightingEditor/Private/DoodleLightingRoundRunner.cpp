#include "DoodleLightingRoundRunner.h"

#include "DoodleLightingExecutor.h"
#include "DoodleLightingJson.h"
#include "DoodleLightingReport.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/Light.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "LevelSequence.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "MovieSceneSpawnable.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "MoviePipelineAntiAliasingSetting.h"
#include "MoviePipelineCameraSetting.h"
#include "MoviePipelineConsoleVariableSetting.h"
#include "MoviePipelineDeferredPasses.h"
#include "MoviePipelineEXROutput.h"
#include "MoviePipelineGameOverrideSetting.h"
#include "MoviePipelineImageSequenceOutput.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelinePIEExecutor.h"
#include "MoviePipelinePrimaryConfig.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineDataTypes.h"

namespace
{
	const TCHAR* const GAiLightParamsKey = TEXT("DoodleAiLightParams=");

	constexpr int32 GDefaultFrameRate = 25;

	bool LoadUtf8File(const FString& InFilePath, FString& OutText, FString& OutError)
	{
		TArray<uint8> L_Bytes;
		if (!FFileHelper::LoadFileToArray(L_Bytes, *InFilePath))
		{
			OutError = FString::Printf(TEXT("读不了载荷文件 %s"), *InFilePath);
			return false;
		}
		int32 L_Offset = 0;
		if (L_Bytes.Num() >= 3 && L_Bytes[0] == 0xEF && L_Bytes[1] == 0xBB && L_Bytes[2] == 0xBF)
		{
			L_Offset = 3;
		}
		OutText = FString(L_Bytes.Num() - L_Offset, reinterpret_cast<const ANSICHAR*>(L_Bytes.GetData() + L_Offset));
		return true;
	}

	bool ParseJsonObject(const FString& InText, TSharedPtr<FJsonObject>& OutObject, FString& OutError)
	{
		const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InText);
		if (!FJsonSerializer::Deserialize(L_Reader, OutObject) || !OutObject.IsValid())
		{
			OutError = TEXT("载荷不是合法 json");
			return false;
		}
		return true;
	}

	FString NormalizePackagePath(const FString& InPath)
	{
		if (InPath.IsEmpty() || InPath.StartsWith(TEXT("/")))
		{
			return InPath;
		}
		FString L_PackageName;
		if (FPackageName::TryConvertFilenameToLongPackageName(InPath, L_PackageName))
		{
			return L_PackageName;
		}
		return InPath;
	}

	int32 CountPngFrames(const FString& InFramesDir)
	{
		if (InFramesDir.IsEmpty())
		{
			return 0;
		}
		TArray<FString> L_Files;
		IFileManager::Get().FindFiles(L_Files, *FPaths::Combine(InFramesDir, TEXT("*.png")), true, false);
		return L_Files.Num();
	}

	FFrameRate ResolveFrameRate(const ULevelSequence* InSequence)
	{
		if (InSequence == nullptr || InSequence->GetMovieScene() == nullptr)
		{
			return FFrameRate(GDefaultFrameRate, 1);
		}
		const FFrameRate L_Rate = InSequence->GetMovieScene()->GetDisplayRate();
		if (!L_Rate.IsValid() || L_Rate.AsDecimal() <= 0.0)
		{
			return FFrameRate(GDefaultFrameRate, 1);
		}
		return L_Rate;
	}

	int32 StripLightSpawnables(ULevelSequence& InSequence)
	{
		UMovieScene* L_MovieScene = InSequence.GetMovieScene();
		if (L_MovieScene == nullptr)
		{
			return 0;
		}
		int32 L_Removed = 0;
		for (int32 L_Index = L_MovieScene->GetSpawnableCount() - 1; L_Index >= 0; --L_Index)
		{
			const FMovieSceneSpawnable& L_Spawnable = L_MovieScene->GetSpawnable(L_Index);
			const UObject* L_Template = L_Spawnable.GetObjectTemplate();
			if (L_Template == nullptr)
			{
				continue;
			}
			if (!L_Template->IsA<ALight>() && !L_Template->IsA<ASkyLight>())
			{
				continue;
			}
			L_MovieScene->RemoveSpawnable(L_Spawnable.GetGuid());
			++L_Removed;
		}
		return L_Removed;
	}

	void ApplyCandidateSettings(UMoviePipelineExecutorJob& InJob, const FDoodleTaskCandidate& InCandidate,
	                            const FFrameRate& InFrameRate, bool bHasBaseConfig)
	{
		UMoviePipelinePrimaryConfig* L_Config = InJob.GetConfiguration();
		if (L_Config == nullptr)
		{
			return;
		}

		if (UMoviePipelineOutputSetting* L_Output = Cast<UMoviePipelineOutputSetting>(
				L_Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass())))
		{
			L_Output->OutputDirectory.Path = InCandidate.FramesDir;
			L_Output->FileNameFormat = TEXT("frame_{frame_number}");
			L_Output->ZeroPadFrameNumbers = 4;
			L_Output->bUseCustomFrameRate = true;
			L_Output->OutputFrameRate = InFrameRate;
			if (!bHasBaseConfig)
			{
				L_Output->OutputResolution = FIntPoint(1920, 1080);
			}
		}

		if (UMoviePipelineImageSequenceOutput_EXR* L_Exr = L_Config->FindSetting<UMoviePipelineImageSequenceOutput_EXR>())
		{
			L_Config->RemoveSetting(L_Exr);
		}
		if (UMoviePipelineImageSequenceOutput_JPG* L_Jpg = L_Config->FindSetting<UMoviePipelineImageSequenceOutput_JPG>())
		{
			L_Config->RemoveSetting(L_Jpg);
		}
		L_Config->FindOrAddSettingByClass(UMoviePipelineImageSequenceOutput_PNG::StaticClass());
		L_Config->FindOrAddSettingByClass(UMoviePipelineDeferredPassBase::StaticClass());

		if (bHasBaseConfig)
		{
			return;
		}

		if (UMoviePipelineConsoleVariableSetting* L_ConsoleVar = Cast<UMoviePipelineConsoleVariableSetting>(
				L_Config->FindOrAddSettingByClass(UMoviePipelineConsoleVariableSetting::StaticClass())))
		{
			L_ConsoleVar->AddOrUpdateConsoleVariable(TEXT("r.MotionBlurQuality"), 0.0);
			L_ConsoleVar->AddOrUpdateConsoleVariable(TEXT("r.DefaultFeature.MotionBlur"), 0.0);
			L_ConsoleVar->AddOrUpdateConsoleVariable(TEXT("r.Streaming.UseFixedPoolSize"), 1.0);
			L_ConsoleVar->AddOrUpdateConsoleVariable(TEXT("r.Streaming.PoolSize"), 4096.0);
		}
		if (UMoviePipelineAntiAliasingSetting* L_AntiAliasing = Cast<UMoviePipelineAntiAliasingSetting>(
				L_Config->FindOrAddSettingByClass(UMoviePipelineAntiAliasingSetting::StaticClass())))
		{
			L_AntiAliasing->SpatialSampleCount = 1;
			L_AntiAliasing->TemporalSampleCount = 1;
			L_AntiAliasing->bOverrideAntiAliasing = true;
			L_AntiAliasing->AntiAliasingMethod = EAntiAliasingMethod::AAM_TSR;
			L_AntiAliasing->bRenderWarmUpFrames = true;
			L_AntiAliasing->EngineWarmUpCount = 128;
			L_AntiAliasing->RenderWarmUpCount = 0;
		}
		if (UMoviePipelineGameOverrideSetting* L_GameOverride = Cast<UMoviePipelineGameOverrideSetting>(
				L_Config->FindOrAddSettingByClass(UMoviePipelineGameOverrideSetting::StaticClass())))
		{
			L_GameOverride->bCinematicQualitySettings = true;
			L_GameOverride->TextureStreaming = EMoviePipelineTextureStreamingMethod::None;
		}
		if (UMoviePipelineCameraSetting* L_Camera = Cast<UMoviePipelineCameraSetting>(
				L_Config->FindOrAddSettingByClass(UMoviePipelineCameraSetting::StaticClass())))
		{
			L_Camera->ShutterTiming = EMoviePipelineShutterTiming::FrameClose;
			L_Camera->OverscanPercentage = 0.0;
		}
	}
}

bool UDoodleLightingRoundRunner::HasAiLightParams()
{
	FString L_Params;
	if (!FParse::Value(FCommandLine::Get(), GAiLightParamsKey, L_Params))
	{
		return false;
	}
	return !L_Params.IsEmpty();
}

void UDoodleLightingRoundRunner::RequestExit(int32 InExitCode)
{
	UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 本轮结束，退出码 %d"), InExitCode);
	FPlatformMisc::RequestExitWithStatus(false, static_cast<uint8>(FMath::Clamp(InExitCode, 0, 255)));
}

void UDoodleLightingRoundRunner::Main(float InDeltaTime)
{
	if (bRoundStarted)
	{
		return;
	}
	FAssetRegistryModule& L_AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	if (L_AssetRegistry.Get().IsLoadingAssets())
	{
		return;
	}
	StartRound();
}

void UDoodleLightingRoundRunner::StartRoundFromLaunchPlan(const FDoodleLaunchPlan& InPlan)
{
	bRoundStarted = true;
	bInteractiveMode = true;

	Payload = FDoodleTaskPayload();
	Payload.SessionId = InPlan.SessionId;
	Payload.Iteration = InPlan.Iteration;
	Payload.OutDir = InPlan.OutDir;
	Payload.RenderMap = InPlan.RenderMap;
	Payload.LevelSequence = InPlan.LevelSequence;
	Payload.MoviePipelineConfig = InPlan.MoviePipelineConfig;
	Payload.FrameBegin = InPlan.FrameBegin;
	Payload.FrameEnd = InPlan.FrameEnd;
	Payload.UeProjectPath = InPlan.UeProjectPath;
	Payload.CameraFilePath = InPlan.CameraFilePath;
	Payload.Candidates = InPlan.Candidates;

	FString L_StartError;
	if (!StartRoundInternal(L_StartError))
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] 本机一轮没能开始：%s"), *L_StartError);
	}
}

void UDoodleLightingRoundRunner::StartRound()
{
	bRoundStarted = true;

	FString L_Params;
	FParse::Value(FCommandLine::Get(), GAiLightParamsKey, L_Params);
	PayloadPath = FPaths::ConvertRelativePathToFull(L_Params);
	UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 载荷：%s"), *PayloadPath);

	FString L_Text;
	FString L_Error;
	if (!LoadUtf8File(PayloadPath, L_Text, L_Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] %s"), *L_Error);
		RequestExit(1);
		return;
	}
	TSharedPtr<FJsonObject> L_Root;
	if (!ParseJsonObject(L_Text, L_Root, L_Error) || !L_Root.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] %s"), *L_Error);
		RequestExit(1);
		return;
	}

	FString L_RawOutDir;
	L_Root->TryGetStringField(TEXT("out_dir"), L_RawOutDir);

	if (!DoodleLightingJson::ParseTaskPayload(L_Root, Payload, L_Error))
	{
		Payload.OutDir = L_RawOutDir;
		FailRound(FString::Printf(TEXT("载荷解析失败：%s"), *L_Error));
		return;
	}
	if (Payload.OutDir.IsEmpty())
	{
		Payload.OutDir = L_RawOutDir;
	}
	if (Payload.OutDir.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] 载荷里既没有 out_dir，也没有别的落点"));
		RequestExit(1);
		return;
	}

	FString L_StartError;
	if (!StartRoundInternal(L_StartError))
	{
		FailRound(L_StartError);
	}
}

bool UDoodleLightingRoundRunner::StartRoundInternal(FString& OutError)
{
	if (Payload.OutDir.IsEmpty())
	{
		OutError = TEXT("计划/载荷里没有 out_dir");
		return false;
	}
	if (Payload.Candidates.IsEmpty())
	{
		OutError = TEXT("计划/载荷里没有候选");
		return false;
	}

	for (const FDoodleTaskCandidate& L_Candidate : Payload.Candidates)
	{
		FDoodleCandidateReport L_Report;
		L_Report.CandidateIndex = L_Candidate.CandidateIndex;
		L_Report.CandidateId = L_Candidate.CandidateId;
		CandidateReports.Add(L_Report);
	}

	UWorld* L_RenderWorld = LoadObject<UWorld>(nullptr, *NormalizePackagePath(Payload.RenderMap));
	if (L_RenderWorld == nullptr)
	{
		OutError = FString::Printf(TEXT("渲染关卡加载失败：%s"), *Payload.RenderMap);
		return false;
	}
	ULevelSequence* L_Sequence = LoadObject<ULevelSequence>(nullptr, *NormalizePackagePath(Payload.LevelSequence));
	if (L_Sequence == nullptr)
	{
		OutError = FString::Printf(TEXT("关卡序列加载失败：%s"), *Payload.LevelSequence);
		return false;
	}

	const int32 L_StrippedLights = StripLightSpawnables(*L_Sequence);
	const FFrameRate L_FrameRate = ResolveFrameRate(L_Sequence);
	FrameRate = FMath::Max(1, FMath::RoundToInt(L_FrameRate.AsDecimal()));

	UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 序列 %s：剥掉 %d 个灯 spawnable，display rate %d fps，帧区间 %d~%d（诊断）"),
	       *Payload.LevelSequence, L_StrippedLights, FrameRate, Payload.FrameBegin, Payload.FrameEnd);

	{
		DoodleLightingExecutor::FRoundContext L_Context;
		L_Context.CameraFilePath = Payload.CameraFilePath;
		FString L_CameraError;
		if (!DoodleLightingExecutor::PrepareAnchorCamera(L_Context, L_CameraError))
		{
			OutError = FString::Printf(TEXT("机位准备失败：%s"), *L_CameraError);
			return false;
		}
	}

	UMoviePipelineQueueSubsystem* L_Subsystem =
		GEditor != nullptr ? GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>() : nullptr;
	if (L_Subsystem == nullptr)
	{
		OutError = TEXT("拿不到 UMoviePipelineQueueSubsystem，建不了渲染队列");
		return false;
	}
	UMoviePipelineQueue* L_Queue = L_Subsystem->GetQueue();
	if (L_Queue == nullptr)
	{
		OutError = TEXT("电影渲染队列不可用");
		return false;
	}
	L_Queue->DeleteAllJobs();

	UMoviePipelinePrimaryConfig* L_BaseConfig = nullptr;
	if (!Payload.MoviePipelineConfig.IsEmpty())
	{
		L_BaseConfig = LoadObject<UMoviePipelinePrimaryConfig>(nullptr, *NormalizePackagePath(Payload.MoviePipelineConfig));
	}
	if (L_BaseConfig == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DoodleLighting] 没有可用的电影管线配置（%s），本轮按内置画质设置渲染"),
		       *Payload.MoviePipelineConfig);
	}

	for (const FDoodleTaskCandidate& L_Candidate : Payload.Candidates)
	{
		UMoviePipelineExecutorJob* L_Job = L_Queue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
		if (L_Job == nullptr)
		{
			OutError = TEXT("建渲染 job 失败");
			return false;
		}
		L_Job->JobName = L_Candidate.CandidateId;
		L_Job->Sequence = FSoftObjectPath(L_Sequence);
		L_Job->Map = FSoftObjectPath(L_RenderWorld);
		if (L_BaseConfig != nullptr)
		{
			L_Job->SetConfiguration(L_BaseConfig);
		}
		ApplyCandidateSettings(*L_Job, L_Candidate, L_FrameRate, L_BaseConfig != nullptr);
	}

	UMoviePipelinePIEExecutor* L_Executor = NewObject<UMoviePipelinePIEExecutor>(GetTransientPackage());
	if (L_Executor == nullptr)
	{
		OutError = TEXT("建 PIE 渲染执行器失败");
		return false;
	}
	L_Executor->AddToRoot();
	L_Executor->SetIsRenderingOffscreen(true);
	L_Executor->SetAllowUsingUnsavedLevels(false);
	L_Executor->OnIndividualJobStarted().AddUObject(this, &UDoodleLightingRoundRunner::OnIndividualJobStarted);
	L_Executor->OnIndividualJobWorkFinished().AddUObject(this, &UDoodleLightingRoundRunner::OnIndividualJobWorkFinished);
	L_Executor->OnExecutorFinished().AddUObject(this, &UDoodleLightingRoundRunner::OnExecutorFinished);

	UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 起渲染：%d 个候选，关卡 %s，序列 %s"),
	       Payload.Candidates.Num(), *Payload.RenderMap, *Payload.LevelSequence);
	L_Subsystem->RenderQueueWithExecutorInstance(L_Executor);
	return true;
}

void UDoodleLightingRoundRunner::FailRound(const FString& InError)
{
	UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] %s"), *InError);
	if (Payload.OutDir.IsEmpty())
	{
		RequestExit(1);
		return;
	}

	FDoodleRoundReport L_Round;
	L_Round.FrameRate = FrameRate > 0 ? FrameRate : GDefaultFrameRate;
	L_Round.Error = InError;

	FString L_WriteError;
	if (!DoodleLightingReport::WriteReportFile(Payload.OutDir, L_Round, L_WriteError))
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] %s"), *L_WriteError);
		RequestExit(1);
		return;
	}
	RequestExit(0);
}

void UDoodleLightingRoundRunner::OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob)
{
	const FString L_CandidateId = InJob != nullptr ? InJob->JobName : FString();
	CandidateStartSeconds.Add(L_CandidateId, FPlatformTime::Seconds());

	FDoodleCandidateReport* L_Report = FindCandidateReport(L_CandidateId);
	const FDoodleTaskCandidate* L_Candidate = FindCandidate(L_CandidateId);
	if (L_Candidate == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] job %s 在载荷里找不到对应候选"), *L_CandidateId);
		if (L_Report != nullptr)
		{
			L_Report->Status = EDoodleRenderStatus::Failed;
		}
		return;
	}

	UWorld* L_World = GEditor != nullptr ? GEditor->PlayWorld : nullptr;
	if (L_World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] 候选 %s：PIE 世界还没起来，灯建不进去"), *L_CandidateId);
		if (L_Report != nullptr)
		{
			L_Report->Status = EDoodleRenderStatus::Failed;
		}
		return;
	}

	const DoodleLightingExecutor::FCandidateRenderResult L_Result =
		DoodleLightingExecutor::ApplyCandidateToWorld(*L_World, *L_Candidate);
	if (L_Report != nullptr)
	{
		L_Report->LightCount = L_Result.LightCount;
		if (!L_Result.bSucceeded)
		{
			L_Report->Status = EDoodleRenderStatus::Failed;
		}
	}
	if (!L_Result.bSucceeded)
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] 候选 %s 建灯失败：%s"), *L_CandidateId, *L_Result.Error);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 候选 %s：%d 盏灯已建进 PIE 世界"), *L_CandidateId, L_Result.LightCount);
	}
}

void UDoodleLightingRoundRunner::OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData)
{
	const FString L_CandidateId = InOutputData.Job != nullptr ? InOutputData.Job->JobName : FString();
	FDoodleCandidateReport* L_Report = FindCandidateReport(L_CandidateId);
	if (L_Report == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DoodleLighting] job %s 渲完了，但载荷里没有这个候选"), *L_CandidateId);
		return;
	}
	if (const double* L_Start = CandidateStartSeconds.Find(L_CandidateId))
	{
		L_Report->RenderMs = static_cast<int64>((FPlatformTime::Seconds() - *L_Start) * 1000.0);
	}

	if (!InOutputData.bSuccess)
	{
		L_Report->Status = EDoodleRenderStatus::Failed;
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] 候选 %s 渲染失败"), *L_CandidateId);
		return;
	}

	const FDoodleTaskCandidate* L_Candidate = FindCandidate(L_CandidateId);
	const int32 L_FrameCount = L_Candidate != nullptr ? CountPngFrames(L_Candidate->FramesDir) : 0;
	L_Report->FrameCount = L_FrameCount;
	if (L_FrameCount < 2)
	{
		L_Report->Status = EDoodleRenderStatus::Failed;
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] 候选 %s 只渲出 %d 帧，合成不出视频"), *L_CandidateId, L_FrameCount);
		return;
	}
	if (L_Report->Status != EDoodleRenderStatus::Failed)
	{
		L_Report->Status = EDoodleRenderStatus::Rendered;
	}
	UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 候选 %s：%d 帧，耗时 %lld ms"),
	       *L_CandidateId, L_FrameCount, L_Report->RenderMs);
}

void UDoodleLightingRoundRunner::OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess)
{
	(void)InExecutor;
	(void)bSuccess;

	FDoodleRoundReport L_Round;
	L_Round.Candidates = CandidateReports;
	L_Round.FrameRate = FrameRate > 0 ? FrameRate : GDefaultFrameRate;

	int32 L_RenderedCount = 0;
	int32 L_FailedCount = 0;
	for (const FDoodleCandidateReport& L_Report : CandidateReports)
	{
		if (L_Report.Status == EDoodleRenderStatus::Rendered)
		{
			++L_RenderedCount;
		}
		else
		{
			++L_FailedCount;
		}
	}
	if (L_RenderedCount == 0)
	{
		L_Round.Error = FString::Printf(TEXT("%d 个候选全部渲染失败"), L_FailedCount);
	}

	if (bInteractiveMode)
	{
		UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 本机一轮渲染结束：成功 %d / 失败 %d，%d fps（停在这里，不退出）"),
		       L_RenderedCount, L_FailedCount, L_Round.FrameRate);
		if (RoundFinished.IsBound())
		{
			RoundFinished.Broadcast(L_Round);
		}
		return;
	}

	FString L_WriteError;
	if (!DoodleLightingReport::WriteReportFile(Payload.OutDir, L_Round, L_WriteError))
	{
		UE_LOG(LogTemp, Error, TEXT("[DoodleLighting] %s"), *L_WriteError);
		RequestExit(1);
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 回传报告 %s：成功 %d / 失败 %d，%d fps"),
	       *DoodleLightingReport::MakeReportPath(Payload.OutDir), L_RenderedCount, L_FailedCount, L_Round.FrameRate);
	RequestExit(0);
}

const FDoodleTaskCandidate* UDoodleLightingRoundRunner::FindCandidate(const FString& InCandidateId) const
{
	return Payload.Candidates.FindByPredicate([&InCandidateId](const FDoodleTaskCandidate& InCandidate) {
		return InCandidate.CandidateId == InCandidateId;
	});
}

FDoodleCandidateReport* UDoodleLightingRoundRunner::FindCandidateReport(const FString& InCandidateId)
{
	return CandidateReports.FindByPredicate([&InCandidateId](const FDoodleCandidateReport& InReport) {
		return InReport.CandidateId == InCandidateId;
	});
}
