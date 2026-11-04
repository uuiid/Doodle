#include "DoodleWebBridge.h"

#include "DoodleKitsuClient.h"
#include "DoodleLightingEditorLog.h"
#include "DoodleLightingJson.h"
#include "DoodleLightingOps.h"
#include "DoodleLightingRoundRunner.h"
#include "DoodleLightingSettings.h"

#include "CineCameraActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/SpotLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	FString GetCurrentMapPath()
	{
		const UWorld* L_World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (L_World == nullptr)
		{
			return FString();
		}
		return L_World->GetPathName();
	}

	ACineCameraActor* FindSceneCamera(UWorld& InWorld)
	{
		for (TActorIterator<ACineCameraActor> L_It(&InWorld); L_It; ++L_It)
		{
			return *L_It;
		}
		return nullptr;
	}

	FDoodleOffset MakeOffsetFromLocation(const FVector& InLocation, const FVector& InCameraLocation,
	                                     const FVector& InCameraForward)
	{
		const FVector L_Anchor = DoodleLightingOps::MakeFallbackSubjectPoint(InCameraLocation, InCameraForward);
		const FVector L_Delta = InLocation - L_Anchor;
		const double L_DistanceM = L_Delta.Size() / 100.0;

		FDoodleOffset L_Offset;
		L_Offset.DistanceM = L_DistanceM;
		if (L_DistanceM < 0.01)
		{
			return L_Offset;
		}

		const FVector L_Direction = L_Delta.GetSafeNormal();
		const FVector L_Forward = InCameraForward.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector L_Right = FVector::CrossProduct(L_Forward, FVector::UpVector).GetSafeNormal();

		L_Offset.AzimuthDeg = FMath::RadiansToDegrees(FMath::Atan2(L_Direction | L_Right, L_Direction | L_Forward));
		L_Offset.ElevationDeg = FMath::Clamp(
			FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(L_Direction.Z, -1.0, 1.0))), -89.0, 89.0);
		return L_Offset;
	}

	EDoodleLightMobility ToDoodleMobility(const ULightComponent& InComponent)
	{
		switch (InComponent.Mobility)
		{
			case EComponentMobility::Static:
				return EDoodleLightMobility::StaticLight;
			case EComponentMobility::Stationary:
				return EDoodleLightMobility::Stationary;
			case EComponentMobility::Movable:
			default:
				return EDoodleLightMobility::Movable;
		}
	}

	double ResolveSoftness(const ULightComponent& InComponent)
	{
		if (const UDirectionalLightComponent* L_Directional = Cast<UDirectionalLightComponent>(&InComponent))
		{
			return FMath::Clamp(L_Directional->LightSourceAngle / 12.0, 0.0, 1.0);
		}
		if (const UPointLightComponent* L_Point = Cast<UPointLightComponent>(&InComponent))
		{
			return FMath::Clamp(L_Point->SourceRadius * 2.0 / 100.0, 0.0, 1.0);
		}
		return 0.5;
	}

	void ApplyCommonComponent(FDoodleLight& InOutLight, const ULightComponent& InComponent)
	{
		InOutLight.Intensity.Value = InComponent.Intensity;
		InOutLight.Shadows.bCast = InComponent.CastShadows;
		InOutLight.Shadows.Softness = ResolveSoftness(InComponent);
		InOutLight.Mobility = ToDoodleMobility(InComponent);

		if (InComponent.bUseTemperature)
		{
			InOutLight.Color.Mode = EDoodleColorMode::Temperature;
			InOutLight.Color.Kelvin = InComponent.Temperature;
		}
		else
		{
			const FLinearColor L_Color = InComponent.GetLightColor();
			InOutLight.Color.Mode = EDoodleColorMode::RGB;
			InOutLight.Color.RGB = FDoodleVec3(L_Color.R, L_Color.G, L_Color.B);
		}

		const FLightingChannels& L_Channels = InComponent.LightingChannels;
		InOutLight.Channels = {L_Channels.bChannel0 ? 1 : 0, L_Channels.bChannel1 ? 1 : 0,
		                       L_Channels.bChannel2 ? 1 : 0};
	}

	FDoodleLight MakeLocalLight(const AActor& InActor, const ULightComponent& InComponent, EDoodleLightType InType,
	                            const FVector& InCameraLocation, const FVector& InCameraForward)
	{
		FDoodleLight L_Light;
		L_Light.Id = InActor.GetName();
		L_Light.Role = EDoodleLightRole::Fill;
		L_Light.Type = InType;
		L_Light.Origin = EDoodleLightOrigin::Artist;
		L_Light.Anchor.Space = EDoodleAnchorSpace::Camera;
		L_Light.Offset = MakeOffsetFromLocation(InActor.GetActorLocation(), InCameraLocation, InCameraForward);
		ApplyCommonComponent(L_Light, InComponent);

		switch (InType)
		{
			case EDoodleLightType::Spot:
				L_Light.Intensity.Unit = EDoodleIntensityUnit::Candela;
				if (const USpotLightComponent* L_Spot = Cast<USpotLightComponent>(&InComponent))
				{
					L_Light.Shape.InnerConeDeg = L_Spot->InnerConeAngle;
					L_Light.Shape.OuterConeDeg = L_Spot->OuterConeAngle;
				}
				break;
			case EDoodleLightType::Rect:
				L_Light.Intensity.Unit = EDoodleIntensityUnit::Lumen;
				if (const URectLightComponent* L_Rect = Cast<URectLightComponent>(&InComponent))
				{
					L_Light.Shape.SourceRadiusCm = FMath::Max(L_Rect->SourceWidth, L_Rect->SourceHeight) * 0.5;
				}
				break;
			case EDoodleLightType::Point:
			default:
				L_Light.Intensity.Unit = EDoodleIntensityUnit::Candela;
				if (const UPointLightComponent* L_Point = Cast<UPointLightComponent>(&InComponent))
				{
					L_Light.Shape.SourceRadiusCm = L_Point->SourceRadius;
				}
				break;
		}
		return L_Light;
	}

	FDoodleLight MakeDirectionalLight(const ADirectionalLight& InActor, const UDirectionalLightComponent& InComponent)
	{
		FDoodleLight L_Light;
		L_Light.Id = InActor.GetName();
		L_Light.Role = EDoodleLightRole::Key;
		L_Light.Type = EDoodleLightType::Directional;
		L_Light.Origin = EDoodleLightOrigin::Artist;
		L_Light.Anchor.Space = EDoodleAnchorSpace::World;
		L_Light.Offset = FDoodleOffset();
		L_Light.Offset.DistanceM = 0.0;
		ApplyCommonComponent(L_Light, InComponent);
		L_Light.Intensity.Unit = EDoodleIntensityUnit::Lux;
		return L_Light;
	}

	FDoodleLight MakeSkyLight(const ASkyLight& InActor, const USkyLightComponent& InComponent)
	{
		FDoodleLight L_Light;
		L_Light.Id = InActor.GetName();
		L_Light.Role = EDoodleLightRole::Ambient;
		L_Light.Type = EDoodleLightType::Sky;
		L_Light.Origin = EDoodleLightOrigin::Artist;
		L_Light.Anchor.Space = EDoodleAnchorSpace::World;
		L_Light.Offset = FDoodleOffset();
		L_Light.Offset.DistanceM = 0.0;
		L_Light.Intensity.Unit = EDoodleIntensityUnit::Unitless;
		L_Light.Intensity.Value = InComponent.Intensity;
		L_Light.Shadows.bCast = InComponent.CastShadows;
		L_Light.Shadows.Softness = 0.5;
		L_Light.Color.Mode = EDoodleColorMode::Temperature;
		L_Light.Color.Kelvin = 5600.0;
		return L_Light;
	}

	FDoodleGlobals MakeGlobalsFromWorld(UWorld& InWorld)
	{
		FDoodleGlobals L_Globals;

		for (TActorIterator<AExponentialHeightFog> L_It(&InWorld); L_It; ++L_It)
		{
			if (const UExponentialHeightFogComponent* L_Fog = L_It->GetComponent())
			{
				L_Globals.Environment.FogDensity = L_Fog->FogDensity;
			}
			break;
		}

		for (TActorIterator<ASkyLight> L_It(&InWorld); L_It; ++L_It)
		{
			if (const USkyLightComponent* L_Sky = L_It->GetLightComponent())
			{
				L_Globals.Environment.SkyLightIntensity = L_Sky->Intensity;
			}
			break;
		}

		for (TActorIterator<APostProcessVolume> L_It(&InWorld); L_It; ++L_It)
		{
			const FPostProcessSettings& L_Settings = L_It->Settings;
			if (L_Settings.bOverride_AutoExposureBias)
			{
				L_Globals.Exposure.Ev100Bias = L_Settings.AutoExposureBias;
			}
			if (L_Settings.bOverride_WhiteTemp)
			{
				L_Globals.WhiteBalance.Kelvin = L_Settings.WhiteTemp;
			}
			if (L_Settings.bOverride_WhiteTint)
			{
				L_Globals.WhiteBalance.Tint = L_Settings.WhiteTint;
			}
			if (L_Settings.bOverride_ColorContrast)
			{
				L_Globals.Grading.Contrast = L_Settings.ColorContrast.X;
			}
			if (L_Settings.bOverride_ColorSaturation)
			{
				L_Globals.Grading.Saturation = L_Settings.ColorSaturation.X;
			}
			if (L_Settings.bOverride_ColorGain)
			{
				L_Globals.Grading.Lift = L_Settings.ColorGain.X - 1.0;
			}
			if (L_Settings.bOverride_VignetteIntensity)
			{
				L_Globals.Grading.Vignette = L_Settings.VignetteIntensity;
			}
			if (L_Settings.bOverride_BloomIntensity)
			{
				L_Globals.Grading.Bloom = L_Settings.BloomIntensity;
			}
			break;
		}

		return L_Globals;
	}
}

