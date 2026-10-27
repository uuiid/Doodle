#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

enum class EDoodleLightRole : uint8
{
	Key,
	Fill,
	Rim,
	Ambient,
	Practical,
	Bounce,
	Fx,
};

enum class EDoodleLightType : uint8
{
	Point,
	Spot,
	Rect,
	Directional,
	Sky,
};

enum class EDoodleAnchorSpace : uint8
{
	Camera,
	World,
	Subject,
};

enum class EDoodleIntensityUnit : uint8
{
	Candela,
	Lumen,
	Lux,
	Unitless,
};

enum class EDoodleColorMode : uint8
{
	Temperature,
	RGB,
	Filter,
};

enum class EDoodleLightMobility : uint8
{
	StaticLight,
	Stationary,
	Movable,
};

enum class EDoodleLightOrigin : uint8
{
	AI,
	Artist,
	Inherited,
};

enum class EDoodleLightOpKind : uint8
{
	Add,
	Modify,
	Remove,
	SetGrading,
	SetEnvironment,
	SetExposure,
	SetWhiteBalance,
};

enum class EDoodleRenderStatus : uint8
{
	Pending,
	Rendered,
	Failed,
};

DOODLELIGHTING_API FString DoodleLightRoleToString(EDoodleLightRole InValue);
DOODLELIGHTING_API bool DoodleLightRoleFromString(const FString& InText, EDoodleLightRole& OutValue);
DOODLELIGHTING_API FString DoodleLightTypeToString(EDoodleLightType InValue);
DOODLELIGHTING_API bool DoodleLightTypeFromString(const FString& InText, EDoodleLightType& OutValue);
DOODLELIGHTING_API FString DoodleAnchorSpaceToString(EDoodleAnchorSpace InValue);
DOODLELIGHTING_API bool DoodleAnchorSpaceFromString(const FString& InText, EDoodleAnchorSpace& OutValue);
DOODLELIGHTING_API FString DoodleIntensityUnitToString(EDoodleIntensityUnit InValue);
DOODLELIGHTING_API bool DoodleIntensityUnitFromString(const FString& InText, EDoodleIntensityUnit& OutValue);
DOODLELIGHTING_API FString DoodleColorModeToString(EDoodleColorMode InValue);
DOODLELIGHTING_API bool DoodleColorModeFromString(const FString& InText, EDoodleColorMode& OutValue);
DOODLELIGHTING_API FString DoodleLightMobilityToString(EDoodleLightMobility InValue);
DOODLELIGHTING_API bool DoodleLightMobilityFromString(const FString& InText, EDoodleLightMobility& OutValue);
DOODLELIGHTING_API FString DoodleLightOriginToString(EDoodleLightOrigin InValue);
DOODLELIGHTING_API bool DoodleLightOriginFromString(const FString& InText, EDoodleLightOrigin& OutValue);
DOODLELIGHTING_API FString DoodleLightOpKindToString(EDoodleLightOpKind InValue);
DOODLELIGHTING_API bool DoodleLightOpKindFromString(const FString& InText, EDoodleLightOpKind& OutValue);
DOODLELIGHTING_API FString DoodleRenderStatusToString(EDoodleRenderStatus InValue);
DOODLELIGHTING_API bool DoodleRenderStatusFromString(const FString& InText, EDoodleRenderStatus& OutValue);

struct DOODLELIGHTING_API FDoodleVec3
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;

	FDoodleVec3() = default;
	FDoodleVec3(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}

	FVector ToVector() const { return FVector(X, Y, Z); }
	static FDoodleVec3 FromVector(const FVector& InValue) { return FDoodleVec3(InValue.X, InValue.Y, InValue.Z); }
};

struct DOODLELIGHTING_API FDoodleTransform
{
	FDoodleVec3 Location;
	FDoodleVec3 Rotation;
};

struct DOODLELIGHTING_API FDoodleIntensity
{
	EDoodleIntensityUnit Unit = EDoodleIntensityUnit::Candela;
	double Value = 0.0;
};

struct DOODLELIGHTING_API FDoodleColor
{
	EDoodleColorMode Mode = EDoodleColorMode::Temperature;
	double Kelvin = 5600.0;
	FDoodleVec3 RGB;
	FString Filter;
};

struct DOODLELIGHTING_API FDoodleAnchor
{
	EDoodleAnchorSpace Space = EDoodleAnchorSpace::Camera;

	FString Target;

	FString LookAt;
};

struct DOODLELIGHTING_API FDoodleOffset
{
	double AzimuthDeg = 0.0;

	double ElevationDeg = 0.0;

	double DistanceM = 3.0;
};

struct DOODLELIGHTING_API FDoodleShape
{
	double SourceRadiusCm = 0.0;
	FString Ies;
	TOptional<double> InnerConeDeg;
	TOptional<double> OuterConeDeg;
};

