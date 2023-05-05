#include "Doodle/DoodleAssetsPreview.h"
#include "Components/SkyLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/TextureCube.h"
#include "Components/PostProcessComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/IConsoleManager.h"
ADoodleAssetsPreview::ADoodleAssetsPreview() {
  static ConstructorHelpers::FObjectFinder<UTextureCube> CubemapMap(TEXT("/Doodle/lock_dev/Assets/HDRI/EpicQuadPanorama_Gray.EpicQuadPanorama_Gray"));

  PrimaryActorTick.bCanEverTick = true;
  SkyLight                      = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLightComponent"));

  SkyLight->SetCubemap(CubemapMap.Object);
  SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
  SkyLight->SetSourceCubemapAngle(60.f);
  SkyLight->SkyDistanceThreshold    = 1.f;
  SkyLight->bLowerHemisphereIsBlack = false;
  SkyLight->SetCastRaytracedShadow(true);
  SkyLight->SetMobility(EComponentMobility::Movable);

  RootComponent                                                           = SkyLight;

  PostProcess                                                             = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
  PostProcess->Settings.bOverride_AutoExposureMethod                      = true;
  PostProcess->Settings.AutoExposureMethod                                = EAutoExposureMethod::AEM_Manual;
  PostProcess->Settings.bOverride_AutoExposureBias                        = true;
  PostProcess->Settings.AutoExposureBias                                  = 0.f;
  PostProcess->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
  PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure           = false;

  PostProcess->Settings.bOverride_VignetteIntensity                       = true;
  PostProcess->Settings.VignetteIntensity                                 = 0.4;
  PostProcess->Settings.bOverride_BloomIntensity                          = true;
  PostProcess->Settings.BloomIntensity                                    = 0.675f;
  PostProcess->Settings.bOverride_WhiteTemp                               = true;
  PostProcess->Settings.WhiteTemp                                         = 6500.f;
  PostProcess->Settings.bOverride_ColorContrastShadows                    = true;
  PostProcess->Settings.ColorContrastShadows                              = FVector4{1.0f, 1.f, 1.f, 0.9f};

  PostProcess->Settings.bOverride_FilmToe                                 = true;
  PostProcess->Settings.FilmToe                                           = 0.5f;
  PostProcess->Settings.bOverride_DynamicGlobalIlluminationMethod         = true;
  PostProcess->Settings.DynamicGlobalIlluminationMethod                   = EDynamicGlobalIlluminationMethod::Lumen;
  PostProcess->Settings.bOverride_LumenSceneDetail                        = true;
  PostProcess->Settings.LumenSceneDetail                                  = 4.f;
  PostProcess->Settings.bOverride_LumenFinalGatherQuality                 = true;
  PostProcess->Settings.LumenFinalGatherQuality                           = 4.f;

  PostProcess->Settings.bOverride_RayTracingGI                            = true;
  PostProcess->Settings.RayTracingGIType                                  = ERayTracingGlobalIlluminationType::BruteForce;
  PostProcess->Settings.bOverride_RayTracingGISamplesPerPixel             = true;
  PostProcess->Settings.RayTracingGISamplesPerPixel                       = 8;
  PostProcess->Settings.bOverride_ReflectionMethod                        = true;
  PostProcess->Settings.ReflectionMethod                                  = EReflectionMethod::Lumen;
  PostProcess->Settings.bOverride_LumenRayLightingMode                    = true;
  PostProcess->Settings.LumenRayLightingMode                              = ELumenRayLightingModeOverride::HitLighting;
  PostProcess->Settings.bOverride_LumenFrontLayerTranslucencyReflections  = true;
  PostProcess->Settings.LumenFrontLayerTranslucencyReflections            = false;
  PostProcess->Settings.bOverride_ScreenSpaceReflectionIntensity          = true;
  PostProcess->Settings.ScreenSpaceReflectionIntensity                    = 100.f;
  PostProcess->Settings.bOverride_RayTracingReflectionsMaxBounces         = true;
  PostProcess->Settings.RayTracingReflectionsMaxBounces                   = 3;
  PostProcess->Settings.bOverride_RayTracingReflectionsSamplesPerPixel    = true;
  PostProcess->Settings.RayTracingReflectionsSamplesPerPixel              = 4;

  PostProcess->Settings.bOverride_AmbientOcclusionIntensity               = true;
  PostProcess->Settings.AmbientOcclusionIntensity                         = 0.f;
  PostProcess->Settings.bOverride_AmbientOcclusionPower                   = true;
  PostProcess->Settings.AmbientOcclusionPower                             = 2.f;
  PostProcess->Settings.bOverride_AmbientOcclusionBias                    = true;
  PostProcess->Settings.AmbientOcclusionBias                              = 3.f;
  PostProcess->Settings.bOverride_AmbientOcclusionQuality                 = true;
  PostProcess->Settings.AmbientOcclusionQuality                           = 100.f;
  PostProcess->Settings.bOverride_AmbientOcclusionMipBlend                = true;
  PostProcess->Settings.AmbientOcclusionMipBlend                          = 0.25f;
  PostProcess->Settings.bOverride_AmbientOcclusionMipScale                = true;
  PostProcess->Settings.AmbientOcclusionMipScale                          = 1.f;
  PostProcess->Settings.bOverride_AmbientOcclusionMipThreshold            = true;
  PostProcess->Settings.AmbientOcclusionMipThreshold                      = 0.01f;
  PostProcess->Settings.bOverride_AmbientOcclusionTemporalBlendWeight     = true;
  PostProcess->Settings.AmbientOcclusionTemporalBlendWeight               = 0.1f;
  PostProcess->Settings.bOverride_RayTracingAOSamplesPerPixel             = true;
  PostProcess->Settings.RayTracingAOSamplesPerPixel                       = 8;

  PostProcess->Settings.AutoExposureMethod                                = EAutoExposureMethod::AEM_Manual;
  PostProcess->Settings.ColorContrast                                     = FVector4{1.0f, 1.0f, 1.0f, .9f};
  PostProcess->Settings.FilmToe                                           = 0.5f;
  PostProcess->Settings.AutoExposureBias                                  = ExposureCompensation;
  PostProcess->Settings.AutoExposureApplyPhysicalCameraExposure           = false;
  PostProcess->Settings.VignetteIntensity                                 = 0.3f;

  PostProcess->SetupAttachment(RootComponent);

  DirectionalLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DirectionalLightComponent"));

  DirectionalLight->SetDynamicShadowCascades(3);
  DirectionalLight->DistanceFieldShadowDistance       = 30000.f;
  DirectionalLight->bAtmosphereSunLight               = false;
  DirectionalLight->bUseRayTracedDistanceFieldShadows = false;
  DirectionalLight->SamplesPerPixel                   = 8;
  DirectionalLight->ForwardShadingPriority            = 100;
  DirectionalLight->SetRelativeLocation(FVector{0.f, 0.f, -550.f});
  DirectionalLight->SetupAttachment(RootComponent);

  SkyDome = CreateDefaultSubobject<UStaticMeshComponent>("SkyDome");
  /// Script/Engine.StaticMesh'/Doodle/lock_dev/Assets/EnviroDome.EnviroDome'
  /// Script/Engine.Material'/Doodle/lock_dev/Assets/BackDrop_M.BackDrop_M'
  static ConstructorHelpers::FObjectFinder<UStaticMesh> SkyDome_Mesh(TEXT("/Doodle/lock_dev/Assets/EnviroDome.EnviroDome"));
  static ConstructorHelpers::FObjectFinder<UMaterial> SkyDome_Mat(TEXT("/Doodle/lock_dev/Assets/BackDrop_M.BackDrop_M"));
  SkyDome->SetStaticMesh(SkyDome_Mesh.Object);
  SkyDome->bVisibleInRayTracing         = false;
  SkyDome->CastShadow                   = false;
  SkyDome->bAffectDistanceFieldLighting = false;
  SkyDome->bCastDynamicShadow           = false;
  SkyDome->bRenderCustomDepth           = true;
  SkyDome->CustomDepthStencilValue      = 1;
  SkyDome->SetCollisionProfileName(FName{TEXT("NoCollision")});
  SkyDome->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
  SkyDome->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
  SkyDome->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
  SkyDome->SetWorldScale3D(FVector{1.0f, 1.0f, 0.5f} * SkyDome->Bounds.SphereRadius);

  SkyDome->SetupAttachment(RootComponent);

  /// 参考物体
  RootStaticMeshs = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
  RootStaticMeshs->SetRelativeLocation(FVector{-6000.f, 0.f, 0.f});
  RootStaticMeshs->SetupAttachment(RootComponent);

  {  // [0]
    static ConstructorHelpers::FObjectFinder<UStaticMesh> L_StaticMesh_Obj(TEXT("/Engine/EditorMeshes/ColorCalibrator/SM_ColorCalibrator.SM_ColorCalibrator"));

    auto& L_S = StaticMeshs.Emplace_GetRef(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SM_ColorCalibrator")));
    L_S->SetStaticMesh(L_StaticMesh_Obj.Object);
    L_S->SetupAttachment(RootComponent);
    L_S->SetRelativeLocation({-160.f, 0.f, L_StaticMesh_Obj.Object->GetBounds().GetBox().GetExtent().Z });
  }

  {  // [1]
    static ConstructorHelpers::FObjectFinder<UStaticMesh> L_StaticMesh_Obj(TEXT("/Doodle/lock_dev/Assets/Character_STM.Character_STM"));

    auto& L_S = StaticMeshs.Emplace_GetRef(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Character_STM")));
    L_S->SetStaticMesh(L_StaticMesh_Obj.Object);
    L_S->SetupAttachment(RootComponent);
    L_S->SetRelativeLocation({-245.f, 0.f, 0.f });
  }

  {  // [2]
    static ConstructorHelpers::FObjectFinder<UStaticMesh> L_StaticMesh_Obj(TEXT("/Engine/BasicShapes/Plane.Plane"));

    auto& L_S = StaticMeshs.Emplace_GetRef(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane")));
    L_S->SetStaticMesh(L_StaticMesh_Obj.Object);
    L_S->SetupAttachment(RootComponent);
    L_S->SetRelativeLocation({-350.f, 0.f, 0.f});
	//L_S->SetRelativeScale3D(FVector{ 100.f,100.f,100.f });
  }
  {  // [3]
    static ConstructorHelpers::FObjectFinder<UStaticMesh> L_StaticMesh_Obj(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    auto& L_S = StaticMeshs.Emplace_GetRef(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cylinder")));
    L_S->SetStaticMesh(L_StaticMesh_Obj.Object);
    L_S->SetupAttachment(StaticMeshs[2]);
    L_S->SetRelativeLocation({0.f, 0.f, 25.f});
    L_S->SetRelativeScale3D({0.175f, 0.175f, 0.5f});
  }
}

void ADoodleAssetsPreview::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction({});
}

void ADoodleAssetsPreview::PostActorCreated() {
  Super::PostActorCreated();
  UMaterial* L_SkyDomeMaterial = LoadObject<UMaterial>(this, TEXT("/Doodle/lock_dev/Assets/BackDrop_M.BackDrop_M"));
  SkyDome_Mat_Inst             = UMaterialInstanceDynamic::Create(L_SkyDomeMaterial, this);
  SkyDome_Mat_Inst->SetScalarParameterValue(TEXT("PDO"), 3000.0f);

  SkyDome->SetMaterial(0, SkyDome_Mat_Inst);
  UMaterial* L_PBRCheckerMaterial = LoadObject<UMaterial>(this, TEXT("/Doodle/lock_dev/Assets/PBR_Checker_M.PBR_Checker_M"));
  PBR_Mat_Inst                    = UMaterialInstanceDynamic::Create(L_PBRCheckerMaterial, this);
  PBR_Mat_Inst->SetScalarParameterValue(TEXT("Max_NoneMetallic"), 0.9f);
  PBR_Mat_Inst->SetScalarParameterValue(TEXT("Min_Metallic"), 0.3f);
  PBR_Mat_Inst->SetScalarParameterValue(TEXT("Min_NoneMetallic"), 0.02f);
  PBRChecker(EnableChecker);

  UMaterial* L_BaseMaterial                = LoadObject<UMaterial>(this, TEXT("/Doodle/lock_dev/Assets/Base_M.Base_M"));
  UMaterialInstanceDynamic* L_Base_MatInst = UMaterialInstanceDynamic::Create(L_BaseMaterial, this);
  L_Base_MatInst->SetScalarParameterValue(TEXT("BaseColor"), 0.21f);
  L_Base_MatInst->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
  L_Base_MatInst->SetScalarParameterValue(TEXT("Specular"), 1.0f);
  L_Base_MatInst->SetScalarParameterValue(TEXT("Roughness"), .0f);
  L_Base_MatInst->SetScalarParameterValue(TEXT("AO"), 1.0f);
  StaticMeshs[2]->SetMaterial(0, L_Base_MatInst);
  StaticMeshs[3]->SetMaterial(0, L_Base_MatInst);
  
  LightingScenarios = EDoodleAssetsPreviewLightModel::MidContrast;
  //this->Mark
  SwitchLight(EDoodleAssetsPreviewLightModel::MidContrast);

  DirectionalLight->SetTransmission(EnableTransmission);
  SwitchConsole();
  ViewCalibrator();
  SwitchStaticMeshs(Calibrator);
}

void ADoodleAssetsPreview::SwitchLight(TEnumAsByte<EDoodleAssetsPreviewLightModel> InModel) {
  switch (InModel) {
    case EDoodleAssetsPreviewLightModel::LowContrast: {
      //(Pitch=-71.685211,Yaw=250.879517,Roll=32.971264)
      DirectionalLight->SetWorldRotation(FRotator{-71.685211f, 250.879517f, 32.971264f});
      DirectionalLight->LightSourceAngle     = 50.f;
      DirectionalLight->LightSourceSoftAngle = 50.f;
      DirectionalLight->Intensity            = 0.8f;
      DirectionalLight->ShadowBias           = 0.25f;
      DirectionalLight->ShadowSlopeBias      = 0.25f;
      SkyLight->SetIntensity(0.42f);
      SetBackgroundValue(BackgroundValueCompensation, 0.18);
    } break;
    case EDoodleAssetsPreviewLightModel::MidContrast: {
      DirectionalLight->SetWorldRotation(FRotator{-45.999348f, 250.000214f, 0.000088f});
      DirectionalLight->LightSourceAngle     = 20.f;
      DirectionalLight->LightSourceSoftAngle = 10.f;
      DirectionalLight->Intensity            = 2.3f;
      DirectionalLight->ShadowBias           = 0.5f;
      DirectionalLight->ShadowSlopeBias      = 0.5f;
      SkyLight->SetIntensity(1.2f);
      SetBackgroundValue(BackgroundValueCompensation, 0.18);
    } break;
    case EDoodleAssetsPreviewLightModel::HighContrast: {
      DirectionalLight->SetWorldRotation(FRotator{-60.f, -110.500160f, 12.080514f});
      DirectionalLight->LightSourceAngle     = 0.5357f;
      DirectionalLight->LightSourceSoftAngle = 0.f;
      DirectionalLight->Intensity            = 7.f;
      DirectionalLight->ShadowBias           = 1.0f;
      DirectionalLight->ShadowSlopeBias      = 1.0f;
      SkyLight->SetIntensity(2.5f);
      SetBackgroundValue(BackgroundValueCompensation, 0.1);
    } break;
    default:
      break;
  }
  DirectionalLight->MarkRenderStateDirty();
  // SkyDome->MarkRenderStateDirty();
  SkyLight->MarkRenderStateDirty();
}

void ADoodleAssetsPreview::PBRChecker(bool InIsEnable) {
  if (InIsEnable) {
    TArray<FWeightedBlendable> LArray{};
    FWeightedBlendable& L_W                        = LArray.Emplace_GetRef();
    L_W.Weight                                     = 1.f;
    L_W.Object                                     = PBR_Mat_Inst;
    PostProcess->Settings.WeightedBlendables.Array = LArray;
  } else {
    PostProcess->Settings.WeightedBlendables.Array.Empty();
  }
}

void ADoodleAssetsPreview::SetBackgroundValue(double InBackgroundValue, double InFarValue) {
  if (!Grounded) SkyDome_Mat_Inst->SetScalarParameterValue(TEXT("PDO"), 3000.0f);
  SkyDome_Mat_Inst->SetScalarParameterValue(TEXT("Mask Radius"), Grounded ? 10000.0f : 0.1f);
  SkyDome_Mat_Inst->SetScalarParameterValue(TEXT("Far Value"), InFarValue * FMath::Exp2(InBackgroundValue));
}

void ADoodleAssetsPreview::ViewCalibrator() {
  if (Grounded) {
    FHitResult L_Hit{};
    if (UKismetSystemLibrary::BoxTraceSingle(GetWorld(), RootStaticMeshs->GetComponentLocation(), FVector{10000.0, 0.f, 0.f}, FVector{500.f, 500.f, 500.f}, FRotator::ZeroRotator, (ETraceTypeQuery)ECollisionChannel::ECC_Visibility, false, {}, EDrawDebugTrace::ForDuration, L_Hit, true)) {
      RootStaticMeshs->SetWorldLocation(FVector{L_Hit.ImpactPoint.X, 0.f, 15.f});
      StaticMeshs[2]->SetVisibility(true);
      StaticMeshs[3]->SetVisibility(true);
    }
  } else {
    RootStaticMeshs->SetWorldLocation(FVector{-6000.f, 0.f, 15.f});
    StaticMeshs[2]->SetVisibility(false);
    StaticMeshs[3]->SetVisibility(false);
  }
}

void ADoodleAssetsPreview::SwitchStaticMeshs(bool InIsEnable) {
  for (auto&& i : StaticMeshs) {
    i->SetVisibility(InIsEnable);
  }
}

void ADoodleAssetsPreview::SwitchConsole() {
  if (auto L_Var = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Shadows"))) {
    auto L_F = L_Var->GetFlags();
    L_Var->SetFlags(EConsoleVariableFlags::ECVF_SetByCode);
    L_Var->Set(RaytracingShadow);
    L_Var->SetFlags(L_F);
  }
  if (auto L_Var = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"))) {
    auto L_F = L_Var->GetFlags();
    L_Var->SetFlags(EConsoleVariableFlags::ECVF_SetByCode);
    L_Var->Set(UseHardWareRaytracing);
    L_Var->SetFlags(L_F);
  }
}

void ADoodleAssetsPreview::TurnLighting_Fun() {
  SkyLight->SetSourceCubemapAngle(60.f - TurnLighting);
  SkyLight->MarkRenderStateDirty();
  FQuat L_Quat{ FVector::ZAxisVector, TurnLighting };
  switch (LightingScenarios) {
  case EDoodleAssetsPreviewLightModel::LowContrast: {
	  L_Quat *= FQuat::MakeFromRotator({ -71.685211f, 250.879517f, 32.971264f });
  } break;
  case EDoodleAssetsPreviewLightModel::MidContrast: {
	  L_Quat *= FQuat::MakeFromRotator({ -45.999348f, 250.000214f, 0.000088f });
  } break;
  case EDoodleAssetsPreviewLightModel::HighContrast: {
	  L_Quat *= FQuat::MakeFromRotator({ -60.f, -110.500160f, 12.080514f });
  } break;
  default:
	  break;
  }
  DirectionalLight->SetRelativeRotation(L_Quat);
  DirectionalLight->MarkRenderStateDirty();
}

#if WITH_EDITOR
void ADoodleAssetsPreview::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangeEvent
) {
  Super::PostEditChangeProperty(PropertyChangeEvent);
  // FName name2 = PropertyChangeEvent.GetPropertyName();
  FName name = PropertyChangeEvent.MemberProperty ? PropertyChangeEvent.MemberProperty->GetFName() : NAME_None;
  // UE_LOG(LogTemp, Log, TEXT("chick name: %s MemberProperty: %s"), *name2.ToString(), *name.ToString());

  if (name == GET_MEMBER_NAME_CHECKED(ThisClass, LightingScenarios) || name == GET_MEMBER_NAME_CHECKED(ThisClass, BackgroundValueCompensation)) {
    SwitchLight(LightingScenarios);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, RaytracingShadow) || name == GET_MEMBER_NAME_CHECKED(ThisClass, UseHardWareRaytracing)) {
    SwitchConsole();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, EnableTransmission)) {
    DirectionalLight->SetTransmission(EnableTransmission);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, ExposureCompensation)) {
    PostProcess->Settings.AutoExposureBias = ExposureCompensation;
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, Grounded)) {
    SwitchLight(LightingScenarios);
    ViewCalibrator();
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, Calibrator)) {
    SwitchStaticMeshs(Calibrator);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, EnableChecker)) {
    PBRChecker(EnableChecker);
  } else if (name == GET_MEMBER_NAME_CHECKED(ThisClass, TurnLighting)) {
    TurnLighting_Fun();
  }
}
void ADoodleAssetsPreview::Tick(float DeltaTime) {
  DirectionalLight->AddRelativeRotation(FQuat{FVector::ZAxisVector, DeltaTime});
}
#endif  // WITH_EDITOR