#include "DoodleLightingRoundRunner.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DoodleLightingEditorLog.h"
#include "DoodleLightingExecutor.h"
#include "DoodleLightingTypes.h"
#include "Editor.h"
#include "Engine/Light.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "LevelSequence.h"
#include "Misc/PackageName.h"
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
#include "MovieScene.h"
#include "MovieSceneSpawnable.h"

namespace
{
	constexpr int32 GDefaultFrameRate = 25;

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

} // namespace

DoodleLightingExecutor::FRoundRenderResultOrError UDoodleLightingRoundRunner::StartRender(
	const FString& InBaselineLevel, const FString& InMoviePipelineConfig, const FString& InOutDir,
	const FDoodleLightingRig& InRig
)
{
	CandidateRig = InRig;
	CandidateReport = FDoodleCandidateReport();

	if (InBaselineLevel.IsEmpty())
	{
		return MakeError(FString(TEXT("基线关卡为空")));
	}
	if (InRig.CandidateId.IsEmpty())
	{
		return MakeError(FString(TEXT("灯光方案没有候选 id")));
	}
	if (InRig.Lights.IsEmpty()) return MakeError(FString(TEXT("灯光方案里没有灯")));

	CandidateFramesDir = DoodleLightingProtocol::MakeCandidateFramesDir(InOutDir, InRig.CandidateId);

	const FString L_BaselinePackage = NormalizePackagePath(InBaselineLevel);

	LoadedSequence = LoadObject<ULevelSequence>(nullptr, *L_BaselinePackage);
	if (LoadedSequence == nullptr)
	{
		return MakeError(FString::Printf(TEXT("基线定序器加载失败：%s"), *L_BaselinePackage));
	}

	bUsingLoadedSequence = LoadedSequence->GetMovieScene() != nullptr && LoadedSequence->GetMovieScene()->IsReadOnly();
	const int32 L_StrippedLights = StripLightSpawnables(*LoadedSequence);
	if (L_StrippedLights > 0 && bUsingLoadedSequence)
	{
		return MakeError(
			FString::Printf(
				TEXT("定序器 %s 是外部引用，改不了，剥不掉里面 %d 个灯的 spawnable"), *L_BaselinePackage, L_StrippedLights
			)
		);
	}

	const FFrameRate L_FrameRate = ResolveFrameRate(LoadedSequence);
	FrameRate = FMath::Max(1, FMath::RoundToInt(L_FrameRate.AsDecimal()));
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("序列 %s：剥掉 %d 个灯 spawnable，display rate %d fps"), *L_BaselinePackage,
		L_StrippedLights, FrameRate
	);

	{
		DoodleLightingExecutor::FRoundContext L_Context;
		L_Context.BaselineLevel = L_BaselinePackage;
		const DoodleLightingExecutor::FAnchorCameraResult L_CameraResult =
			DoodleLightingExecutor::PrepareAnchorCamera(L_Context);
		if (L_CameraResult.HasError())
		{
			return MakeError(FString::Printf(TEXT("机位准备失败：%s"), *L_CameraResult.GetError()));
		}
		AnchorCamera = L_CameraResult.GetValue();
		UE_LOG(
			LogDoodleLightingEditor, Log, TEXT("机位：位置 %s，旋转 %s，水平视场角 %.2f"),
			*AnchorCamera.Location.ToCompactString(), *AnchorCamera.Rotation.ToCompactString(),
			AnchorCamera.HorizontalFovDeg
		);
	}

	UMoviePipelinePrimaryConfig* L_BaseConfig = nullptr;
	if (!InMoviePipelineConfig.IsEmpty())
	{
		L_BaseConfig = LoadObject<UMoviePipelinePrimaryConfig>(nullptr, *NormalizePackagePath(InMoviePipelineConfig));
		if (L_BaseConfig == nullptr)
		{
			return MakeError(FString::Printf(TEXT("电影管线配置加载失败：%s"), *InMoviePipelineConfig));
		}
	}
	else
	{
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("没有指定电影管线配置，本轮按内置画质设置渲染"));
	}

	UMoviePipelineQueueSubsystem* L_Subsystem =
		GEditor != nullptr
		? GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>()
		: nullptr;
	if (L_Subsystem == nullptr)
	{
		return MakeError(FString(TEXT("拿不到 UMoviePipelineQueueSubsystem，建不了渲染队列")));
	}
	UMoviePipelineQueue* L_Queue = L_Subsystem->GetQueue();
	if (L_Queue == nullptr)
	{
		return MakeError(FString(TEXT("电影渲染队列不可用")));
	}
	L_Queue->DeleteAllJobs();

	UMoviePipelineExecutorJob* L_Job = L_Queue->AllocateNewJob(UMoviePipelineExecutorJob::StaticClass());
	if (L_Job == nullptr)
	{
		return MakeError(FString(TEXT("建渲染 job 失败")));
	}
	L_Job->JobName = InRig.CandidateId;
	L_Job->Sequence = FSoftObjectPath(LoadedSequence);
	L_Job->Map = FSoftObjectPath(LoadedMap);
	if (L_BaseConfig != nullptr)
	{
		L_Job->SetConfiguration(L_BaseConfig);
	}
	if (UMoviePipelinePrimaryConfig* L_Config = L_Job->GetConfiguration())
	{
		if (UMoviePipelineOutputSetting* L_Output = Cast<UMoviePipelineOutputSetting>(
			L_Config->FindOrAddSettingByClass(UMoviePipelineOutputSetting::StaticClass())
		))
		{
			L_Output->OutputDirectory.Path = CandidateFramesDir;
		}
	}

	UMoviePipelinePIEExecutor* L_Executor = NewObject<UMoviePipelinePIEExecutor>(GetTransientPackage());
	if (L_Executor == nullptr)
	{
		return MakeError(FString(TEXT("建 PIE 渲染执行器失败")));
	}
	L_Executor->AddToRoot();
	L_Executor->SetIsRenderingOffscreen(true);
	L_Executor->SetAllowUsingUnsavedLevels(false);
	L_Executor->OnIndividualJobStarted().AddUObject(this, &UDoodleLightingRoundRunner::OnIndividualJobStarted);
	L_Executor->OnIndividualJobWorkFinished().AddUObject(this, &UDoodleLightingRoundRunner::OnIndividualJobWorkFinished);
	L_Executor->OnExecutorFinished().AddUObject(this, &UDoodleLightingRoundRunner::OnExecutorFinished);

	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("起渲染：候选 %s，基线关卡 %s，出图目录 %s"), *InRig.CandidateId,
		*L_BaselinePackage, *CandidateFramesDir
	);
	L_Subsystem->RenderQueueWithExecutorInstance(L_Executor);
	return MakeValue(DoodleLightingExecutor::FRoundRenderResult{});
}

