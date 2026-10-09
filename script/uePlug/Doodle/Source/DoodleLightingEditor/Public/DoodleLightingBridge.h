#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

/**
 * 引擎内原生 AI 灯光模式的**桥目录**（D52/D53）。
 *
 * 为什么要有它：编辑器面板**不连服务端**。UE 的 `FHttpManager` 只在引擎模块加载期读一次证书配置
 * （`HttpManager.cpp:340-404` 的 `FHttpManager::CreateHttpManager`，由 `ModuleManager.cpp:1365-1368`
 * 惰性触发、`:1547-1548` 按 `HttpManagerClassName` 取类），所以「运行时再挂自签证书钩子」的时序不可靠 ——
 * 面板要是自己去请求服务端，就得赌这条路。改成：**面板只写一份「启动计划」到本地目录**，
 * 由本机的 Doodle 客户端（独立进程，走它自己的证书环境）读走、起 UE、收产物、上传、回传。
 *
 * 目录约定（两族目录的归属见 D38）：
 *   - 桥目录是**工作机/本机本地目录**，不是共享盘、不是 UNC；
 *   - 取法：环境变量 `DOODLE_AI_LIGHT_BRIDGE_DIR`，缺省 `<project>/Saved/DoodleAiLight`；
 *   - 文件名即轮次：`launch_plan_<session_uuid>_it<NNN>.json`（本文件与工作机必须逐字一致）。
 *
 * 本类刻意只做「目录 + 文件名 + 读写整份 json 文本」，不解析 json —— 解析在 `DoodleLightingJson` 里，
 * 面板与客户端各自读自己那份类型（面板写 `FDoodleLaunchPlan`，客户端读同一份）。
 */
namespace DoodleLightingBridge
{
	/// 桥目录的绝对路径（会建目录）。环境变量为空时退回 `<project>/Saved/DoodleAiLight`。
	DOODLELIGHTINGEDITOR_API FString GetBridgeDir();

	/// 一份启动计划在桥目录里的绝对路径。轮次号用 `{:03}` 对齐服务端的 `it_<NNN>`。
	DOODLELIGHTINGEDITOR_API FString MakeLaunchPlanPath(const FString& InSessionId, int32 InIteration);

	/// 把启动计划写成 UTF-8（无 BOM）的 json 文件。成功返回 true；失败时 `OutError` 说得清是哪一步。
	DOODLELIGHTINGEDITOR_API bool WriteLaunchPlan(const FDoodleLaunchPlan& InPlan, FString& OutPath, FString& OutError);

	/// 读一份启动计划。文件不存在 / 解析失败都返回 false 并写明原因。
	DOODLELIGHTINGEDITOR_API bool ReadLaunchPlan(const FString& InPath, FDoodleLaunchPlan& OutPlan, FString& OutError);

	/// 计划跑完之后由客户端删掉（面板只管写；消费方负责清理，避免桥目录里堆历史）。
	DOODLELIGHTINGEDITOR_API bool DeleteLaunchPlan(const FString& InPath, FString& OutError);
}
