#include "DoodleLightingCapture.h"

#include "Engine/Scene.h"

namespace DoodleLightingCapture
{
	void ApplyFrozenPostProcessBaseline(FPostProcessSettings& InOut)
	{
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
