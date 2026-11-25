#pragma once

#include "CoreMinimal.h"
#include "FrameAnimationData.h"
#include "MetaHumanFaceTypes.h"

class UControlRig;
class UControlRigBlueprint;
class URigHierarchy;
class USkeleton;

class FMetaHumanFaceRigBaker
{
  public:
    FMetaHumanFaceRigBaker();
    ~FMetaHumanFaceRigBaker();

    FMetaHumanFaceRigBaker(const FMetaHumanFaceRigBaker&) = delete;
    FMetaHumanFaceRigBaker(FMetaHumanFaceRigBaker&&) = delete;
    FMetaHumanFaceRigBaker& operator=(const FMetaHumanFaceRigBaker&) = delete;
    FMetaHumanFaceRigBaker& operator=(FMetaHumanFaceRigBaker&&) = delete;

    static const TArray<FString>& DefaultCtrlRigAssets();
    static const TArray<FString>& DefaultSkeletonAssets();

    FMetaHumanFaceJobResult Preflight(const FMetaHumanFaceJobRequest& InRequest) const;

    bool LoadRigForBaking(const FString& InCtrlRigAsset, const FString& InSkeletonAsset, FString& OutError);

    TValueOrError<FMetaHumanFaceBakedSet, FMetaHumanFaceSolverError> Bake(
        const TArray<FFrameAnimationData>& InAnimationData, int32 InFirstFrame, int32 InFrameCount,
        uint32 InFpsNum, uint32 InFpsDen
    );

  private:
    UControlRigBlueprint* CtrlRigBlueprint = nullptr;
    USkeleton* FaceSkeleton = nullptr;
    UControlRig* ControlRig = nullptr;

    void ReleaseRig();

    static TValueOrError<FMetaHumanFaceBakedSet, FMetaHumanFaceSolverError> MakeUnknownRigError(
        const FString& InDetail
    );
};
