#include "MetaHumanFaceRigBaker.h"

#include "ControlRig.h"
#include "ControlRigBlueprintLegacy.h"
#include "ControlRigObjectBinding.h"
#include "Animation/Skeleton.h"
#include "MetaHumanPerformance.h"
#include "MetaHumanPerformanceExportUtils.h"
#include "Rigs/RigHierarchy.h"
#include "UObject/Package.h"

namespace
{
    const TCHAR* const BackwardsSolveName = TEXT("Backwards Solve");
    const TCHAR* const HeadBoneName = TEXT("head");
    const TCHAR* const DefaultCtrlRig = TEXT("/Doodle/FaceCapture/Face_ControlBoard_CtrlRig");
    const TCHAR* const DefaultSkeleton = TEXT("/Doodle/FaceCapture/Face_Archetype_Skeleton");

    struct FChannelBinding
    {
        FRigControlElement* Control = nullptr;
        FString Field;
        int32 Axis = 0;
    };

    FString ControlTypeField(const ERigControlType InType) {
        switch (InType)
        {
        case ERigControlType::Float:
            return TEXT("float");
        case ERigControlType::Vector2D:
            return TEXT("vector2d");
        case ERigControlType::Position:
            return TEXT("position");
        case ERigControlType::Rotator:
            return TEXT("rotator");
        case ERigControlType::Scale:
            return TEXT("scale");
        default:
            return TEXT("unsupported");
        }
    }

    int32 ControlTypeAxisCount(const ERigControlType InType) {
        switch (InType)
        {
        case ERigControlType::Float:
            return 1;
        case ERigControlType::Vector2D:
            return 2;
        case ERigControlType::Position:
        case ERigControlType::Rotator:
        case ERigControlType::Scale:
            return 3;
        default:
            return 0;
        }
    }

    TArray<double> ReadAxisValues(URigHierarchy* InHierarchy, FRigControlElement* InControl) {
        const FRigControlValue Value = InHierarchy->GetControlValue(InControl, ERigControlValueType::Current);

        switch (InControl->Settings.ControlType)
        {
        case ERigControlType::Float:
            return {Value.Get<float>()};
        case ERigControlType::Vector2D:
        {
            const FVector3f Vector = Value.Get<FVector3f>();
            return {Vector.X, Vector.Y};
        }
        case ERigControlType::Position:
        case ERigControlType::Rotator:
        case ERigControlType::Scale:
        {
            const FVector3f Vector = Value.Get<FVector3f>();
            return {Vector.X, Vector.Y, Vector.Z};
        }
        default:
            return {};
        }
    }
}

FMetaHumanFaceRigBaker::FMetaHumanFaceRigBaker() {
}

FMetaHumanFaceRigBaker::~FMetaHumanFaceRigBaker() {
    ReleaseRig();
}

const TArray<FString>& FMetaHumanFaceRigBaker::DefaultCtrlRigAssets() {
    static const TArray<FString> Assets = { DefaultCtrlRig };
    return Assets;
}

const TArray<FString>& FMetaHumanFaceRigBaker::DefaultSkeletonAssets() {
    static const TArray<FString> Assets = { DefaultSkeleton };
    return Assets;
}

FMetaHumanFaceJobResult FMetaHumanFaceRigBaker::Preflight(const FMetaHumanFaceJobRequest& InRequest) const {
    FMetaHumanFaceJobResult Result;
    Result.Stage = EMetaHumanFaceStage::Solving;
    Result.Code = EMetaHumanFaceErrorCode::None;

    const FString CtrlRigAsset = InRequest.CtrlRigAsset.IsEmpty() ? FString(DefaultCtrlRig) : InRequest.CtrlRigAsset;
    const FString SkeletonAsset =
        InRequest.FaceSkeletonAsset.IsEmpty() ? FString(DefaultSkeleton) : InRequest.FaceSkeletonAsset;

    if (LoadObject<UControlRigBlueprint>(nullptr, *(CtrlRigAsset + TEXT(".") + FPackageName::GetShortName(CtrlRigAsset))) ==
        nullptr)
    {
        Result.MissingModelAssets.Add(CtrlRigAsset);
    }

    if (LoadObject<USkeleton>(nullptr, *(SkeletonAsset + TEXT(".") + FPackageName::GetShortName(SkeletonAsset))) == nullptr)
    {
        Result.MissingModelAssets.Add(SkeletonAsset);
    }

    if (Result.MissingModelAssets.IsEmpty())
    {
        return Result;
    }

    Result.Code = EMetaHumanFaceErrorCode::MetaHumanModelMissing;
    Result.Stage = EMetaHumanFaceStage::Failed;
    Result.Detail = FString::Printf(
        TEXT("missing MetaHuman rig assets: %s"), *FString::Join(Result.MissingModelAssets, TEXT(", "))
    );
    return Result;
}

