
#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "DoodleGhostTrailComponent.generated.h"
class UPoseableMeshComponent;

USTRUCT()
struct FDoodleGhostTrailInfo
{
    GENERATED_BODY();
    float Life;
    float Age;
    UPoseableMeshComponent *Ghost;
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DOODLE_API UDoodleGhostTrailComponent : public UActorComponent
{
public:
    GENERATED_BODY()
    UDoodleGhostTrailComponent(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        enum ELevelTick TickType,
        FActorComponentTickFunction *ThisTickFunction) override;

    UPROPERTY(EditAnywhere, Category = "Doodle")
    FName BoneName;

    UPROPERTY(EditAnywhere, Category = "Doodle")
    float Distance{30.0f};

    UPROPERTY(EditAnywhere, Category = "Doodle")
    int MaxCount{50};

    UPROPERTY(EditAnywhere, Category = "Doodle")
    float Life{1.5f};

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Interp, Category = "Doodle")
    FRuntimeFloatCurve TransparentCurve;

    UPROPERTY(EditAnywhere, Category = "Doodle")
    FName TransparentName{TEXT("TransparentName")};

private:
    FVector PreviousLocation;
    FTransform PreviousLocationTransform;
    TArray<FTransform> PreviousTransform;
    USkeletalMeshComponent *SkeletalMeshComponent_P;
    TArray<FDoodleGhostTrailInfo> GhostInfos;

    void CreateGhost(FVector InLocation,
                     float DeltaTime);

    void UpdataGhost(float DeltaTime);

    void SetMaterial_Doodle(UPoseableMeshComponent *InGhost);

    void UpdataMaterial_Doodle(UPoseableMeshComponent *InGhost,
                               float InValue);

    void ClearGhost();
};