#include "DoodleLightingOps.h"

namespace
{
	/// IR 的 `offset_t::distance_m` 默认 3m（rig.h:265 的默认值），这里做兜底。
	constexpr double GDefaultDistanceM = 3.0;
	constexpr float GDefaultSpotOuterDeg = 45.0f;
	constexpr float GDefaultSpotInnerDeg = 25.0f;
	/// 平行光的「太阳张角」软硬区间：0.5° 是硬太阳，12° 已经非常软。
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
				// UE 5.7 的签名是 GetSafeNormal(Tolerance, ResultIfZero)：传 FVector 当第一个实参
				// 会去匹配 Tolerance（double）→ C2664。零向量兜底必须走第二个形参。
				return InFrame.CameraForward.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
		}
	}
}  // namespace

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
	// 显式单位是 §7.5 第 5 条：不许让 UE 按「当前默认」替我们决定量纲。
	switch (InUnit)
	{
		case EDoodleIntensityUnit::Candela: return ELightUnits::Candelas;
		case EDoodleIntensityUnit::Lumen: return ELightUnits::Lumens;
		// UE 5.7 的 ELightUnits 只有 Unitless/Candelas/Lumens/EV/Nits（Scene.h:74-81），
		// 没有照度单位 Lux；IR 的 lux 只能落到最接近的亮度单位 Nits（非同量纲，执行器按此近似处理）。
		case EDoodleIntensityUnit::Lux: return ELightUnits::Nits;
		case EDoodleIntensityUnit::Unitless:
		default: return ELightUnits::Unitless;
	}
}

float DoodleLightingOps::ResolveSourceRadiusCm(const FDoodleLight& InLight)
{
	const double LRadius = FMath::Max(0.0, InLight.Shape.SourceRadiusCm);
	const double LSoftness = FMath::Clamp(InLight.Shadows.Softness, 0.0, 1.0);
	// softness = 0.5 → 系数 0.9，基本沿用 IR 里写好的来源半径；两端分别硬化和柔化。
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
	// IR 的锥角是 optional：服务端建基线 rig 时留空（ai_light_seam_candidates.cpp:446-448 的交待）。
	// TOptional 不能直接跟字面量比较、也不能 static_cast（C2678/C2440），必须先 IsSet 再解引用；
	// 另外「值是 0」与「没填」在这里同义，所以判 IsSet 后再看是否 > 0。
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
	return InCameraLocation + InCameraForward.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector) * 500.0;  // 5m
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

	// 这两个字段需要「场景里的具体对象」语义，服务端不填；填了也只能告警后按锚点算。
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

	// 方位角绕世界上轴旋转「方位角零点」：相机空间就是相机前向，向右为正（UE 里 +Yaw 即向右）。
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
