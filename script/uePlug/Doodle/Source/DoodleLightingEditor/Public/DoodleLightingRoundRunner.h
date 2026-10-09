#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "UObject/Object.h"

#include "DoodleLightingRoundRunner.generated.h"

class UMoviePipelineExecutorBase;
class UMoviePipelineExecutorJob;
struct FMoviePipelineOutputData;

/**
 * AI 灯光一轮渲染的驱动者（编辑器侧）。
 *
 * 为什么不是一个 commandlet：MRQ 靠 `FCoreDelegates::OnBeginFrame` 推进帧，而这个委托全引擎只有
 * `FEngineLoop::Tick` 一处广播（`LaunchEngineLoop.cpp:5663`）；`CommandletHelpers::TickEngine` 只 tick
 * 引擎与 Slate、不广播它 —— 在 commandlet 里管线会被建起来，然后一帧都不渲。而 MRQ 挑世界用的
 * `MoviePipeline::FindCurrentWorld()` 也只认 Game/PIE 世界，看不到 commandlet 里的编辑器世界。
 * 所以这里照 auto_light 渲染步的形状：**无 `-run=`** 启动编辑器（`<uproject> <render_map> -DoodleAiLightParams=<载荷 json>`），
 * 本类挂在 `FSlateApplication::OnPreTick` 上等资产注册表就绪，然后自己建队列与 job、
 * 交给 `UMoviePipelinePIEExecutor` 逐个 job 渲染，渲完写回传报告再退出进程。
 *
 * 每个 job = 一个候选。MRQ 在 PIE 世界里渲染，所以「这个候选的灯」必须在
 * `OnIndividualJobStarted` 里建进 `GEditor->PlayWorld` —— 那个回调在 `UMoviePipeline::Initialize`
 * 之前、PIE 已经起来之后触发，是唯一可靠的注入点（此时建灯，MRQ 的第一帧就带上了它）。
 */
UCLASS()
class UDoodleLightingRoundRunner : public UObject
{
	GENERATED_BODY()

public:
	/// 命令行里有没有 `-DoodleAiLightParams=`。模块启动时用它决定要不要接管这一轮。
	static bool HasAiLightParams();

	/// `FSlateApplication::OnPreTick` 的入口：等资产注册表加载完，然后起渲染。
	/// 载荷从命令行 `-DoodleAiLightParams=<json 路径>` 读（队列路线：工作机起进程的那条）。
	void Main();

	/// 引擎内原生路线（D52/D53）的入口：计划由调用方在**内存里**给（本机客户端读完桥目录后直接调用），
	/// 不经过命令行、也不写报告文件 —— 回传由客户端做（D54 的 `render_report_local`）。
	/// 计划里的路径必须是**绝对路径**（客户端已把 D39 的相对路径拼上本地缓存根）。
	void StartRoundFromLaunchPlan(const FDoodleLaunchPlan& InPlan);

	/// 引擎内原生路线：整轮渲完时触发一次（参数就是本该写进 `render_report.json` 的那份内容）。
	/// 队列路线不触发它 —— 那条路线自己写报告然后退进程。
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundFinished, const FDoodleRoundReport&);
	FOnRoundFinished RoundFinished;

	/// 结束进程。只有「报告根本没写出来」才用非零退出码 —— 工作机看到非零只会报「UE 没有写出渲染报告」。
	/// 引擎内原生路线**不调它**：那条路线渲染完还要留在编辑器里给美术看。
	static void RequestExit(int32 InExitCode);

private:
	/// 「引擎内原生」模式：不写报告文件、渲染完不退出进程（D52/D53/D56）。
	bool bInteractiveMode = false;

	/// 读载荷、载资产、建队列与 job、起 PIE；失败时把原因写进 OutError（好让两个入口各自决定怎么报）。
	void StartRound();

	/// 这一轮没法开始时用它：写一份只带 error 的报告，然后退出。
	void FailRound(const FString& InError);

	/// 每个候选开渲前（`UMoviePipeline::Initialize` 之前、PIE 已起）把该候选的 rig 建进 PIE 世界。
	void OnIndividualJobStarted(UMoviePipelineExecutorJob* InJob);

	/// 单个 job 渲完（成功或失败）记一次账。
	void OnIndividualJobWorkFinished(FMoviePipelineOutputData InOutputData);

	/// 整条队列渲完：收尾、写报告、退出。
	void OnExecutorFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess);

	const FDoodleTaskCandidate* FindCandidate(const FString& InCandidateId) const;
	FDoodleCandidateReport* FindCandidateReport(const FString& InCandidateId);

	/// 数一个目录里的 `.png` 帧数。数不出来返回 0（当失败处理）。
	/// 注意：.cpp 里的实现就叫这个名字；历史上头里曾错写成一个不存在的 `CountRenderFrames`（死声明）。
	static int32 CountPngFrames(const FString& InFramesDir);

	/// 载荷 json 的绝对路径（诊断用）。引擎内原生路线为空。
	FString PayloadPath;

	/// 消费 `Payload`：载资产、建队列与 job、起 PIE。两个入口都走它。
	bool StartRoundInternal(FString& OutError);
	FDoodleTaskPayload Payload;
	/// 与 `Payload.Candidates` 同序同长的回传条目。
	TArray<FDoodleCandidateReport> CandidateReports;
	/// 每个候选的开渲时刻，用来算 `RenderMs`。
	TMap<FString, double> CandidateStartSeconds;

	/// 序列的 display rate。工作机按它合成视频，取不到就退回 25。
	int32 FrameRate = 25;

	bool bRoundStarted = false;
};
