#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"

/**
 * 载荷 / 回传报文的 JSON 读写。
 *
 * 严格程度按「谁写的」分：
 *   - 派发载荷由我们自己的服务端写（task_payload.h），关键字段缺失 = 派发链路坏了 → 直接报错，别硬跑；
 *   - 回传报文由本工作机写、服务端那边 `parse_round_render_report` 才是权威校验点，这里只保证
 *     自己输出的形状对。
 *
 * 所有解析函数的失败都通过 OutError 返回**带字段路径**的说明（"candidates[3].rig.lights[1].role"），
 * 否则现场只剩一句 "parse failed"，排查成本极高。
 */
namespace DoodleLightingJson
{
	/// 读整份派发载荷。失败时 OutError 给出字段路径。
	DOODLELIGHTING_API bool ParseTaskPayload(const TSharedPtr<FJsonObject>& InRoot, FDoodleTaskPayload& OutPayload, FString& OutError);

	/// 读一个 rig（`candidates[i].rig`）。
	DOODLELIGHTING_API bool ParseLightingRig(const TSharedPtr<FJsonObject>& InRoot, FDoodleLightingRig& OutRig, FString& OutError);

	/// 写整份派发载荷（本地调试用：可以把载荷 dump 出来重放）。
	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeTaskPayload(const FDoodleTaskPayload& InPayload);

	/// 写整轮回传报文（= commandlet 落盘的 render_report.json 内容）。
	DOODLELIGHTING_API TSharedPtr<FJsonObject> SerializeRoundReport(const FDoodleRoundReport& InReport);

	/// 读整轮回传报文（工作机读 commandlet 的输出；也是本模块单测的入口）。
	DOODLELIGHTING_API bool ParseRoundReport(const TSharedPtr<FJsonObject>& InRoot, FDoodleRoundReport& OutReport, FString& OutError);
}
