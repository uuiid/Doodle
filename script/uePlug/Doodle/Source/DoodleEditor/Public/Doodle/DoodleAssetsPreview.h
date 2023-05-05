#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GameFramework/Actor.h"
// clang-format off
#include "DoodleAssetsPreview.generated.h"
// clang-format on

class USkyLightComponent;
class UDirectionalLightComponent;
class UStaticMeshComponent;
class UPostProcessComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;

UENUM()
enum class EDoodleAssetsPreviewLightModel : uint8 {
  LowContrast UMETA(DisplayName = "LowContrast"),  // 如果想要显示中文，需要设置编码格式为UTF-8
  MidContrast UMETA(DisplayName = "MidContrast"),
  HighContrast UMETA(DisplayName = "HighContrast"),
};

UCLASS()
class DOODLEEDITOR_API ADoodleAssetsPreview : public AActor {
 public:
  GENERATED_BODY()
  ADoodleAssetsPreview();
  virtual void OnConstruction(const FTransform &Transform) override;
  virtual void PostActorCreated() override;

  /// 强度
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods", meta = (MultiLine = "true"))
  TEnumAsByte<EDoodleAssetsPreviewLightModel> LightingScenarios;
  // 背景
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  double BackgroundValueCompensation;
  // 光阴影
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods", meta = (MultiLine = "true"))
  bool RaytracingShadow;
  // 透射
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  bool EnableTransmission;
  // 硬件
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  bool UseHardWareRaytracing;
  // 曝光补偿
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods", meta = (MultiLine = "true", UIMin = "-15.0", UIMax = "15.0"))
  double ExposureCompensation;
  // 接地
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  bool Grounded;
  /// 参考物体
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  bool Calibrator;

  /// 检查pbr
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  bool EnableChecker;
  // 转灯
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Moods")
  float TurnLighting;

#if WITH_EDITOR
  void PostEditChangeProperty(
      FPropertyChangedEvent &PropertyChangeEvent
  ) override;
#endif  // WITH_EDITOR

  virtual void Tick(float DeltaTime) override;

 private:
  UPROPERTY()
  TObjectPtr<USkyLightComponent> SkyLight;
  UPROPERTY()
  TObjectPtr<UPostProcessComponent> PostProcess;
  UPROPERTY()
  TObjectPtr<UDirectionalLightComponent> DirectionalLight;
  UPROPERTY()
  TObjectPtr<UStaticMeshComponent> SkyDome;
  UPROPERTY()
  TObjectPtr<USceneComponent> RootStaticMeshs;
  UPROPERTY()
  TArray<TObjectPtr<UStaticMeshComponent>> StaticMeshs;
  UPROPERTY()
  TObjectPtr<UMaterialInstanceDynamic> PBR_Mat_Inst;
  UPROPERTY()
  TObjectPtr<UMaterialInstanceDynamic> SkyDome_Mat_Inst;

  void SwitchLight(TEnumAsByte<EDoodleAssetsPreviewLightModel> InModel);
  void PBRChecker(bool InIsEnable);
  void SetBackgroundValue(double InBackgroundValue, double InFarValue);
  void ViewCalibrator();
  void SwitchStaticMeshs(bool InIsEnable);
  void SwitchConsole();
  void TurnLighting_Fun();
};