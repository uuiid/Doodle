#include "DoodleLightingExecutor.h"

#include "DoodleLightingEditorLog.h"

#include "DoodleLightingCapture.h"
#include "DoodleLightingOps.h"

#include "Bindings/MovieSceneCustomBinding.h"
#include "Bindings/MovieSceneSpawnableActorBinding.h"
#include "Bindings/MovieSceneSpawnableBinding.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/SpotLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "LevelSequence.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Tracks/MovieSceneCameraCutTrack.h"

namespace DoodleLightingExecutor
{
	namespace
	{
		using namespace DoodleLightingOps;

		UWorld* G_World = nullptr;

		void RemoveSpawnedActors(const TArray<TStrongObjectPtr<AActor>>& InActors)
		{
			for (const TStrongObjectPtr<AActor>& L_Actor : InActors)
			{
				AActor* L_Raw = L_Actor.Get();
				if (L_Raw != nullptr && !L_Raw->IsPendingKillPending())
				{
					L_Raw->Destroy();
				}
			}
		}

		EComponentMobility::Type ToMobility(EDoodleLightMobility InMobility)
		{
			switch (InMobility)
			{
			case EDoodleLightMobility::StaticLight: return EComponentMobility::Static;
			case EDoodleLightMobility::Stationary: return EComponentMobility::Stationary;
			case EDoodleLightMobility::Movable:
			default: return EComponentMobility::Movable;
			}
		}

		FLinearColor ToLinearColor(const FDoodleLight& InLight, TArray<FString>& OutWarnings)
		{
			switch (InLight.Color.Mode)
			{
			case EDoodleColorMode::RGB:
				return FLinearColor(
					static_cast<float>(InLight.Color.RGB.X),
					static_cast<float>(InLight.Color.RGB.Y),
					static_cast<float>(InLight.Color.RGB.Z));
			case EDoodleColorMode::Filter:
				OutWarnings.Add(FString::Printf(TEXT("灯 %s 用色纸 %s 指定颜色，执行器无法解析色纸名，按白处理"),
				                                *InLight.Id, *InLight.Color.Filter));
				return FLinearColor::White;
			case EDoodleColorMode::Temperature:
			default:
				return FLinearColor::White;
			}
		}

		void ApplyCommonLightProperties(ULightComponent& InComponent, const FDoodleLight& InLight, TArray<FString>& OutWarnings)
		{
			InComponent.SetMobility(ToMobility(InLight.Mobility));
			InComponent.SetCastShadows(InLight.Shadows.bCast);
			InComponent.SetIntensity(static_cast<float>(InLight.Intensity.Value));

			if (InLight.Color.Mode == EDoodleColorMode::Temperature)
			{
				InComponent.SetUseTemperature(true);
				InComponent.SetTemperature(static_cast<float>(InLight.Color.Kelvin));
			}
			else
			{
				InComponent.SetUseTemperature(false);
				InComponent.SetLightColor(ToLinearColor(InLight, OutWarnings), false);
			}

			const bool L_Channel0 = InLight.Channels.IsValidIndex(0) && InLight.Channels[0] != 0;
			const bool L_Channel1 = InLight.Channels.IsValidIndex(1) && InLight.Channels[1] != 0;
			const bool L_Channel2 = InLight.Channels.IsValidIndex(2) && InLight.Channels[2] != 0;
			InComponent.SetLightingChannels(L_Channel0, L_Channel1, L_Channel2);
		}

