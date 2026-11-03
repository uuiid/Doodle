#include "DoodleLightingRoundRunner.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DoodleLightingEditorLog.h"
#include "DoodleLightingExecutor.h"
#include "DoodleLightingTypes.h"
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "LevelSequence.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MoviePipelineOutputSetting.h"
#include "MoviePipelinePIEExecutor.h"
#include "MoviePipelinePrimaryConfig.h"
#include "MoviePipelineQueue.h"
#include "MoviePipelineQueueSubsystem.h"
#include "MovieRenderPipelineDataTypes.h"
#include "MovieScene.h"

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
} // namespace

DoodleLightingExecutor::FRoundRenderResultOrError UDoodleLightingRoundRunner::StartRender(
	const DoodleKitsuClient::FDoodleLightingIterationValue& InIteration
)
{
	Iteration = InIteration;
	RenderedCandidates.Reset();
	CurrentIndex = 0;

	if (Iteration.RenderMap.IsEmpty())
	{
		return MakeError(FString(TEXT("渲染关卡为空")));
	}
	if (Iteration.BaselineLevel.IsEmpty())
	{
		return MakeError(FString(TEXT("基线关卡为空")));
	}
	if (Iteration.Rigs.IsEmpty())
	{
		return MakeError(FString(TEXT("本轮没有候选灯光方案")));
	}
	for (int32 L_Index = 0; L_Index < Iteration.Rigs.Num(); ++L_Index)
	{
		const FDoodleLightingRig& L_Rig = Iteration.Rigs[L_Index];
		if (L_Rig.CandidateId.IsEmpty())
		{
			return MakeError(FString::Printf(TEXT("第 %d 个灯光方案没有候选 id"), L_Index));
		}
		if (L_Rig.Lights.IsEmpty())
		{
			return MakeError(FString::Printf(TEXT("候选 %s 里没有灯"), *L_Rig.CandidateId));
		}
	}

	OutDir = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("DoodleLighting"), Iteration.SessionId, FString::FromInt(Iteration.Iteration)
	);

	const DoodleLightingExecutor::FRoundRenderResultOrError L_PrepareResult = PrepareRound();
	if (L_PrepareResult.HasError())
	{
		return L_PrepareResult;
	}

	return StartCandidate(0);
}

DoodleLightingExecutor::FRoundRenderResultOrError UDoodleLightingRoundRunner::PrepareRound()
{
	const FString L_BaselinePackage = NormalizePackagePath(Iteration.BaselineLevel);

	LoadedSequence = LoadObject<ULevelSequence>(nullptr, *L_BaselinePackage);
	if (LoadedSequence == nullptr)
	{
		return MakeError(FString::Printf(TEXT("基线定序器加载失败：%s"), *L_BaselinePackage));
	}

	const FString L_RenderMapPackage = NormalizePackagePath(Iteration.RenderMap);
	LoadedMap = LoadObject<UWorld>(nullptr, *L_RenderMapPackage);
	if (LoadedMap == nullptr)
	{
		return MakeError(FString::Printf(TEXT("渲染关卡加载失败：%s"), *L_RenderMapPackage));
	}

	AnchorFrameRate = ResolveFrameRate(LoadedSequence);
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("序列 %s：display rate %.3f fps"), *L_BaselinePackage,
		AnchorFrameRate.AsDecimal()
	);

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

	BaseConfig = nullptr;
	if (!Iteration.MoviePipelineConfig.IsEmpty())
	{
		BaseConfig = LoadObject<UMoviePipelinePrimaryConfig>(
			nullptr, *NormalizePackagePath(Iteration.MoviePipelineConfig)
		);
		if (BaseConfig == nullptr)
		{
			return MakeError(FString::Printf(TEXT("电影管线配置加载失败：%s"), *Iteration.MoviePipelineConfig));
		}
	}
	else
	{
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("没有指定电影管线配置，本轮按内置画质设置渲染"));
	}

	QueueSubsystem = GEditor != nullptr
		? GEditor->GetEditorSubsystem<UMoviePipelineQueueSubsystem>()
		: nullptr;
	if (QueueSubsystem == nullptr)
	{
		return MakeError(FString(TEXT("拿不到 UMoviePipelineQueueSubsystem，建不了渲染队列")));
	}

	return MakeValue(DoodleLightingExecutor::FRoundRenderResult{});
}

