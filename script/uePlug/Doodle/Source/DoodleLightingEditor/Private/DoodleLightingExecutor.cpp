#include "DoodleLightingExecutor.h"

#include "DoodleLightingCapture.h"
#include "DoodleLightingOps.h"

#include "Bindings/MovieSceneCustomBinding.h"
#include "Bindings/MovieSceneSpawnableBinding.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "CommandletHelpers.h"
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
#include "FbxImporter.h"
#include "GameFramework/WorldSettings.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "MovieSceneBindingReferences.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneToolsUserSettings.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Tracks/MovieSceneSpawnTrack.h"

namespace DoodleLightingExecutor
{
	namespace
	{
		/// 整轮状态。这个执行器一次进程只服务一轮 6 个候选，所以用文件级状态而不是类。
		UWorld* G_World = nullptr;

		ULevelSequence* G_CameraSequence = nullptr;
		ALevelSequenceActor* G_CameraSequenceActor = nullptr;
		ULevelSequencePlayer* G_CameraPlayer = nullptr;
		ACineCameraActor* G_CameraActor = nullptr;
		FGuid G_CameraGuid;
		FVector G_CameraLocation = FVector::ZeroVector;
		FRotator G_CameraRotation = FRotator::ZeroRotator;
		float G_CameraFovDeg = 0.0f;

		/// 我们自己建出来的 actor（每轮/每候选清理，美术手工放的东西一律不碰）
		TArray<TWeakObjectPtr<AActor>> G_AiSpawnedActors;
		TWeakObjectPtr<APostProcessVolume> G_PostProcessVolume;
		bool G_PostProcessVolumeOwned = false;

		// ------------------------------------------------------------------
		// 小工具
		// ------------------------------------------------------------------

		void CollectActor(AActor* InActor)
		{
			if (InActor != nullptr)
			{
				G_AiSpawnedActors.Add(InActor);
			}
		}

		void DestroyAiSpawnedActors()
		{
			if (!G_AiSpawnedActors.IsEmpty())
			{
				for (const TWeakObjectPtr<AActor>& L_Weak : G_AiSpawnedActors)
				{
					if (AActor* L_Actor = L_Weak.Get())
					{
						if (!L_Actor->IsPendingKillPending())
						{
							L_Actor->Destroy();
						}
					}
				}
				G_AiSpawnedActors.Reset();
				if (G_World != nullptr)
				{
					GEngine->ForceGarbageCollection(true);
					GEngine->ConditionalCollectGarbage();
					CommandletHelpers::TickEngine(G_World);
				}
			}
		}

