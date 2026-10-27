#include "DoodleLightingOps.h"

namespace
{
	constexpr double GDefaultDistanceM = 3.0;
	constexpr float GDefaultSpotOuterDeg = 45.0f;
	constexpr float GDefaultSpotInnerDeg = 25.0f;

	constexpr float GDirectionalHardDeg = 0.5f;
	constexpr float GDirectionalSoftDeg = 12.0f;

	FVector AnchorPointOf(const FDoodleAnchorFrame& InFrame, EDoodleAnchorSpace InSpace)
	{
		switch (InSpace)
		{
			case EDoodleAnchorSpace::World:
				return FVector::ZeroVector;
			case EDoodleAnchorSpace::Subject:
			case EDoodleAnchorSpace::Camera:
			default:
				return InFrame.SubjectPoint;
		}
	}

	FVector AzimuthZeroOf(const FDoodleAnchorFrame& InFrame, EDoodleAnchorSpace InSpace)
	{
		switch (InSpace)
		{
			case EDoodleAnchorSpace::World:
			case EDoodleAnchorSpace::Subject:
				return FVector::ForwardVector;
			case EDoodleAnchorSpace::Camera:
			default:
				return InFrame.CameraForward.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
		}
	}
}

EDoodleUeLightKind DoodleLightingOps::ResolveUeLightKind(EDoodleLightType InType)
{
	switch (InType)
	{
		case EDoodleLightType::Spot: return EDoodleUeLightKind::Spot;
		case EDoodleLightType::Rect: return EDoodleUeLightKind::Rect;
		case EDoodleLightType::Directional: return EDoodleUeLightKind::Directional;
		case EDoodleLightType::Sky: return EDoodleUeLightKind::Sky;
		case EDoodleLightType::Point:
		default: return EDoodleUeLightKind::Point;
	}
}

ELightUnits DoodleLightingOps::ResolveIntensityUnits(EDoodleIntensityUnit InUnit)
{
	switch (InUnit)
	{
		case EDoodleIntensityUnit::Candela: return ELightUnits::Candelas;
		case EDoodleIntensityUnit::Lumen: return ELightUnits::Lumens;
		case EDoodleIntensityUnit::Lux: return ELightUnits::Nits;
		case EDoodleIntensityUnit::Unitless:
		default: return ELightUnits::Unitless;
	}
}

float DoodleLightingOps::ResolveSourceRadiusCm(const FDoodleLight& InLight)
{
	const double LRadius = FMath::Max(0.0, InLight.Shape.SourceRadiusCm);
	const double LSoftness = FMath::Clamp(InLight.Shadows.Softness, 0.0, 1.0);

	const double LScale = 0.3 + 1.2 * LSoftness;
	return static_cast<float>(FMath::Clamp(LRadius * LScale, 0.0, 200.0));
}

float DoodleLightingOps::ResolveDirectionalSourceAngleDeg(const FDoodleLight& InLight)
{
	const double LSoftness = FMath::Clamp(InLight.Shadows.Softness, 0.0, 1.0);
	return FMath::Lerp(GDirectionalHardDeg, GDirectionalSoftDeg, static_cast<float>(LSoftness));
}

void DoodleLightingOps::ResolveSpotConeDeg(const FDoodleLight& InLight, float& OutInnerDeg, float& OutOuterDeg)
{
	const bool bHasOuter = InLight.Shape.OuterConeDeg.IsSet() && InLight.Shape.OuterConeDeg.GetValue() > 0.0;
	const bool bHasInner = InLight.Shape.InnerConeDeg.IsSet() && InLight.Shape.InnerConeDeg.GetValue() > 0.0;
	OutOuterDeg = bHasOuter ? static_cast<float>(InLight.Shape.OuterConeDeg.GetValue()) : GDefaultSpotOuterDeg;
	OutInnerDeg = bHasInner ? static_cast<float>(InLight.Shape.InnerConeDeg.GetValue())
	                        : OutOuterDeg * (GDefaultSpotInnerDeg / GDefaultSpotOuterDeg);
	OutOuterDeg = FMath::Clamp(OutOuterDeg, 1.0f, 89.0f);
	OutInnerDeg = FMath::Clamp(OutInnerDeg, 0.0f, OutOuterDeg);
}

FVector DoodleLightingOps::MakeFallbackSubjectPoint(const FVector& InCameraLocation, const FVector& InCameraForward)
{
	return InCameraLocation + InCameraForward.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector) * 500.0;
}

bool DoodleLightingOps::ResolveLightPlacement(
    const FDoodleLight& InLight,
    const FDoodleAnchorFrame& InFrame,
    FDoodleLightPlacement& OutPlacement,
    FString& OutError,
    TArray<FString>& OutWarnings)
{
	if (!FMath::IsFinite(InLight.Offset.AzimuthDeg) || !FMath::IsFinite(InLight.Offset.ElevationDeg) || !FMath::IsFinite(InLight.Offset.DistanceM))
	{
		OutError = FString::Printf(TEXT("灯 %s 的 offset 含非有限值"), *InLight.Id);
		return false;
	}

	if (!InLight.Anchor.Target.IsEmpty() || !InLight.Anchor.LookAt.IsEmpty())
	{
		OutWarnings.Add(FString::Printf(
		    TEXT("灯 %s 的 anchor 指定了 target=\"%s\" look_at=\"%s\"，执行器无法解析对象名，已退化为按锚点 + 球面偏移定位"),
		    *InLight.Id, *InLight.Anchor.Target, *InLight.Anchor.LookAt));
	}

	const FVector LAnchorPoint = AnchorPointOf(InFrame, InLight.Anchor.Space);
	const FVector LAzimuthZero = AzimuthZeroOf(InFrame, InLight.Anchor.Space);

	const double LDistanceM = InLight.Offset.DistanceM > 0.0 ? InLight.Offset.DistanceM : GDefaultDistanceM;
	const double LElevationDeg = FMath::Clamp(InLight.Offset.ElevationDeg, -89.0, 89.0);

	const FVector LHorizontal = LAzimuthZero.RotateAngleAxis(InLight.Offset.AzimuthDeg, FVector::UpVector).GetSafeNormal();
	const double LElevationRad = FMath::DegreesToRadians(LElevationDeg);
	const FVector LDirection =
	    (LHorizontal * FMath::Cos(LElevationRad) + FVector::UpVector * FMath::Sin(LElevationRad)).GetSafeNormal();

	const FVector LLocation = LAnchorPoint + LDirection * (LDistanceM * 100.0);
	const FVector LToAnchor = LAnchorPoint - LLocation;

	OutPlacement.Location = LLocation;
	OutPlacement.SubjectDistanceCm = FVector::Dist(LLocation, LAnchorPoint);
	OutPlacement.Rotation = LToAnchor.IsNearlyZero() ? LDirection.Rotation() : LToAnchor.Rotation();
	return true;
}