void UDoodleWebBridge::Logout()
{
	UDoodleLightingSettings* L_Settings = GetMutableDefault<UDoodleLightingSettings>();
	L_Settings->Cookies.Reset();
	L_Settings->SaveConfig();
}

FString UDoodleWebBridge::SetAiLightIteration(const FString& InIteration)
{
	TSharedPtr<FJsonObject> L_Root;
	const TSharedRef<TJsonReader<>> L_Reader = TJsonReaderFactory<>::Create(InIteration);
	if (!FJsonSerializer::Deserialize(L_Reader, L_Root) || !L_Root.IsValid())
	{
		const FString L_Error = FString::Printf(TEXT("不是合法 json 对象，长度 %d"), InIteration.Len());
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 解析失败: %s"), *L_Error);
		return L_Error;
	}

	const DoodleKitsuClient::FDoodleIterationResult L_Result = DoodleKitsuClient::ParseIterationBody(L_Root);
	if (L_Result.HasError())
	{
		const FString L_Error = FString::Printf(TEXT("轮次解析失败: %s"), *L_Result.GetError());
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 解析失败: %s"), *L_Error);
		return L_Error;
	}

	const DoodleKitsuClient::FDoodleLightingIterationValue& L_Value = L_Result.GetValue();

	const FString L_CurrentMap = GetCurrentMapPath();
	if (L_CurrentMap.IsEmpty())
	{
		const FString L_Error = TEXT("编辑器里没有打开的关卡，无法校验渲染关卡");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 校验失败: %s"), *L_Error);
		return L_Error;
	}

	if (!L_Value.RenderMap.Equals(L_CurrentMap, ESearchCase::IgnoreCase))
	{
		const FString L_Error = FString::Printf(
			TEXT("渲染关卡不匹配：本轮要求 %s，当前打开的是 %s"),
			L_Value.RenderMap.IsEmpty() ? TEXT("<空>") : *L_Value.RenderMap, *L_CurrentMap
		);
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 校验失败: %s"), *L_Error);
		return L_Error;
	}

	UE_LOG(
		LogDoodleLightingEditor, Log,
		TEXT("ai light iteration 第 %d 轮，state=%s，会话 %s，镜头任务 %s，基线定序器 %s，渲染关卡 %s，候选 %d 个"),
		L_Value.Iteration, *L_Value.State, *L_Value.SessionId, *L_Value.ShotTaskId, *L_Value.BaselineLevel,
		*L_Value.RenderMap, L_Value.Rigs.Num()
	);

	if (L_Value.Rigs.IsEmpty())
	{
		const FString L_Error = TEXT("本轮没有候选灯光方案");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 校验失败: %s"), *L_Error);
		return L_Error;
	}

	UDoodleLightingRoundRunner* L_Runner = NewObject<UDoodleLightingRoundRunner>(GetTransientPackage());
	L_Runner->AddToRoot();
	L_Runner->RoundFinished.AddWeakLambda(L_Runner, [L_Runner](const TArray<FDoodleRenderedCandidate>& InRendered)
	{
		L_Runner->RemoveFromRoot();
	});

	const DoodleLightingExecutor::FRoundRenderResultOrError L_StartResult = L_Runner->StartRender(L_Value);
	if (L_StartResult.HasError())
	{
		L_Runner->RemoveFromRoot();
		L_Runner->RoundFinished.Clear();
		const FString L_Error = FString::Printf(TEXT("起渲染失败: %s"), *L_StartResult.GetError());
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("ai light iteration 起渲染失败: %s"), *L_Error);
		return L_Error;
	}

	return FString();
}