DoodleLightingExecutor::FRoundRenderResultOrError UDoodleLightingRoundRunner::StartCandidate(int32 InIndex)
{
	if (!Iteration.Rigs.IsValidIndex(InIndex))
	{
		return MakeError(FString::Printf(TEXT("候选下标越界：%d"), InIndex));
	}
	const FDoodleLightingRig& L_Rig = Iteration.Rigs[InIndex];

	CurrentIndex = InIndex;
	CandidateRig = L_Rig;
	CandidateSpawned = DoodleLightingExecutor::FCandidateRenderResult();
	CandidateFramesDir = DoodleLightingProtocol::MakeCandidateFramesDir(OutDir, L_Rig.CandidateId);

	UMoviePipelineQueue* L_Queue = QueueSubsystem->GetQueue();
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
	L_Job->JobName = L_Rig.CandidateId;
	L_Job->Sequence = FSoftObjectPath(LoadedSequence);
	L_Job->Map = FSoftObjectPath(LoadedMap);
	L_Job->SetConfiguration(BaseConfig);
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
		LogDoodleLightingEditor, Log, TEXT("起渲染：第 %d 个候选 %s，出图目录 %s"), InIndex, *L_Rig.CandidateId,
		*CandidateFramesDir
	);
	QueueSubsystem->RenderQueueWithExecutorInstance(L_Executor);
	return MakeValue(DoodleLightingExecutor::FRoundRenderResult{});
}

void UDoodleLightingRoundRunner::FinishRound()
{
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("本机一轮渲染结束：%d/%d 个候选渲出视频"), RenderedCandidates.Num(),
		Iteration.Rigs.Num()
	);

	if (RoundFinished.IsBound())
	{
		RoundFinished.Broadcast(RenderedCandidates);
	}
}

void UDoodleLightingRoundRunner::RemoveCandidateFromWorld()
{
	if (CandidateSpawned.SpawnedActors.IsEmpty() && !CandidateSpawned.OwnedPostProcessVolume.IsValid())
	{
		return;
	}
	DoodleLightingExecutor::RemoveCandidate(CandidateSpawned);
	CandidateSpawned = DoodleLightingExecutor::FCandidateRenderResult();
	UE_LOG(LogDoodleLightingEditor, Log, TEXT("候选 %s：灯已从 PIE 世界删除"), *CandidateRig.CandidateId);
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
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s：PIE 世界还没起来，灯建不进去"), *L_CandidateId);
		return;
	}

	const DoodleLightingExecutor::FCandidateRenderResultOrError L_Result =
		DoodleLightingExecutor::ApplyCandidateToWorld(*L_World, CandidateFramesDir, CandidateRig, AnchorCamera);
	if (L_Result.HasError())
	{
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s 建灯失败：%s"), *L_CandidateId, *L_Result.GetError());
		return;
	}
	CandidateSpawned = L_Result.GetValue();
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("候选 %s：%d 盏灯已建进 PIE 世界"), *L_CandidateId,
		CandidateSpawned.LightCount
	);
}

void UDoodleLightingRoundRunner::OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData)
{
	const FString L_CandidateId = InOutputData.Job != nullptr
		? InOutputData.Job->JobName
		: FString();
	const int64 L_RenderMs = static_cast<int64>((FPlatformTime::Seconds() - CandidateStartSeconds) * 1000.0);
	RemoveCandidateFromWorld();

	if (!InOutputData.bSuccess)
	{
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s 渲染失败"), *L_CandidateId);
		return;
	}

	const int32 L_FrameCount = CountPngFrames(CandidateFramesDir);
	if (L_FrameCount < 2)
	{
		UE_LOG(LogDoodleLightingEditor, Error, TEXT("候选 %s 只渲出 %d 帧，合成不出视频"), *L_CandidateId, L_FrameCount);
		return;
	}

	RenderedCandidates.Add(FDoodleRenderedCandidate{CandidateRig.CandidateId, CandidateFramesDir});
	UE_LOG(
		LogDoodleLightingEditor, Log, TEXT("候选 %s：%d 帧，耗时 %lld ms"), *L_CandidateId, L_FrameCount, L_RenderMs
	);
}

void UDoodleLightingRoundRunner::OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess)
{
	(void)InExecutor;
	(void)bSuccess;

	RemoveCandidateFromWorld();

	const int32 L_NextIndex = CurrentIndex + 1;
	if (Iteration.Rigs.IsValidIndex(L_NextIndex))
	{
		const DoodleLightingExecutor::FRoundRenderResultOrError L_NextResult = StartCandidate(L_NextIndex);
		if (!L_NextResult.HasError())
		{
			return;
		}
		UE_LOG(
			LogDoodleLightingEditor, Error, TEXT("候选 %s 起不来：%s"), *Iteration.Rigs[L_NextIndex].CandidateId,
			*L_NextResult.GetError()
		);
	}

	FinishRound();
}
