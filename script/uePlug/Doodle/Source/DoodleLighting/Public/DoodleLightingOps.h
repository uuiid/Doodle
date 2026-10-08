#pragma once

#include "CoreMinimal.h"
#include "DoodleLightingTypes.h"
#include "Engine/Scene.h"

/**
 * IR → UE 的纯换算（不碰世界、不碰资产），因此可以脱离编辑器验证。
 *
 * 为什么锚点换算放在执行器（见 `ai_light/rig.h:69-71`）：IR 里的灯位是「相对相机/主体的球面偏移」，
 * 只有执行器知道相机在哪、主体点在哪。服务端刻意不写世界坐标 —— 它不认识场景。
 */

/// 一次渲染内固定的两个参照：相机与主体点。主体点 = 相机前向射线的命中点。
struct FDoodleAnchorFrame
{
	/// 交付机位（相机 Actor 的世界位置）
	FVector CameraLocation = FVector::ZeroVector;
	/// 相机前向（世界空间、单位向量）
	FVector CameraForward = FVector::ForwardVector;
	/// 主体点：相机前向射线命中点；打不到时由调用方给「相机位置 + 前向 × 5m」
	FVector SubjectPoint = FVector::ZeroVector;
	/// 主体点是否是真实命中（false 时日志里要说明是在用兜底点）
	bool bHasSubjectPoint = false;
};

struct FDoodleLightPlacement
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;

	/// 灯到主体点的距离（cm）：调色/日志里用，也方便验证「灯确实落在主体点周围」
	double SubjectDistanceCm = 0.0;
};

/// IR 灯型 → UE 的灯类型分支（Sky 走 SkyLight，不生成 ALight）
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

	/// IR 单位 → UE 的 ELightUnits（unitless 直接映射到 Unitless，不做「帮调用方决定」的换算）
	DOODLELIGHTING_API ELightUnits ResolveIntensityUnits(EDoodleIntensityUnit InUnit);

	/// 阴影软硬 → 点/聚/矩形光的 SourceRadius（cm）。softness=0.5 时大致等于 IR 里的 source_radius_cm。
	DOODLELIGHTING_API float ResolveSourceRadiusCm(const FDoodleLight& InLight);

	/// 阴影软硬 → 平行光的 LightSourceAngle（度）。软的时候太阳张角大。
	DOODLELIGHTING_API float ResolveDirectionalSourceAngleDeg(const FDoodleLight& InLight);

	/// 聚光灯锥角（度）。IR 没给时用默认（外 45 / 内 25）。
	DOODLELIGHTING_API void ResolveSpotConeDeg(const FDoodleLight& InLight, float& OutInnerDeg, float& OutOuterDeg);

	/// 把灯放到世界里。
	///
	/// 方位角：camera 空间以相机前向为 0°、向右为正（UE 里 +Yaw 即向右）；subject 与 world 空间以世界 +X 为 0°。
	/// 仰角：以过锚点的水平面为 0°，向上为正。距离单位是米（IR 里是 `distance_m`）。
	/// 朝向：始终注视锚点（点/聚光如此；平行光只取朝向）。
	///
	/// `anchor.target` / `look_at` 是给「场景里的具体对象」用的名字，服务端建基线 rig 时不会填；
	/// 一旦填了这里无法解析 —— 返回 true 但把 `OutWarnings` 里记一条，由调用方决定要不要告警。
	DOODLELIGHTING_API bool ResolveLightPlacement(
	    const FDoodleLight& InLight,
	    const FDoodleAnchorFrame& InFrame,
	    FDoodleLightPlacement& OutPlacement,
	    FString& OutError,
	    TArray<FString>& OutWarnings);

	/// 主体点兜底：相机前向 × 5m。
	DOODLELIGHTING_API FVector MakeFallbackSubjectPoint(const FVector& InCameraLocation, const FVector& InCameraForward);
}
