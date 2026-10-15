#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * AI 灯光 IR 的 UE 侧镜像。
 *
 * **契约权威在服务端**：`E:\Doodle\src\doodle_core\metadata\ai_light\rig.h`（rig）与
 * `...\ai_light\task_payload.h`（派发载荷 / 回传报文）。这里只做镜像，键名必须逐字一致，
 * 不许自作主张加字段或改名 —— 改契约要同时改服务端、本文件与方案 §15.12/§15.13。
 *
 * 命名空间：沿用插件习惯使用全局的 `FDoodle*` 前缀（本插件其它模块也不用命名空间）。
 */

// ---------------------------------------------------------------------------
// 枚举：JSON 名 = 服务端 rig.h 里 NLOHMANN_JSON_SERIALIZE_ENUM 的字符串
// ---------------------------------------------------------------------------

/// 灯光在布光中的角色
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

/// UE 灯类型
enum class EDoodleLightType : uint8
{
	Point,
	Spot,
	Rect,
	Directional,
	Sky,
};

/// 锚点所在空间。camera 是默认也是唯一推荐值（绝对世界坐标对 LLM 无意义）
enum class EDoodleAnchorSpace : uint8
{
	Camera,
	World,
	Subject,
};

/// 强度单位
enum class EDoodleIntensityUnit : uint8
{
	Candela,
	Lumen,
	Lux,
	Unitless,
};

/// 颜色给定方式
enum class EDoodleColorMode : uint8
{
	Temperature,
	RGB,
	Filter,
};

/// UE Mobility
enum class EDoodleLightMobility : uint8
{
	StaticLight,
	Stationary,
	Movable,
};

/// 灯光来源。inherited = 基线关卡里美术已布好的灯
enum class EDoodleLightOrigin : uint8
{
	AI,
	Artist,
	Inherited,
};

/// ops 的操作种类
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

/// 单张候选图的渲染状态（服务端 candidate.h）。**没有 succeeded**：成功就是 rendered。
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

// ---------------------------------------------------------------------------
// 数据结构：字段名与 rig.h / task_payload.h 逐一对应
// ---------------------------------------------------------------------------

/// 三维向量。JSON 里是定长数组 [x, y, z]。
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

/// 变换。rotation 用 [pitch, yaw, roll]，单位度。
struct DOODLELIGHTING_API FDoodleTransform
{
	FDoodleVec3 Location;
	FDoodleVec3 Rotation;
};

/// 强度
struct DOODLELIGHTING_API FDoodleIntensity
{
	EDoodleIntensityUnit Unit = EDoodleIntensityUnit::Candela;
	double Value = 0.0;
};

/// 颜色。三个分支按 Mode 路由：
///   Temperature → Kelvin（开 UseTemperature）
///   RGB         → RGB
///   Filter      → Filter（字符串，如色纸名；执行器认不出来时按白处理并告警）
struct DOODLELIGHTING_API FDoodleColor
{
	EDoodleColorMode Mode = EDoodleColorMode::Temperature;
	double Kelvin = 5600.0;
	FDoodleVec3 RGB;
	FString Filter;
};

/// 锚点
struct DOODLELIGHTING_API FDoodleAnchor
{
	EDoodleAnchorSpace Space = EDoodleAnchorSpace::Camera;
	/// 目标名（可选）。空 = 用相机视线命中的「主体」点。
	FString Target;
	/// 注视目标名（可选）。空 = 注视锚点本身。
	FString LookAt;
};

/// 相对锚点的球面偏移
struct DOODLELIGHTING_API FDoodleOffset
{
	/// 方位角（度）。camera 空间里 0 = 相机正前方，正 = 向右。
	double AzimuthDeg = 0.0;
	/// 仰角（度）。0 = 水平面，正 = 向上。
	double ElevationDeg = 0.0;
	/// 距锚点距离（米）
	double DistanceM = 3.0;
};

/// 形状
struct DOODLELIGHTING_API FDoodleShape
{
	double SourceRadiusCm = 0.0;
	FString Ies;
	TOptional<double> InnerConeDeg;
	TOptional<double> OuterConeDeg;
};

/// 阴影
struct DOODLELIGHTING_API FDoodleShadows
{
	bool bCast = true;
	/// 0 = 硬，1 = 最软
	double Softness = 0.5;
};

/// 动画关键帧（本次派发不用，但契约里在）
struct DOODLELIGHTING_API FDoodleAnimKey
{
	int32 Frame = 0;
	double Intensity = 0.0;
};

/// 单盏灯的完整描述
struct DOODLELIGHTING_API FDoodleLight
{
	/// 稳定语义 id（服务端生成 L00/L01…）。**不是** UE actor 名。
	FString Id;
	EDoodleLightRole Role = EDoodleLightRole::Key;
	EDoodleLightType Type = EDoodleLightType::Spot;
	FDoodleAnchor Anchor;
	FDoodleOffset Offset;
	FDoodleIntensity Intensity;
	FDoodleColor Color;
	FDoodleShape Shape;
	FDoodleShadows Shadows;
	/// [r, g, b]，1/0 表示启用。项目约定主光走 Channel1 → [0,1,0]
	TArray<int32> Channels = {0, 1, 0};
	EDoodleLightMobility Mobility = EDoodleLightMobility::Movable;
	TArray<FDoodleAnimKey> Anim;
	EDoodleLightOrigin Origin = EDoodleLightOrigin::AI;
};

/// 出处
struct DOODLELIGHTING_API FDoodleProvenance
{
	FString Model;
	FString PromptHash;
	FString Reference;
	int32 Seed = 0;
	FString Tier;
};