FString UDoodleWebBridge::GetAiLightSceneContext()
{
	UWorld* L_World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (L_World == nullptr)
	{
		const FString L_Error = TEXT("编辑器里没有打开的关卡");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("导出场景灯光失败: %s"), *L_Error);
		return L_Error;
	}

	const ACineCameraActor* L_Camera = FindSceneCamera(*L_World);
	if (L_Camera == nullptr)
	{
		const FString L_Error = TEXT("关卡里没有 CineCameraActor，无法以相机为锚点导出灯光");
		UE_LOG(LogDoodleLightingEditor, Warning, TEXT("导出场景灯光失败: %s"), *L_Error);
		return L_Error;
	}

	const FVector L_CameraLocation = L_Camera->GetActorLocation();
	const FVector L_CameraForward = L_Camera->GetActorRotation().Vector();

	FDoodleLightingRig L_Rig;
	L_Rig.Globals = MakeGlobalsFromWorld(*L_World);

	for (TActorIterator<ADirectionalLight> L_It(L_World); L_It; ++L_It)
	{
		ADirectionalLight* L_Actor = *L_It;
		const UDirectionalLightComponent* L_Component = L_Actor != nullptr ? L_Actor->GetComponent() : nullptr;
		if (L_Component != nullptr)
		{
			L_Rig.Lights.Add(MakeDirectionalLight(*L_Actor, *L_Component));
		}
	}

	for (TActorIterator<ASkyLight> L_It(L_World); L_It; ++L_It)
	{
		ASkyLight* L_Actor = *L_It;
		const USkyLightComponent* L_Component = L_Actor != nullptr ? L_Actor->GetLightComponent() : nullptr;
		if (L_Component != nullptr)
		{
			L_Rig.Lights.Add(MakeSkyLight(*L_Actor, *L_Component));
		}
	}

	for (TActorIterator<ARectLight> L_It(L_World); L_It; ++L_It)
	{
		ARectLight* L_Actor = *L_It;
		const URectLightComponent* L_Component = L_Actor != nullptr ? L_Actor->RectLightComponent : nullptr;
		if (L_Component != nullptr)
		{
			L_Rig.Lights.Add(MakeLocalLight(*L_Actor, *L_Component, EDoodleLightType::Rect, L_CameraLocation, L_CameraForward));
		}
	}

	for (TActorIterator<ASpotLight> L_It(L_World); L_It; ++L_It)
	{
		ASpotLight* L_Actor = *L_It;
		const USpotLightComponent* L_Component = L_Actor != nullptr ? L_Actor->SpotLightComponent : nullptr;
		if (L_Component != nullptr)
		{
			L_Rig.Lights.Add(MakeLocalLight(*L_Actor, *L_Component, EDoodleLightType::Spot, L_CameraLocation, L_CameraForward));
		}
	}

	for (TActorIterator<APointLight> L_It(L_World); L_It; ++L_It)
	{
		APointLight* L_Actor = *L_It;
		const UPointLightComponent* L_Component = L_Actor != nullptr ? L_Actor->PointLightComponent : nullptr;
		if (L_Component != nullptr)
		{
			L_Rig.Lights.Add(MakeLocalLight(*L_Actor, *L_Component, EDoodleLightType::Point, L_CameraLocation, L_CameraForward));
		}
	}

	const TSharedPtr<FJsonObject> L_Root = DoodleLightingJson::SerializeLightingRig(L_Rig);
	FString L_Json;
	const TSharedRef<TJsonWriter<>> L_Writer = TJsonWriterFactory<>::Create(&L_Json);
	FJsonSerializer::Serialize(L_Root.ToSharedRef(), L_Writer);

	UE_LOG(LogDoodleLightingEditor, Log, TEXT("导出场景灯光 %d 盏，相机位置 %s"), L_Rig.Lights.Num(), *L_CameraLocation.ToString());
	return L_Json;
}

UDoodleWebBridge* UDoodleWebBridge::Get()
{
	static UDoodleWebBridge* L_Bridge = []()
	{
		UDoodleWebBridge* L_New = NewObject<UDoodleWebBridge>(GetTransientPackage());
		L_New->AddToRoot();
		return L_New;
	}();
	return L_Bridge;
}
