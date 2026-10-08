#include "DoodleLightingCapture.h"

#include "Engine/Scene.h"

namespace DoodleLightingCapture
{
	void ApplyFrozenPostProcessBaseline(FPostProcessSettings& InOut)
	{
		// 逐条对应 `DoodleAutoAnimationCommandlet.cpp:557-603` 的 `PostProcessVolumeConfig`。
		// 这组值是美术成片的验收基准：AI 灯光只是改变光，不该顺带改掉调色/曝光的解释口径。
		InOut.bOverride_BloomIntensity = true;
		InOut.BloomIntensity = 0.0f;

		InOut.bOverride_LumenSceneLightingQuality = true;
		InOut.LumenSceneLightingQuality = 2.0f;
		InOut.bOverride_LumenSceneDetail = true;
		InOut.LumenSceneDetail = 2.0f;
		InOut.bOverride_LumenFinalGatherQuality = true;
		InOut.LumenFinalGatherQuality = 4.0f;
		InOut.bOverride_LumenReflectionQuality = true;
		InOut.LumenReflectionQuality = 2.0f;
		InOut.bOverride_LumenRayLightingMode = true;
		InOut.LumenRayLightingMode = ELumenRayLightingModeOverride::HitLighting;
		InOut.bOverride_LumenMaxReflectionBounces = true;
		InOut.LumenMaxReflectionBounces = 3;

		// 曝光锁（§7.5）：min == max 就是手动锁定，候选的 ev100_bias 再叠在 AutoExposureBias 上
		InOut.bOverride_AutoExposureMinBrightness = true;
		InOut.AutoExposureMinBrightness = 1.0f;
		InOut.bOverride_AutoExposureMaxBrightness = true;
		InOut.AutoExposureMaxBrightness = 1.0f;
		InOut.bOverride_AutoExposureBias = true;
		InOut.AutoExposureBias = 0.0f;

		InOut.bOverride_LocalExposureDetailStrength = true;
		InOut.LocalExposureDetailStrength = 1.1f;
		InOut.bOverride_Sharpen = true;
		InOut.Sharpen = 1.5f;
	}
}
