#pragma once

#include "CoreMinimal.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

//这个必须在最后面
#include "DoodleMatrixLight.generated.h"


UCLASS()
class DOODLE_API ADoodleMatrixLight : public AActor {
	GENERATED_BODY()
public:
	ADoodleMatrixLight();


	bool Enabled;

	/**
	 * Total energy that the light emits.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
		meta = (DisplayName = "Intensity", UIMin = "0.0", UIMax = "20.0"))
		float Intensity;

	/**
	 * Filter color of the light.
	 * Note that this can change the light's effective intensity.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
		meta = (HideAlphaChannel))
		FColor LightColor;

	/**
	 *
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
		meta = (DisplayName = "FocalAngleOuter"))
		float FocalAngleOuter;

	/**
	 *
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
		meta = (DisplayName = "FocalAngleOuter"))
		float FocalAngleInner;

	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
		meta = (DisplayName = "FocalAngleOuter"))
		float AttenuationDistance;


	float LightWidth;
	float LightLength;
	bool CastShadows;
	int LightSamplesSquared;
	float SourceRadiusMult;
	float CenterOfInterestLength;

	FLightingChannels Channels;
	float SoftRadius;
	float ShadowBias;
};
