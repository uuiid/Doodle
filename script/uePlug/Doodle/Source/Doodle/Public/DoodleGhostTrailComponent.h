
#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "DoodleGhostTrailComponent.generated.h"
class UPoseableMeshComponent;

USTRUCT()
struct FDoodleGhostTrailInfo {
  GENERATED_BODY();
  float Life;
  float Age;
  UPoseableMeshComponent *Ghost;
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DOODLE_API UDoodleGhostTrailComponent : public UActorComponent {
 public:
  GENERATED_BODY()
  UDoodleGhostTrailComponent(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction *ThisTickFunction
  ) override;

  UPROPERTY(EditAnywhere, Interp, Category = "Doodle", DisplayName = "骨骼名称")
  FName BoneName;

  UPROPERTY(EditAnywhere, Interp, Category = "Doodle", DisplayName = "残影距离")
  float Distance{30.0f};

  UPROPERTY(EditAnywhere, Interp, Category = "Doodle", DisplayName = "最大个数")
  int MaxCount{50};

  UPROPERTY(EditAnywhere, Interp, Category = "Doodle", DisplayName = "残影寿命")
  float Life{1.5f};

  UPROPERTY(EditAnyWhere, Interp, BlueprintReadOnly, Interp, Category = "Doodle", DisplayName = "透明曲线")
  FRuntimeFloatCurve TransparentCurve;

  UPROPERTY(EditAnywhere, Interp, Category = "Doodle", DisplayName = "材质透明参数")
  FName TransparentName{TEXT("TransparentName")};

  /** If true, this component will be rendered in the CustomDepth pass (usually used for outlines) */
  UPROPERTY(EditAnywhere, Interp, AdvancedDisplay, BlueprintReadOnly, Category = "Doodle", meta = (DisplayName = "Render CustomDepth Pass"))
  uint8 bRenderCustomDepth : 1;

 private:
  FVector PreviousLocation;
  FTransform PreviousLocationTransform;
  TArray<FTransform> PreviousTransform;
  USkeletalMeshComponent *SkeletalMeshComponent_P;
  TArray<FDoodleGhostTrailInfo> GhostInfos;

  void CreateGhost(FVector InLocation, float DeltaTime);

  void UpdataGhost(float DeltaTime);

  void SetMaterial_Doodle(UPoseableMeshComponent *InGhost);

  void UpdataMaterial_Doodle(UPoseableMeshComponent *InGhost, float InValue);

  void ClearGhost();
};