		AActor* SpawnLightActor(EDoodleUeLightKind InKind, ULightComponent*& OutLightComponent)
		{
			OutLightComponent = nullptr;
			switch (InKind)
			{
			case EDoodleUeLightKind::Point:
			{
				APointLight* L_Actor = G_World->SpawnActor<APointLight>();
				OutLightComponent = L_Actor != nullptr ? L_Actor->PointLightComponent : nullptr;
				return L_Actor;
			}
			case EDoodleUeLightKind::Spot:
			{
				ASpotLight* L_Actor = G_World->SpawnActor<ASpotLight>();
				OutLightComponent = L_Actor != nullptr ? L_Actor->SpotLightComponent : nullptr;
				return L_Actor;
			}
			case EDoodleUeLightKind::Rect:
			{
				ARectLight* L_Actor = G_World->SpawnActor<ARectLight>();
				OutLightComponent = L_Actor != nullptr ? L_Actor->RectLightComponent : nullptr;
				return L_Actor;
			}
			case EDoodleUeLightKind::Directional:
			{
				ADirectionalLight* L_Actor = G_World->SpawnActor<ADirectionalLight>();
				OutLightComponent = L_Actor != nullptr ? L_Actor->GetLightComponent() : nullptr;
				return L_Actor;
			}
			case EDoodleUeLightKind::Sky:
			default:
				return G_World->SpawnActor<ASkyLight>();
			}
		}

		void ApplyLightProperties(AActor* InActor, EDoodleUeLightKind InKind, ULightComponent* InLightComponent,
		                          const FDoodleLight& InLight, TArray<FString>& OutWarnings)
		{
			if (InKind == EDoodleUeLightKind::Sky)
			{
				ASkyLight* L_SkyActor = Cast<ASkyLight>(InActor);
				if (L_SkyActor == nullptr)
				{
					OutWarnings.Add(FString::Printf(TEXT("灯 %s：天空光 actor 生成失败"), *InLight.Id));
					return;
				}
				USkyLightComponent* L_Sky = L_SkyActor->GetLightComponent();
				if (L_Sky == nullptr)
				{
					OutWarnings.Add(FString::Printf(TEXT("灯 %s：天空光没有 USkyLightComponent"), *InLight.Id));
					return;
				}
				if (InLight.Color.Mode != EDoodleColorMode::Temperature)
				{
					OutWarnings.Add(FString::Printf(TEXT("灯 %s：天空光不支持 RGB/色纸颜色，忽略"), *InLight.Id));
				}
				L_Sky->SetMobility(EComponentMobility::Movable);
				L_Sky->SetCastShadows(InLight.Shadows.bCast);
				L_Sky->SetIntensity(static_cast<float>(InLight.Intensity.Value));
				L_Sky->RecaptureSky();
				return;
			}

			if (InLightComponent == nullptr)
			{
				OutWarnings.Add(FString::Printf(TEXT("灯 %s：没有拿到灯组件"), *InLight.Id));
				return;
			}

			ApplyCommonLightProperties(*InLightComponent, InLight, OutWarnings);

			if (InKind == EDoodleUeLightKind::Directional)
			{
				if (UDirectionalLightComponent* L_Directional = Cast<UDirectionalLightComponent>(InLightComponent))
				{
					L_Directional->SetLightSourceAngle(ResolveDirectionalSourceAngleDeg(InLight));
				}
				if (InLight.Intensity.Unit != EDoodleIntensityUnit::Lux)
				{
					OutWarnings.Add(FString::Printf(TEXT("灯 %s：平行光只认 lux，载荷给的是 %s，按 lux 处理"),
					                                *InLight.Id, *DoodleIntensityUnitToString(InLight.Intensity.Unit)));
				}
				return;
			}

			if (ULocalLightComponent* L_Local = Cast<ULocalLightComponent>(InLightComponent))
			{
				L_Local->SetIntensityUnits(ResolveIntensityUnits(InLight.Intensity.Unit));
			}

			if (InKind == EDoodleUeLightKind::Rect)
			{
				if (URectLightComponent* L_Rect = Cast<URectLightComponent>(InLightComponent))
				{
					const float L_SizeCm = 2.0f * ResolveSourceRadiusCm(InLight);
					L_Rect->SourceWidth = L_SizeCm;
					L_Rect->SourceHeight = L_SizeCm;
				}
				return;
			}

			if (UPointLightComponent* L_Point = Cast<UPointLightComponent>(InLightComponent))
			{
				const float L_SourceRadius = ResolveSourceRadiusCm(InLight);
				L_Point->SetSourceRadius(L_SourceRadius);
				L_Point->SetSoftSourceRadius(L_SourceRadius);
				if (USpotLightComponent* L_Spot = Cast<USpotLightComponent>(InLightComponent))
				{
					float L_Inner = 0.0f;
					float L_Outer = 0.0f;
					ResolveSpotConeDeg(InLight, L_Inner, L_Outer);
					L_Spot->SetInnerConeAngle(L_Inner);
					L_Spot->SetOuterConeAngle(L_Outer);
				}
			}
		}

