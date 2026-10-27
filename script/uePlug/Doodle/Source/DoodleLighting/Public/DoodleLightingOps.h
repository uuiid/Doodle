#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "Engine/Scene.h"

struct FDoodleAnchorFrame
{
	FVector CameraLocation = FVector::ZeroVector;
	FVector CameraForward = FVector::ForwardVector;
	FVector SubjectPoint = FVector::ZeroVector;
	bool bHasSubjectPoint = false;
};

struct FDoodleLightPlacement
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;

	double SubjectDistanceCm = 0.0;
};

enum class EDoodleUeLightKind : uint8
{
	Point,
	Spot,
	Rect,
	Directional,
	Sky
};

namespace DoodleLightingOps
{
	DOODLELIGHTING_API EDoodleUeLightKind ResolveUeLightKind(EDoodleLightType InType);

	DOODLELIGHTING_API ELightUnits ResolveIntensityUnits(EDoodleIntensityUnit InUnit);

	DOODLELIGHTING_API float ResolveSourceRadiusCm(const FDoodleLight& InLight);

	DOODLELIGHTING_API float ResolveDirectionalSourceAngleDeg(const FDoodleLight& InLight);

	DOODLELIGHTING_API void ResolveSpotConeDeg(const FDoodleLight& InLight, float& OutInnerDeg, float& OutOuterDeg);

	DOODLELIGHTING_API bool ResolveLightPlacement(
	    const FDoodleLight& InLight,
	    const FDoodleAnchorFrame& InFrame,
	    FDoodleLightPlacement& OutPlacement,
	    FString& OutError,
	    TArray<FString>& OutWarnings);

	DOODLELIGHTING_API FVector MakeFallbackSubjectPoint(const FVector& InCameraLocation, const FVector& InCameraForward);
}