void UDoodleLightingRoundRunner::FailRound(const FString& InError)
{
	UE_LOG(LogDoodleLightingEditor, Error, TEXT("%s"), *InError);

	FDoodleRoundReport L_Round;
	L_Round.Candidates.Add(CandidateReport);
	L_Round.FrameRate = FrameRate > 0
		? FrameRate
		: GDefaultFrameRate;
	L_Round.Error = InError;

	if (RoundFinished.IsBound())
	{
		RoundFinished.Broadcast(L_Round);
	}
}

void UDoodleLightingRoundRunner::OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob)
{
	const FString L_CandidateId = InJob != nullptr
		? InJob->JobName
		: FString();
	CandidateStartSeconds = FPlatformTime::Seconds();

	UWorld* L_World = GEditor != nullptr
		? GEditor->PlayWorld
		: nullptr;
	if (L_World == nullptr)
	{
		CandidateReport.Status = EDoodleRenderStatus::Failed;
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s：PIE 世界还没起来，灯建不进去"), *L_CandidateId);
		return;
	}

	const DoodleLightingExecutor::FCandidateRenderResultOrError L_Result =
		DoodleLightingExecutor::ApplyCandidateToWorld(*L_World, CandidateFramesDir, CandidateRig, AnchorCamera);
	if (L_Result.HasError())
	{
		CandidateReport.Status = EDoodleRenderStatus::Failed;
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s 建灯失败：%s"), *L_CandidateId, *L_Result.GetError());
		return;
	}
	CandidateReport.LightCount = L_Result.GetValue().LightCount;
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("候选 %s：%d 盏灯已建进 PIE 世界"), *L_CandidateId,
		L_Result.GetValue().LightCount
	);
}

void UDoodleLightingRoundRunner::OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData)
{
	const FString L_CandidateId = InOutputData.Job != nullptr
		? InOutputData.Job->JobName
		: FString();
	CandidateReport.RenderMs = static_cast<int64>((FPlatformTime::Seconds() - CandidateStartSeconds) * 1000.0);

	if (!InOutputData.bSuccess)
	{
		CandidateReport.Status = EDoodleRenderStatus::Failed;
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s 渲染失败"), *L_CandidateId);
		return;
	}

	const int32 L_FrameCount = CountPngFrames(CandidateFramesDir);
	CandidateReport.FrameCount = L_FrameCount;
	if (L_FrameCount < 2)
	{
		CandidateReport.Status = EDoodleRenderStatus::Failed;
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s 只渲出 %d 帧，合成不出视频"), *L_CandidateId, L_FrameCount);
		return;
	}
	if (CandidateReport.Status != EDoodleRenderStatus::Failed)
	{
		CandidateReport.Status = EDoodleRenderStatus::Rendered;
	}
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("候选 %s：%d 帧，耗时 %lld ms"), *L_CandidateId, L_FrameCount,
		CandidateReport.RenderMs
	);
}

void UDoodleLightingRoundRunner::OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess)
{
	(void)InExecutor;
	(void)bSuccess;

	FDoodleRoundReport L_Round;
	L_Round.Candidates.Add(CandidateReport);
	L_Round.FrameRate = FrameRate > 0
		? FrameRate
		: GDefaultFrameRate;

	const bool bRendered = CandidateReport.Status == EDoodleRenderStatus::Rendered;
	if (!bRendered)
	{
		L_Round.Error = FString::Printf(TEXT("候选 %s 渲染失败"), *CandidateRig.CandidateId);
	}

	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("本机一轮渲染结束：候选 %s %s，%d 帧，%d fps"), *CandidateRig.CandidateId,
		bRendered ? TEXT("成功") : TEXT("失败"), CandidateReport.FrameCount, L_Round.FrameRate
	);
	if (RoundFinished.IsBound())
	{
		RoundFinished.Broadcast(L_Round);
	}
}
