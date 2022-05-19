#pragma once

#include "CoreMinimal.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

//这个必须在最后面
#include "DoodleMatrixLight.generated.h"

class USpotLightComponent;
class USceneComponent;

UCLASS()
class DOODLE_API ADoodleMatrixLight : public AActor
{
	GENERATED_BODY()
public:
	ADoodleMatrixLight();

	bool Enabled;

	/**
	 * Total energy that the light emits.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "Intensity", UIMin = "0.0", UIMax = "20.0"))
	float Intensity{200};

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
			  meta = (DisplayName = "FocalAngleInner"))
	float FocalAngleInner;

	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "AttenuationDistance"))
	float AttenuationDistance;

	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "LightWidth"))
	float LightWidth;
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "LightLength"))
	float LightLength;
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "CastShadows"))
	bool CastShadows;

	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "LightSamplesSquared"))
	int LightSamplesSquared{3};
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "LightSamplesQueue"))
	int LightSamplesQueue{8};

	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "SourceRadiusMult"))
	float SourceRadiusMult;
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "CenterOfInterestLength"))
	float CenterOfInterestLength;

	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "Channels"))
	FLightingChannels Channels;
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "SoftRadius"))
	float SoftRadius;
	UPROPERTY(BlueprintReadOnly, interp, Category = Light,
			  meta = (DisplayName = "ShadowBias"))
	float ShadowBias;

  UFUNCTION(BlueprintCallable,   
            meta = (CallInEditor = "true", OverrideNativeName = "测试" ))
  void TEST();

private:
	UPROPERTY()
	TArray<USpotLightComponent *> LightList_;

	UPROPERTY()
	TArray<USceneComponent *> SceneComponentList_;

	void CreateLightSqueue();

	void CreateLightSquare(int InSceneComponentIndex);

	void SetLightAttr();
};