bool FMetaHumanFaceRigBaker::LoadRigForBaking(
    const FString& InCtrlRigAsset, const FString& InSkeletonAsset, FString& OutError
) {
    const FString CtrlRigAsset = InCtrlRigAsset.IsEmpty() ? FString(DefaultCtrlRig) : InCtrlRigAsset;
    const FString SkeletonAsset = InSkeletonAsset.IsEmpty() ? FString(DefaultSkeleton) : InSkeletonAsset;

    CtrlRigBlueprint = LoadObject<UControlRigBlueprint>(
        nullptr, *(CtrlRigAsset + TEXT(".") + FPackageName::GetShortName(CtrlRigAsset))
    );
    if (CtrlRigBlueprint == nullptr)
    {
        OutError = FString::Printf(TEXT("failed to load control rig %s"), *CtrlRigAsset);
        return false;
    }

    FaceSkeleton = LoadObject<USkeleton>(nullptr, *(SkeletonAsset + TEXT(".") + FPackageName::GetShortName(SkeletonAsset)));
    if (FaceSkeleton == nullptr)
    {
        OutError = FString::Printf(TEXT("failed to load face skeleton %s"), *SkeletonAsset);
        return false;
    }

    UClass* RigClass = CtrlRigBlueprint->GetRigVMBlueprintGeneratedClass();
    if (RigClass == nullptr)
    {
        OutError = FString::Printf(TEXT("control rig %s has no generated class"), *CtrlRigAsset);
        return false;
    }

    ControlRig = NewObject<UControlRig>(GetTransientPackage(), RigClass);
    if (ControlRig == nullptr)
    {
        OutError = FString::Printf(TEXT("failed to instantiate control rig %s"), *CtrlRigAsset);
        return false;
    }

    ControlRig->SetObjectBinding(MakeShared<FControlRigObjectBinding>());
    ControlRig->Initialize();
    ControlRig->Evaluate_AnyThread();

    if (!ControlRig->SupportsEvent(BackwardsSolveName))
    {
        OutError = FString::Printf(TEXT("control rig %s does not support the backwards solve"), *CtrlRigAsset);
        return false;
    }

    return true;
}

void FMetaHumanFaceRigBaker::ReleaseRig() {
    ControlRig = nullptr;
    FaceSkeleton = nullptr;
    CtrlRigBlueprint = nullptr;
}

TValueOrError<FMetaHumanFaceBakedSet, FMetaHumanFaceSolverError> FMetaHumanFaceRigBaker::MakeUnknownRigError(
    const FString& InDetail
) {
    FMetaHumanFaceSolverError Error;
    Error.Code = EMetaHumanFaceSolverErrorCode::RigAssetMissing;
    Error.Detail = InDetail;
    return MakeError(MoveTemp(Error));
}

