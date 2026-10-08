#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

/**
 * 整轮回传报文的落盘 / 读取。
 *
 * 文件路径固定 `<out_dir>/render_report.json`：
 *   - 渲染进程（`UDoodleLightingRoundRunner`；手工跑 commandlet 时也一样）跑完写它；
 *   - 工作机（C++ 服务端侧）在起 UE **之前**先删它 —— 否则渲染进程崩在写报告之前，
 *     工作机会把上一轮的残留当成本轮结果回传（这条是服务端 `ai_light_render.cpp` 的既有约定，
 *     这里只是同一个文件名的另一半）。
 *
 * 接口保持「只碰文件、不碰世界」，方便将来脱离编辑器单测。
 */
namespace DoodleLightingReport
{
	/// `<out_dir>/render_report.json`
	DOODLELIGHTING_API FString MakeReportPath(const FString& InOutDir);

	/// 写报告。目录不存在时创建；失败返回 false 并给出 OS 级原因。
	DOODLELIGHTING_API bool WriteReportFile(const FString& InOutDir, const FDoodleRoundReport& InReport, FString& OutError);

	/// 读报告。给工作机与单测用。
	DOODLELIGHTING_API bool ReadReportFile(const FString& InFilePath, FDoodleRoundReport& OutReport, FString& OutError);
}