		ELightMobilityType ToMobility(EDoodleLightMobility InMobility)
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
				// 色纸名（字符串）执行器认不出来 —— 契约里说好按白处理 + 告警，不静默乱猜
				OutWarnings.Add(FString::Printf(TEXT("灯 %s 用色纸 %s 指定颜色，执行器无法解析色纸名，按白处理"),
				                                *InLight.Id, *InLight.Color.Filter));
				return FLinearColor::White;
			case EDoodleColorMode::Temperature:
			default:
				return FLinearColor::White; // 走 UseTemperature 分支，这里只是兜底
			}
		}

		/// 灯的公共属性。`ULightComponent::SetIntensity/SetLightColor/SetTemperature/...` 的入参都是 float，
		/// 契约里是 double，这里显式收敛一次。
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

			// 项目约定：主光走 Channel1（`FDoodleLight::Channels` 默认 [0,1,0]）
			const bool L_Channel0 = InLight.Channels.IsValidIndex(0) && InLight.Channels[0] != 0;
			const bool L_Channel1 = InLight.Channels.IsValidIndex(1) && InLight.Channels[1] != 0;
			const bool L_Channel2 = InLight.Channels.IsValidIndex(2) && InLight.Channels[2] != 0;
			InComponent.SetLightingChannels(L_Channel0, L_Channel1, L_Channel2);
		}

		/// 建 actor 并返回它的灯组件（天空光返回 nullptr，它走 USkyLightComponent 分支）
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
				// 天空光只有强度/阴影有意义（颜色是 USkyLightComponent 自己的 LightColor 语义，
				// M1 不动它）：载荷要 RGB/色纸时只告警，不瞎猜
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
					                                *InLight.Id, *DoodleLightIntensityUnitToString(InLight.Intensity.Unit)));
				}
				return;
			}

			if (ULocalLightComponent* L_Local = Cast<ULocalLightComponent>(InLightComponent))
			{
				// SetIntensityUnits 会连带把 bUseInverseSquaredFalloff 切到对应模式（Unitless ↔ false）
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

		FDoodleAnchorFrame BuildAnchorFrame()
		{
			FDoodleAnchorFrame L_Frame;
			L_Frame.CameraLocation = G_CameraLocation;
			L_Frame.CameraForward = G_CameraRotation.Vector();
			L_Frame.SubjectPoint = MakeFallbackSubjectPoint(G_CameraLocation, L_Frame.CameraForward);
			// M1：主体点用「相机前方 5m」的兜底值（没有碰撞体可以做射线命中，见 §15.13 D32）
			L_Frame.bHasSubjectPoint = true;
			return L_Frame;
		}

		// ------------------------------------------------------------------
		// 相机
		// ------------------------------------------------------------------

		bool ResolveCameraView(FString& OutError);

		/// 交付机位：把 `camera_file_path_` 的 FBX 相机导进一个临时 level sequence，求值出位姿与 HFOV。
		/// 做法照抄 `DoodleAutoAnimationCommandlet.cpp:606-688`，唯一区别是序列建在临时包里、不注册成资产。
		bool ImportDeliveryCamera(const FString& InFbxPath, FString& OutError)
		{
			if (InFbxPath.IsEmpty())
			{
				OutError = TEXT("载荷里没有相机文件路径（camera_file_path）");
				return false;
			}
			if (!FPaths::FileExists(InFbxPath))
			{
				OutError = FString::Printf(TEXT("相机 FBX 不存在：%s"), *InFbxPath);
				return false;
			}

			// 临时序列：不 CreatePackage 到 /Game，也不 AssetCreated —— 进程退出即消失
			UPackage* L_Package = CreatePackage(*FString::Printf(TEXT("/Temp/DoodleLightingCamera_%s"), *FGuid::NewGuid().ToString()));
			G_CameraSequence = NewObject<ULevelSequence>(L_Package, TEXT("DoodleLightingCamera"), RF_Public | RF_Transient);
			if (G_CameraSequence == nullptr)
			{
				OutError = TEXT("建临时 level sequence 失败");
				return false;
			}
			G_CameraSequence->Initialize();
			G_CameraSequence->GetMovieScene()->SetDisplayRate(FFrameRate(24, 1));
			G_CameraSequence->GetMovieScene()->SetTickResolutionDirectly(FFrameRate(24000, 1));

			FActorSpawnParameters L_SpawnParams{};
			L_SpawnParams.ObjectFlags &= ~RF_Transactional;
			G_CameraActor = Cast<ACineCameraActor>(G_World->SpawnActor<ACineCameraActor>(L_SpawnParams));
			if (G_CameraActor == nullptr)
			{
				OutError = TEXT("生成交付机位 ACineCameraActor 失败");
				return false;
			}
			// 机位不是「每候选清理」的对象：它整轮存活，不进 G_AiSpawnedActors
			if (UCineCameraComponent* L_CameraComponent = G_CameraActor->GetCineCameraComponent())
			{
				L_CameraComponent->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
				L_CameraComponent->PostProcessSettings.bOverride_MotionBlurAmount = true;
				L_CameraComponent->PostProcessSettings.MotionBlurAmount = 0.1f;
			}

			UMovieScene* L_MovieScene = G_CameraSequence->GetMovieScene();
			G_CameraGuid = L_MovieScene->AddPossessable(G_CameraActor->GetName(), G_CameraActor->GetClass());
			G_CameraSequence->BindPossessableObject(G_CameraGuid, *G_CameraActor, G_World);

			// 自定义绑定 + spawn track：和既有链路一致，否则序列求值不会把 actor 生成/驱动起来
			if (const FMovieSceneBindingReference* L_Reference = G_CameraSequence->GetBindingReferences()->GetReference(G_CameraGuid, 0))
			{
				UMovieSceneCustomBinding* L_Binding = NewObject<UMovieSceneSpawnableActorBinding>()
					->CreateCustomBindingFromBinding(*L_Reference, G_CameraActor, *L_MovieScene);
				L_Binding->SetupDefaults(nullptr, L_Reference->ID, *L_MovieScene);
			}
			UMovieSceneSpawnTrack* L_SpawnTrack = L_MovieScene->AddTrack<UMovieSceneSpawnTrack>(G_CameraGuid);
			if (L_SpawnTrack != nullptr)
			{
				UMovieSceneSpawnSection* L_SpawnSection = CastChecked<UMovieSceneSpawnSection>(L_SpawnTrack->CreateNewSection());
				L_SpawnSection->SetRange(L_MovieScene->GetPlaybackRange());
				L_SpawnTrack->AddSection(*L_SpawnSection);
			}

			// 相机切换段（同时给出后面 FBX 导入要用的绑定 id）
			MovieSceneToolHelpers::CreateCameraCutSectionForCamera(L_MovieScene, G_CameraGuid, FFrameNumber(0));
			FMovieSceneObjectBindingID L_CameraBindingId;
			if (const UMovieSceneCameraCutTrack* L_CutTrack = Cast<UMovieSceneCameraCutTrack>(L_MovieScene->GetCameraCutTrack()))
			{
				const TArray<UMovieSceneSection*>& L_Sections = L_CutTrack->GetAllSections();
				if (const UMovieSceneCameraCutSection* L_CutSection = L_Sections.IsValidIndex(0) ? Cast<UMovieSceneCameraCutSection>(L_Sections[0]) : nullptr)
				{
					L_CameraBindingId = L_CutSection->GetCameraBindingID();
				}
			}
			if (!L_CameraBindingId.GetGuid().IsValid())
			{
				OutError = TEXT("没能建出相机切换段（拿不到 camera binding id）");
				return false;
			}

			ULevelSequencePlayer* L_Player = ULevelSequencePlayer::CreateLevelSequencePlayer(G_World->PersistentLevel, G_CameraSequence,
			                                                                                FMovieSceneSequencePlaybackSettings{}, G_CameraSequenceActor);
			if (L_Player == nullptr || G_CameraSequenceActor == nullptr)
			{
				OutError = TEXT("建 level sequence 播放器失败");
				return false;
			}
			G_CameraPlayer = L_Player;
			G_CameraSequenceActor->AddToRoot();
			G_CameraSequenceActor->InitializePlayer();
			L_Player->Play();

			// FBX 导入（bCreateCameras=false：相机 actor 我们已经建好了，导入只写轨道）
			TMap<FGuid, FString> L_BindingMap{};
			L_BindingMap.Add(L_CameraBindingId.GetGuid(), G_CameraActor->GetActorNameOrLabel());

			FFBXInOutParameters L_FbxParams;
			UMovieSceneUserImportFBXSettings* L_ImportSettings = GetMutableDefault<UMovieSceneUserImportFBXSettings>();
			L_ImportSettings->bMatchByNameOnly = false;
			L_ImportSettings->bForceFrontXAxis = false;
			L_ImportSettings->bConvertSceneUnit = false;
			L_ImportSettings->bCreateCameras = false;
			L_ImportSettings->bReplaceTransformTrack = true;
			L_ImportSettings->bReduceKeys = false;

			if (!MovieSceneToolHelpers::ReadyFBXForImport(InFbxPath, L_ImportSettings, L_FbxParams))
			{
				OutError = FString::Printf(TEXT("FBX 预读失败：%s"), *InFbxPath);
				return false;
			}
			UnFbx::FFbxImporter* L_FbxImporter = UnFbx::FFbxImporter::GetInstance();
			L_FbxImporter->ImportFromFile(*InFbxPath, FPaths::GetExtension(InFbxPath));
			MovieSceneToolHelpers::ImportFBXCameraToExisting(L_FbxImporter, G_CameraSequence, L_Player,
			                                                 L_CameraBindingId.GetRelativeSequenceID(), L_BindingMap, false, false);
			MovieSceneToolHelpers::ImportFBXIfReady(G_World, G_CameraSequence, L_Player,
			                                                L_CameraBindingId.GetRelativeSequenceID(), L_BindingMap, L_ImportSettings, L_FbxParams);
			L_FbxImporter->ReleaseScene();

			CommandletHelpers::TickEngine(G_World);
			return ResolveCameraView(OutError);
		}

		/// 求值机位。主路径：让序列播放器把姿态写到绑定的 actor 上再读 actor。
		/// 兜底：直接求值 3D 变换通道（`DoodleAutoAnimationCommandlet.cpp:701-726` 的读法）。
		bool ResolveCameraView(FString& OutError)
		{
			if (G_CameraActor == nullptr || G_CameraPlayer == nullptr)
			{
				OutError = TEXT("机位没准备好");
				return false;
			}

			G_CameraPlayer->Play();
			CommandletHelpers::TickEngine(G_World);

			G_CameraLocation = G_CameraActor->GetActorLocation();
			G_CameraRotation = G_CameraActor->GetActorRotation();
			if (UCineCameraComponent* L_CameraComponent = G_CameraActor->GetCineCameraComponent())
			{
				G_CameraFovDeg = L_CameraComponent->GetHorizontalFieldOfView();
			}

			// 播放器没驱动到 actor（姿态还是生成时的原点）就从轨道里直接求值
			if (G_CameraLocation.IsNearlyZero() && G_CameraRotation.IsNearlyZero())
			{
				UMovieScene* L_MovieScene = G_CameraSequence != nullptr ? G_CameraSequence->GetMovieScene() : nullptr;
				UMovieScene3DTransformTrack* L_TransformTrack = L_MovieScene != nullptr
					? L_MovieScene->FindTrack<UMovieScene3DTransformTrack>(G_CameraGuid)
					: nullptr;
				if (L_TransformTrack != nullptr)
				{
					const FFrameNumber L_Frame = L_MovieScene->GetPlaybackRange().GetLowerBoundValue();
					for (UMovieSceneSection* L_Section : L_TransformTrack->GetAllSections())
					{
						UMovieScene3DTransformSection* L_TransformSection = Cast<UMovieScene3DTransformSection>(L_Section);
						if (L_TransformSection == nullptr)
						{
							continue;
						}
						FMovieSceneChannelProxy& L_Proxy = L_TransformSection->GetChannelProxy();
						double L_Values[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
						const TCHAR* L_Names[6] = {TEXT("Location.X"), TEXT("Location.Y"), TEXT("Location.Z"),
						                            TEXT("Rotation.X"), TEXT("Rotation.Y"), TEXT("Rotation.Z")};
						for (int32 i = 0; i < 6; ++i)
						{
							if (FMovieSceneDoubleChannel* L_Channel = L_Proxy.GetChannelByName<FMovieSceneDoubleChannel>(L_Names[i]).Get())
							{
								L_Channel->Evaluate(L_Frame, L_Values[i]);
							}
						}
						G_CameraLocation = FVector(L_Values[0], L_Values[1], L_Values[2]);
						G_CameraRotation = FRotator(L_Values[4], L_Values[5], L_Values[3]); // Pitch, Yaw, Roll
					}
				}
			}

			if (G_CameraFovDeg <= 0.0f || G_CameraFovDeg >= 179.0f)
			{
				OutError = FString::Printf(TEXT("相机水平视场角不合法：%f"), G_CameraFovDeg);
				return false;
			}
			return true;
		}

		// ------------------------------------------------------------------
		// 全局量（曝光/白平衡/天光/雾/调色）
		// ------------------------------------------------------------------

		void ApplyFogDensity(double InDensity)
		{
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
					return;
				}
				L_Fog = G_World->SpawnActor<AExponentialHeightFog>();
				CollectActor(L_Fog);
			}
			if (L_Fog != nullptr && L_Fog->GetComponent() != nullptr)
			{
				L_Fog->GetComponent()->SetFogDensity(static_cast<float>(InDensity));
			}
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

		/// 调色映射（§15.13 D34）。基线里已冻结的项（Bloom=0）会被这里的载荷值覆盖 —— 载荷是 AI 的意图。
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

			// 曝光：基线已锁 min == max == 1.0，这里只叠 ev100 偏移（§7.5）
			L_Settings.bOverride_AutoExposureBias = true;
			L_Settings.AutoExposureBias = static_cast<float>(InGlobals.Exposure.Ev100Bias);

			L_Settings.bOverride_WhiteTemp = true;
			L_Settings.WhiteTemp = static_cast<float>(InGlobals.WhiteBalance.Kelvin);
			L_Settings.bOverride_WhiteTint = true;
			L_Settings.WhiteTint = static_cast<float>(InGlobals.WhiteBalance.Tint);

			ApplyGrading(L_Settings, InGlobals.Grading);
			return L_Settings;
		}

		APostProcessVolume* EnsurePostProcessVolume()
		{
			if (APostProcessVolume* L_Existing = G_PostProcessVolume.Get())
			{
				return L_Existing;
			}
			for (TActorIterator<APostProcessVolume> L_It(G_World); L_It; ++L_It)
			{
				G_PostProcessVolume = *L_It;
				return *L_It;
			}
			APostProcessVolume* L_Volume = G_World->SpawnActor<APostProcessVolume>();
			if (L_Volume != nullptr)
			{
				L_Volume->bEnabled = true;
				L_Volume->bUnbound = true;
				G_PostProcessVolume = L_Volume;
				G_PostProcessVolumeOwned = true;
			}
			return L_Volume;
		}
	}

	/// 拆掉交付机位的导入物（播放器、序列 actor、相机 actor）。
	/// 只清导入物：`G_CameraLocation/G_CameraRotation/G_CameraFovDeg` 三个数值保留 —— 锚点换算还要用。
	void ReleaseDeliveryCamera()
	{
		if (G_CameraPlayer != nullptr && G_CameraSequenceActor != nullptr)
		{
			G_CameraPlayer->Stop();
		}
		if (G_CameraSequenceActor != nullptr)
		{
			G_CameraSequenceActor->RemoveFromRoot();
			G_CameraSequenceActor->Destroy();
		}
		if (G_CameraActor != nullptr)
		{
			G_CameraActor->Destroy();
		}
		G_CameraPlayer = nullptr;
		G_CameraSequenceActor = nullptr;
		G_CameraSequence = nullptr;
		G_CameraActor = nullptr;
	}

	/// 按 rig 全量建灯（`rig.ops` 只做审计：`diff_rigs` 永不产生 add/remove，§6.6 同轮 6 个候选灯集合完全一致）。
	/// 返回建出来的灯数。
	int32 BuildRigLights(const FDoodleLightingRig& InRig, TArray<FString>& OutWarnings)
	{
		const FDoodleAnchorFrame L_AnchorFrame = BuildAnchorFrame();
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
			CollectActor(L_Actor);
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

	// ----------------------------------------------------------------------
	// 对外接口
	// ----------------------------------------------------------------------

	bool PrepareAnchorCamera(const FRoundContext& InContext, FString& OutError)
	{
		if (InContext.CameraFilePath.IsEmpty())
		{
			OutError = TEXT("载荷里没有交付机位 FBX（camera_file_path）");
			return false;
		}
		if (GEditor == nullptr)
		{
			OutError = TEXT("没有编辑器实例，导不了交付机位");
			return false;
		}
		// 机位只用来求一次姿态（锚点换算要用它），所以借编辑器世界临时搭一下，求完立刻拆掉：
		// 灯真正建进去的是 MRQ 的 PIE 世界（`ApplyCandidateToWorld`），不是这里。
		G_World = GEditor->GetEditorWorldContext().World();
		if (G_World == nullptr)
		{
			OutError = TEXT("编辑器世界还没准备好");
			return false;
		}

		if (!ImportDeliveryCamera(InContext.CameraFilePath, OutError))
		{
			return false;
		}
		const bool L_Resolved = ResolveCameraView(OutError);
		ReleaseDeliveryCamera();
		if (!L_Resolved)
		{
			return false;
		}
		UE_LOG(LogTemp, Log, TEXT("[DoodleLighting] 交付机位：位置 %s，旋转 %s，水平视场角 %.2f"),
		       *G_CameraLocation.ToCompactString(), *G_CameraRotation.ToCompactString(), G_CameraFovDeg);
		return true;
	}

	FCandidateRenderResult ApplyCandidateToWorld(UWorld& InWorld, const FDoodleTaskCandidate& InCandidate)
	{
		FCandidateRenderResult L_Result;
		// 世界是 MRQ 的 PIE 世界（每个 job 一次），但它是本文件所有建灯辅助函数的落点
		G_World = &InWorld;

		// 上一个候选的 AI 灯先清掉：每个候选都要从「基线 + 本候选的灯」重来，不能叠
		DestroyAiSpawnedActors();

		const FDoodleLightingRig& L_Rig = InCandidate.Rig;
		TArray<FString> L_Warnings;

		// 全局量：曝光/白平衡/调色落 PostProcessVolume（MRQ 渲的就是它），天光与雾落世界里的 actor
		const FPostProcessSettings L_Settings = BuildPostProcessSettings(L_Rig.Globals);
		if (APostProcessVolume* L_Volume = EnsurePostProcessVolume())
		{
			L_Volume->Settings = L_Settings;
			L_Volume->BlendWeight = 1.0f;
		}
		ApplySkyLightIntensity(L_Rig.Globals.Environment.SkyLightIntensity);
		ApplyFogDensity(L_Rig.Globals.Environment.FogDensity);

		L_Result.LightCount = BuildRigLights(L_Rig, L_Warnings);

		for (const FString& L_Warning : L_Warnings)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DoodleLighting] 候选 %d：%s"), InCandidate.CandidateIndex, *L_Warning);
		}
		L_Result.bSucceeded = true;
		return L_Result;
	}

}
