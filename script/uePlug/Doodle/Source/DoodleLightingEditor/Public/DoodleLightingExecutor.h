#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

/// AI 灯光执行器：把服务端下发的 `rig` 落成 UE 世界里的真实灯光。
///
/// 只剩两个入口，都是给 `UDoodleLightingRoundRunner`（MRQ + PIE）用的：
///   - `PrepareAnchorCamera`：导一次交付机位、求出姿态，随后立刻拆掉 —— 锚点换算要用它。
///   - `ApplyCandidateToWorld`：把某个候选的灯与全局量建进 MRQ 的 PIE 世界（每次先清掉上一个候选建的灯）。
/// 早期那条「换编辑器世界 + SceneCapture 抓一帧静图出 PNG/EXR」的老路径
/// （`PrepareRound`/`FinishRound`/`RenderCandidate`/`GetCameraView`）只由 `-run=DoodleLighting` 的
/// commandlet 调用，已经删除。
///
/// 约束（写死在实现里，不要在这里加开关）：
///   - 只改光：灯 + 全局量（曝光/白平衡/天光/雾/调色）。几何、材质、相机一概不动。
///   - 关卡永不落盘：不 DuplicateAsset、不 SaveLoadedAssets（世界就是命令行打开的那个 map 的 PIE 副本）。
///   - 美术手工灯不删：只清我们自己建出来的 actor（§7.5 的 `ClearAllLight` 会连美术灯一起清掉，不能照抄）。
namespace DoodleLightingExecutor
{
	/// 一次调用的上下文。现在只剩交付机位：关卡由命令行指定，产物目录由工作机定（D38/D39）。
	struct FRoundContext
	{
		/// 交付机位 FBX（服务端 `camera_file_path_`）
		FString CameraFilePath;
	};

	/// 单个候选的渲染结果
	struct FCandidateRenderResult
	{
		bool bSucceeded = false;
		FString Error;
		int32 LightCount = 0;
	};

	/// 导入交付机位、求一次姿态（锚点换算要用它），随后立刻把导入物拆掉 —— 它不该跟着进 PIE。
	bool PrepareAnchorCamera(const FRoundContext& InContext, FString& OutError);

	/// 把某个候选的灯与全局量建进给定世界（MRQ 的 PIE 世界）。每次调用先清掉上一个候选建的灯。
	FCandidateRenderResult ApplyCandidateToWorld(UWorld& InWorld, const FDoodleTaskCandidate& InCandidate);
}
