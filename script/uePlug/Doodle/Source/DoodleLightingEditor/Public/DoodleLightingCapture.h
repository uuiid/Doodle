#pragma once

#include "CoreMinimal.h"
#include "Engine/Scene.h"

/**
 * 冻结的后处理基线：AI 灯光只改光，不该顺带改掉调色/曝光的解释口径。
 *
 * **这里只剩这一件事**。候选产物是视频，出帧交给电影渲染管线（MRQ，见 `UDoodleLightingRoundRunner`）；
 * 早期那条「换编辑器世界 + SceneCapture 抓一帧静图 → PNG/EXR」的老路径已经删掉：
 *   - `FCaptureScope` / `FFramePixels`（两个 `ASceneCapture2D` + 两个 RenderTarget + 回读）
 *   - `WritePng`（`IImageWrapper`，显示空间 sRGB 8bit）/ `WriteExr`（OpenEXR，线性 float）
 *   - `-run=DoodleLighting` 的 `UDoodleLightingCommandlet`（它唯一的调用者）
 * 出图规格（1920×1080、预热帧、EXR 与否）现在只由 MRQ 的设置决定，见 `UDoodleLightingRoundRunner`。
 */
namespace DoodleLightingCapture
{
	/// 冻结的后处理基线（照抄 `DoodleAutoAnimationCommandlet::PostProcessVolumeConfig`，
	/// 那组值是美术成片的验收基准，改了就不是同一个观感了）。
	void DOODLELIGHTINGEDITOR_API ApplyFrozenPostProcessBaseline(FPostProcessSettings& OutSettings);
}