struct DOODLELIGHTING_API FDoodleShadows
{
	bool bCast = true;

	double Softness = 0.5;
};

struct DOODLELIGHTING_API FDoodleAnimKey
{
	int32 Frame = 0;
	double Intensity = 0.0;
};

struct DOODLELIGHTING_API FDoodleLight
{
	FString Id;
	EDoodleLightRole Role = EDoodleLightRole::Key;
	EDoodleLightType Type = EDoodleLightType::Spot;
	FDoodleAnchor Anchor;
	FDoodleOffset Offset;
	FDoodleIntensity Intensity;
	FDoodleColor Color;
	FDoodleShape Shape;
	FDoodleShadows Shadows;

	TArray<int32> Channels = {0, 1, 0};
	EDoodleLightMobility Mobility = EDoodleLightMobility::Movable;
	TArray<FDoodleAnimKey> Anim;
	EDoodleLightOrigin Origin = EDoodleLightOrigin::AI;
};

struct DOODLELIGHTING_API FDoodleProvenance
{
	FString Model;
	FString PromptHash;
	FString Reference;
	int32 Seed = 0;
	FString Tier;
};

struct DOODLELIGHTING_API FDoodleExposure
{
	FString Method = TEXT("manual");
	double Ev100Bias = 0.0;
};

struct DOODLELIGHTING_API FDoodleWhiteBalance
{
	double Kelvin = 5600.0;
	double Tint = 0.0;
};

struct DOODLELIGHTING_API FDoodleEnvironment
{
	double SkyLightIntensity = 1.0;
	double FogDensity = 0.0;
};

struct DOODLELIGHTING_API FDoodleGrading
{
	double Contrast = 1.0;
	double Saturation = 1.0;
	double Lift = 0.0;
	double Vignette = 0.0;
	double Bloom = 0.0;
};

struct DOODLELIGHTING_API FDoodleGlobals
{
	FDoodleExposure Exposure;
	FDoodleWhiteBalance WhiteBalance;
	FDoodleEnvironment Environment;
	FDoodleGrading Grading;
};

struct DOODLELIGHTING_API FDoodleLightOp
{
	EDoodleLightOpKind Op = EDoodleLightOpKind::Modify;
	FString Light;
	TSharedPtr<FJsonObject> Patch;
	FString Rationale;
};

struct DOODLELIGHTING_API FDoodleLightingRig
{
	FString SchemaVersion = TEXT("1.1");
	FString RigId;
	FString SessionId;
	FString ShotTaskId;
	FString SpecId;
	int32 Iteration = 1;
	int32 CandidateIndex = 0;
	FDoodleProvenance Provenance;
	FString BaselineLevel;
	FDoodleGlobals Globals;
	TArray<FDoodleLight> Lights;
	TArray<FDoodleLightOp> Ops;
};

struct DOODLELIGHTING_API FDoodleTaskCandidate
{
	int32 CandidateIndex = 0;
	FString CandidateId;

	FString FramesDir;

	FString PngPath;
	FString ExrPath;

	FString MoviePath;
	FDoodleLightingRig Rig;
};

struct DOODLELIGHTING_API FDoodleTaskPayload
{
	FString SessionId;
	int32 Iteration = 1;
	FString SpecId;
	FString OutDir;
	FString BaselineLevel;
	FString UeProjectPath;
	FString CameraFilePath;

	FString RenderMap;
	FString LevelSequence;
	FString MoviePipelineConfig;

	int32 FrameBegin = 0;
	int32 FrameEnd = 0;
	TArray<FDoodleTaskCandidate> Candidates;
};

struct DOODLELIGHTING_API FDoodleLaunchPlan
{
	FString SessionId;

	FString IterationId;
	int32 Iteration = 1;

	FString OutDir;
	FString RenderMap;
	FString LevelSequence;
	FString MoviePipelineConfig;
	int32 FrameBegin = 0;
	int32 FrameEnd = 0;
	FString UeProjectPath;
	FString CameraFilePath;
	TArray<FDoodleTaskCandidate> Candidates;
};

struct DOODLELIGHTING_API FDoodleCandidateReport
{
	int32 CandidateIndex = 0;
	FString CandidateId;
	EDoodleRenderStatus Status = EDoodleRenderStatus::Pending;
	int64 RenderMs = 0;
	int32 LightCount = 0;

	int32 FrameCount = 0;

	TSharedPtr<FJsonObject> Metrics;
};

struct DOODLELIGHTING_API FDoodleRoundReport
{
	TArray<FDoodleCandidateReport> Candidates;

	int32 FrameRate = 0;
	FString Error;
};

namespace DoodleLightingProtocol
{
	static const TCHAR* const RenderReportFileName = TEXT("render_report.json");
}