/// 曝光（§7.5：迭代期必须手动锁定）
struct DOODLELIGHTING_API FDoodleExposure
{
	FString Method = TEXT("manual");
	double Ev100Bias = 0.0;
};

/// 白平衡
struct DOODLELIGHTING_API FDoodleWhiteBalance
{
	double Kelvin = 5600.0;
	double Tint = 0.0;
};

/// 环境
struct DOODLELIGHTING_API FDoodleEnvironment
{
	double SkyLightIntensity = 1.0;
	double FogDensity = 0.0;
};

/// 调色
struct DOODLELIGHTING_API FDoodleGrading
{
	double Contrast = 1.0;
	double Saturation = 1.0;
	double Lift = 0.0;
	double Vignette = 0.0;
	double Bloom = 0.0;
};

/// 全局量
struct DOODLELIGHTING_API FDoodleGlobals
{
	FDoodleExposure Exposure;
	FDoodleWhiteBalance WhiteBalance;
	FDoodleEnvironment Environment;
	FDoodleGrading Grading;
};

/// 一条 op。patch 保持原始 JSON：服务端 diff_rigs 只发「真的变了的子块」，
/// 键名是给执行器看的稳定名字（offset/intensity/color/shape/shadows 或整块 globals）。
struct DOODLELIGHTING_API FDoodleLightOp
{
	EDoodleLightOpKind Op = EDoodleLightOpKind::Modify;
	FString Light;
	TSharedPtr<FJsonObject> Patch;
	FString Rationale;
};

/// 一个候选的完整 rig
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

// ---------------------------------------------------------------------------
// 派发载荷（服务端 ai_light_task_payload_t）
// ---------------------------------------------------------------------------

/// 派发载荷里的一个候选
struct DOODLELIGHTING_API FDoodleTaskCandidate
{
	int32 CandidateIndex = 0;
	FString CandidateId;
	/// MRQ 把这个候选的帧写进哪个目录（工作机已拼成本地绝对路径）
	FString FramesDir;
	/// 首帧海报的落点（工作机写，UE 不碰）
	FString PngPath;
	FString ExrPath;
	/// 合成后的视频落点（工作机写，UE 不碰）
	FString MoviePath;
	FDoodleLightingRig Rig;
};

/// 服务端派发的一份任务（= server_task_info.command_）
struct DOODLELIGHTING_API FDoodleTaskPayload
{
	FString SessionId;
	int32 Iteration = 1;
	FString SpecId;
	FString OutDir;
	FString BaselineLevel;
	FString UeProjectPath;
	FString CameraFilePath;
	/// auto_light 已经生成好的三个资产：渲染关卡、关卡序列、电影管线配置（后两个可能是包路径）
	FString RenderMap;
	FString LevelSequence;
	FString MoviePipelineConfig;
	/// 本轮帧区间，只作诊断：真正的渲染区间由序列自己的 playback range 决定
	int32 FrameBegin = 0;
	int32 FrameEnd = 0;
	TArray<FDoodleTaskCandidate> Candidates;
};

/// 引擎内原生路线（D52）的**启动计划**：服务端 `/iterate` 直接回给调用方的那一份。
///
/// 它与 `FDoodleTaskPayload` 的差别只有两处，都是刻意的：
///   - **没有 `spec_id` / `baseline_level`**：队列路线里它们是给工作机记日志用的，本机路线不需要；
///   - 多一个 `iteration_id`（轮次 uuid）：本机路线没有 `server_task_info` 行，回传时服务端只能用
///     「会话 + 轮次」定位（D54），而磁盘目录名就是轮次 uuid（D44）。
/// 候选结构与 `FDoodleTaskCandidate` 逐字相同 —— 面板把计划交给客户端，客户端再原样喂给 Runner。
struct DOODLELIGHTING_API FDoodleLaunchPlan
{
	FString SessionId;
	/// 轮次 uuid。就是磁盘上的 `<out_dir>/it_xxx` 那个轮次目录名（D44），也是回传定位键（D54）
	FString IterationId;
	int32 Iteration = 1;
	/// 工作机/本机本地缓存根下的**相对**目录（D39）：`<session_uuid>/it_<iteration_uuid>`
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

// ---------------------------------------------------------------------------
// 回传报文（服务端 round_render_report_t / candidate_render_report_t）
// ---------------------------------------------------------------------------

/// 单个候选的回传。**不带路径**：路径由服务端按 uuid 现算（D27）。
struct DOODLELIGHTING_API FDoodleCandidateReport
{
	int32 CandidateIndex = 0;
	FString CandidateId;
	EDoodleRenderStatus Status = EDoodleRenderStatus::Pending;
	int64 RenderMs = 0;
	int32 LightCount = 0;
	/// 这个候选实际渲染出的帧数（诊断用，服务端没有对应列）
	int32 FrameCount = 0;
	/// 原样透传给服务端的 candidate_metrics_t。本里程碑不算指标（服务端可复算），所以通常为 null。
	TSharedPtr<FJsonObject> Metrics;
};

/// 整轮回传。error 非空 = 整轮失败（轮级通道）；单个失败走 Status=Failed。
struct DOODLELIGHTING_API FDoodleRoundReport
{
	TArray<FDoodleCandidateReport> Candidates;
	/// 序列的 display rate。工作机按它合成视频，取不到就退回 25
	int32 FrameRate = 0;
	FString Error;
};

/**
 * 整轮回传文件名。与服务端 `doodle::ai_light::g_ai_light_render_report_file_name` 相同，
 * 工作机在起 UE 前会先删掉它、跑完再读它（残留会让上一轮的结果被当成本轮）。
 */
namespace DoodleLightingProtocol
{
	static const TCHAR* const RenderReportFileName = TEXT("render_report.json");
}