		FDoodleAnchorFrame BuildAnchorFrame(const FAnchorCamera& InCamera)
		{
			FDoodleAnchorFrame L_Frame;
			L_Frame.CameraLocation = InCamera.Location;
			L_Frame.CameraForward = InCamera.Rotation.Vector();
			L_Frame.SubjectPoint = MakeFallbackSubjectPoint(InCamera.Location, L_Frame.CameraForward);
			L_Frame.bHasSubjectPoint = true;
			return L_Frame;
		}

		ACineCameraActor* FindCameraActorByGuid(ULevelSequence& InSequence, UWorld& InWorld, const FGuid& InGuid)
		{
			if (!InGuid.IsValid())
			{
				return nullptr;
			}
			TArray<UObject*, TInlineAllocator<1>> L_Bound;
			UE::UniversalObjectLocator::FResolveParams L_Params(&InWorld);
			InSequence.LocateBoundObjects(InGuid, L_Params, nullptr, L_Bound);
			for (UObject* L_Object : L_Bound)
			{
				if (ACineCameraActor* L_Camera = Cast<ACineCameraActor>(L_Object))
				{
					return L_Camera;
				}
			}
			return nullptr;
		}

		TValueOrError<ACineCameraActor*, FString> FindCameraInSequence(ULevelSequence& InSequence, UWorld& InWorld)
		{
			UMovieScene* L_MovieScene = InSequence.GetMovieScene();
			if (L_MovieScene == nullptr)
			{
				return MakeError(FString(TEXT("定序器里没有 MovieScene")));
			}

			const UMovieSceneCameraCutTrack* L_CutTrack = Cast<UMovieSceneCameraCutTrack>(L_MovieScene->GetCameraCutTrack());
			if (L_CutTrack == nullptr)
			{
				return MakeError(FString(TEXT("定序器里没有相机切换轨道（UMovieSceneCameraCutTrack）")));
			}

			for (UMovieSceneSection* L_Section : L_CutTrack->GetAllSections())
			{
				const UMovieSceneCameraCutSection* L_CutSection = Cast<UMovieSceneCameraCutSection>(L_Section);
				if (L_CutSection == nullptr)
				{
					continue;
				}
				if (ACineCameraActor* L_Camera = FindCameraActorByGuid(
				        InSequence, InWorld, L_CutSection->GetCameraBindingID().GetGuid()))
				{
					return MakeValue(L_Camera);
				}
			}

			return MakeError(FString(TEXT("相机切换轨道里没有能解析到 CineCameraActor 的段")));
		}

		TArray<AActor*> ApplyFogDensity(double InDensity)
		{
			TArray<AActor*> L_Spawned;
			AExponentialHeightFog* L_Fog = nullptr;
			for (TActorIterator<AExponentialHeightFog> L_It(G_World); L_It; ++L_It)
			{
				L_Fog = *L_It;
				break;
			}
			if (L_Fog == nullptr)
			{
				if (InDensity <= 0.0)
				{
					return L_Spawned;
				}
				L_Fog = G_World->SpawnActor<AExponentialHeightFog>();
				if (L_Fog != nullptr)
				{
					L_Spawned.Add(L_Fog);
				}
			}
			if (L_Fog != nullptr && L_Fog->GetComponent() != nullptr)
			{
				L_Fog->GetComponent()->SetFogDensity(static_cast<float>(InDensity));
			}
			return L_Spawned;
		}

		void ApplySkyLightIntensity(double InIntensity)
		{
			ASkyLight* L_SkyLight = nullptr;
			for (TActorIterator<ASkyLight> L_It(G_World); L_It; ++L_It)
			{
				L_SkyLight = *L_It;
				break;
			}
			if (L_SkyLight != nullptr && L_SkyLight->GetLightComponent() != nullptr)
			{
				L_SkyLight->GetLightComponent()->SetIntensity(static_cast<float>(InIntensity));
			}
		}