TValueOrError<FMetaHumanFaceBakedSet, FMetaHumanFaceSolverError> FMetaHumanFaceRigBaker::Bake(
    const TArray<FFrameAnimationData>& InAnimationData, int32 InFirstFrame, int32 InFrameCount, uint32 InFpsNum,
    uint32 InFpsDen
) {
    if (ControlRig == nullptr || FaceSkeleton == nullptr)
    {
        return MakeUnknownRigError(TEXT("the control rig has not been loaded"));
    }

    if (InFrameCount <= 0)
    {
        FMetaHumanFaceSolverError Error;
        Error.Code = EMetaHumanFaceSolverErrorCode::NoAnimationData;
        Error.Detail = TEXT("the bake range is empty");
        return MakeError(MoveTemp(Error));
    }

    URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
    if (Hierarchy == nullptr)
    {
        return MakeUnknownRigError(TEXT("the control rig has no hierarchy"));
    }

    FTransform HeadBoneInitial;
    const bool bHeadBoneOk =
        UMetaHumanPerformanceExportUtils::GetBoneGlobalTransform(FaceSkeleton, HeadBoneName, HeadBoneInitial);
    if (!bHeadBoneOk)
    {
        const FString SkeletonName = FaceSkeleton != nullptr ? FaceSkeleton->GetName() : FString(TEXT("none"));
        return MakeUnknownRigError(
            FString::Printf(TEXT("the face skeleton %s has no head bone"), *SkeletonName)
        );
    }

    const FTransform HeadBoneInverse = HeadBoneInitial.Inverse();

    const TArray<FRigElementKey> CurveKeys = Hierarchy->GetCurveKeys();

    TArray<FRigControlElement*> Controls;
    ControlRig->GetControlsInOrder(Controls);

    TArray<FChannelBinding> Channels;
    FMetaHumanFaceBakedSet Baked;
    for (FRigControlElement* Control : Controls)
    {
        if (!Hierarchy->IsAnimatable(Control))
        {
            continue;
        }

        const int32 AxisCount = ControlTypeAxisCount(Control->Settings.ControlType);
        if (AxisCount == 0)
        {
            continue;
        }

        const TArray<double> Baseline = ReadAxisValues(Hierarchy, Control);
        if (Baseline.Num() < AxisCount)
        {
            continue;
        }

        const FString Field = ControlTypeField(Control->Settings.ControlType);
        const FString ControlName = Control->GetName();

        for (int32 Axis = 0; Axis < AxisCount; ++Axis)
        {
            FChannelBinding Binding;
            Binding.Control = Control;
            Binding.Field = Field;
            Binding.Axis = Axis;
            Channels.Add(MoveTemp(Binding));

            FMetaHumanFaceControllerChannel Channel;
            Channel.ControlName = ControlName;
            Channel.ChannelField = Field;
            Channel.Axis = Axis;
            Channel.ChannelName = AxisCount == 1
                                      ? FString::Printf(TEXT("%s.%s"), *ControlName, *Field)
                                      : FString::Printf(TEXT("%s.%s%d"), *ControlName, *Field, Axis);
            Channel.Values.Reserve(InFrameCount);
            Baked.Channels.Add(MoveTemp(Channel));
        }
    }

    if (Baked.Channels.IsEmpty())
    {
        return MakeUnknownRigError(TEXT("the control rig exposes no animatable float channels"));
    }

    Baked.FirstFrame = InFirstFrame;
    Baked.FrameCount = InFrameCount;
    Baked.FpsNum = InFpsNum;
    Baked.FpsDen = InFpsDen;
    Baked.RootPoses.Reserve(InFrameCount);

    TArray<float> CurveValues;
    CurveValues.Init(0.0f, CurveKeys.Num());

    for (int32 LocalIndex = 0; LocalIndex < InFrameCount; ++LocalIndex)
    {
        const FFrameAnimationData& Frame = InAnimationData[LocalIndex];

        FTransform HeadPose;
        if (Frame.Pose.IsValid())
        {
            HeadPose = HeadBoneInitial * Frame.Pose * HeadBoneInverse;
        }

        const FVector Location = HeadPose.GetLocation();
        const FRotator Rotation = HeadPose.Rotator();

        for (int32 CurveIndex = 0; CurveIndex < CurveKeys.Num(); ++CurveIndex)
        {
            const FRigElementKey& Curve = CurveKeys[CurveIndex];
            const FString CurveName = Curve.Name.ToString();

            float Value = 0.0f;
            if (const float* Found = Frame.AnimationData.Find(CurveName))
            {
                Value = *Found;
            }
            else if (CurveName == TEXT("HeadRoll"))
            {
                Value = static_cast<float>(Rotation.Roll);
            }
            else if (CurveName == TEXT("HeadPitch"))
            {
                Value = static_cast<float>(Rotation.Pitch);
            }
            else if (CurveName == TEXT("HeadYaw"))
            {
                Value = static_cast<float>(Rotation.Yaw);
            }
            else if (CurveName == TEXT("HeadTranslationX"))
            {
                Value = static_cast<float>(Location.X);
            }
            else if (CurveName == TEXT("HeadTranslationY"))
            {
                Value = static_cast<float>(Location.Y);
            }
            else if (CurveName == TEXT("HeadTranslationZ"))
            {
                Value = static_cast<float>(Location.Z);
            }

            Hierarchy->SetCurveValue(Curve, Value);
        }

        ControlRig->Execute(BackwardsSolveName);

        for (int32 ChannelIndex = 0; ChannelIndex < Channels.Num(); ++ChannelIndex)
        {
            const FChannelBinding& Binding = Channels[ChannelIndex];
            const TArray<double> AxisValues = ReadAxisValues(Hierarchy, Binding.Control);
            const float Value = AxisValues.IsValidIndex(Binding.Axis) ? static_cast<float>(AxisValues[Binding.Axis]) : 0.0f;
            Baked.Channels[ChannelIndex].Values.Add(Value);
        }

        Baked.RootPoses.Add(Frame.Pose);
    }

    return MakeValue(MoveTemp(Baked));
}