		void ApplyGrading(FPostProcessSettings& InOut, const FDoodleGrading& InGrading)
		{
			const float L_Contrast = static_cast<float>(InGrading.Contrast);
			const float L_Saturation = static_cast<float>(InGrading.Saturation);
			const float L_Gain = 1.0f + static_cast<float>(InGrading.Lift);
			InOut.bOverride_ColorContrast = true;
			InOut.ColorContrast = FVector4(L_Contrast, L_Contrast, L_Contrast, 1.0);
			InOut.bOverride_ColorSaturation = true;
			InOut.ColorSaturation = FVector4(L_Saturation, L_Saturation, L_Saturation, 1.0);
			InOut.bOverride_ColorGain = true;
			InOut.ColorGain = FVector4(L_Gain, L_Gain, L_Gain, 1.0);
			InOut.bOverride_VignetteIntensity = true;
			InOut.VignetteIntensity = static_cast<float>(InGrading.Vignette);
			InOut.bOverride_BloomIntensity = true;
			InOut.BloomIntensity = static_cast<float>(InGrading.Bloom);
		}

		FPostProcessSettings BuildPostProcessSettings(const FDoodleGlobals& InGlobals)
		{
			FPostProcessSettings L_Settings{};
			DoodleLightingCapture::ApplyFrozenPostProcessBaseline(L_Settings);

			L_Settings.bOverride_AutoExposureBias = true;
			L_Settings.AutoExposureBias = static_cast<float>(InGlobals.Exposure.Ev100Bias);

			L_Settings.bOverride_WhiteTemp = true;
			L_Settings.WhiteTemp = static_cast<float>(InGlobals.WhiteBalance.Kelvin);
			L_Settings.bOverride_WhiteTint = true;
			L_Settings.WhiteTint = static_cast<float>(InGlobals.WhiteBalance.Tint);

			ApplyGrading(L_Settings, InGlobals.Grading);
			return L_Settings;
		}

		APostProcessVolume* EnsurePostProcessVolume(bool& bOutSpawned)
		{
			bOutSpawned = false;
			for (TActorIterator<APostProcessVolume> L_It(G_World); L_It; ++L_It)
			{
				return *L_It;
			}
			APostProcessVolume* L_Volume = G_World->SpawnActor<APostProcessVolume>();
			if (L_Volume != nullptr)
			{
				L_Volume->bEnabled = true;
				L_Volume->bUnbound = true;
				bOutSpawned = true;
			}
			return L_Volume;
		}
	}

	int32 BuildRigLights(const FDoodleLightingRig& InRig, const FAnchorCamera& InCamera, TArray<AActor*>& OutSpawned,
	                     TArray<FString>& OutWarnings)
	{
		const FDoodleAnchorFrame L_AnchorFrame = BuildAnchorFrame(InCamera);
		int32 L_LightCount = 0;
		for (const FDoodleLight& L_Light : InRig.Lights)
		{
			const EDoodleUeLightKind L_Kind = ResolveUeLightKind(L_Light.Type);
			ULightComponent* L_LightComponent = nullptr;
			AActor* L_Actor = SpawnLightActor(L_Kind, L_LightComponent);
			if (L_Actor == nullptr)
			{
				OutWarnings.Add(FString::Printf(TEXT("灯 %s：生成 actor 失败"), *L_Light.Id));
				continue;
			}
			OutSpawned.Add(L_Actor);
			ApplyLightProperties(L_Actor, L_Kind, L_LightComponent, L_Light, OutWarnings);

			if (L_Kind != EDoodleUeLightKind::Sky)
			{
				FDoodleLightPlacement L_Placement;
				FString L_PlacementError;
				if (ResolveLightPlacement(L_Light, L_AnchorFrame, L_Placement, L_PlacementError, OutWarnings))
				{
					L_Actor->SetActorLocationAndRotation(L_Placement.Location, L_Placement.Rotation);
				}
				else
				{
					OutWarnings.Add(FString::Printf(TEXT("灯 %s：锚点换算失败（%s）"), *L_Light.Id, *L_PlacementError));
				}
			}
			++L_LightCount;
		}
		return L_LightCount;
	}

	FAnchorCameraResult PrepareAnchorCamera(const FRoundContext& InContext)
	{
		ULevelSequence* L_Sequence = LoadObject<ULevelSequence>(nullptr, *InContext.BaselineLevel);
		if (L_Sequence == nullptr)
		{
			return MakeError(FString::Printf(TEXT("定序器加载失败：%s"), *InContext.BaselineLevel));
		}
		UWorld* L_EditorWorld = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (L_EditorWorld == nullptr)
		{
			return MakeError(FString(TEXT("编辑器世界还没准备好")));
		}
		G_World = L_EditorWorld;

		const TValueOrError<ACineCameraActor*, FString> L_CameraResult = FindCameraInSequence(*L_Sequence, *L_EditorWorld);
		if (L_CameraResult.HasError())
		{
			return MakeError(L_CameraResult.GetError());
		}
		const ACineCameraActor* L_Camera = L_CameraResult.GetValue();
		const UCineCameraComponent* L_Component = L_Camera->GetCineCameraComponent();
		if (L_Component == nullptr)
		{
			return MakeError(FString(TEXT("定序器里的相机没有 CineCameraComponent")));
		}

		FAnchorCamera L_Anchor;
		L_Anchor.Location = L_Camera->GetActorLocation();
		L_Anchor.Rotation = L_Camera->GetActorRotation();
		L_Anchor.HorizontalFovDeg = L_Component->GetHorizontalFieldOfView();
		if (L_Anchor.HorizontalFovDeg <= 0.0f || L_Anchor.HorizontalFovDeg >= 179.0f)
		{
			return MakeError(FString::Printf(TEXT("相机水平视场角不合法：%f"), L_Anchor.HorizontalFovDeg));
		}

		return MakeValue(L_Anchor);
	}

	FCandidateRenderResultOrError ApplyCandidateToWorld(UWorld& InWorld, const FString& InFramesDir,
	                                                    const FDoodleLightingRig& InRig, const FAnchorCamera& InCamera)
	{
		if (InRig.Lights.IsEmpty())
		{
			return MakeError(FString(TEXT("候选没有灯，无法建灯")));
		}

		FCandidateRenderResult L_Result;
		G_World = &InWorld;

		TArray<FString> L_Warnings;
		TArray<AActor*> L_Spawned;

		const FPostProcessSettings L_Settings = BuildPostProcessSettings(InRig.Globals);
		bool bVolumeSpawned = false;
		APostProcessVolume* L_Volume = EnsurePostProcessVolume(bVolumeSpawned);
		if (L_Volume != nullptr)
		{
			L_Volume->Settings = L_Settings;
			L_Volume->BlendWeight = 1.0f;
			if (bVolumeSpawned)
			{
				L_Result.OwnedPostProcessVolume.Reset(L_Volume);
			}
		}
		ApplySkyLightIntensity(InRig.Globals.Environment.SkyLightIntensity);
		L_Spawned.Append(ApplyFogDensity(InRig.Globals.Environment.FogDensity));

		L_Result.LightCount = BuildRigLights(InRig, InCamera, L_Spawned, L_Warnings);

		L_Result.SpawnedActors.Reserve(L_Spawned.Num());
		for (AActor* L_Actor : L_Spawned)
		{
			L_Result.SpawnedActors.Emplace(L_Actor);
		}

		for (const FString& L_Warning : L_Warnings)
		{
			UE_LOG(LogDoodleLightingEditor, Warning, TEXT("出图目录 %s：%s"), *InFramesDir, *L_Warning);
		}
		return MakeValue(L_Result);
	}

	void RemoveCandidate(const FCandidateRenderResult& InResult)
	{
		RemoveSpawnedActors(InResult.SpawnedActors);
		if (APostProcessVolume* L_Volume = InResult.OwnedPostProcessVolume.Get())
		{
			if (!L_Volume->IsPendingKillPending())
			{
				L_Volume->Destroy();
			}
		}
	}

}
