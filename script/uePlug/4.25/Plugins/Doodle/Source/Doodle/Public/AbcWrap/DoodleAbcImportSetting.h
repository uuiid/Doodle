#pragma once

#include "CoreMinimal.h"
#include "AbcWarpHeader.h"
#include "DoodleAbcImportSetting.generated.h"

UENUM(BlueprintType)
enum class EDooAlembicImportType : uint8
{
	/** Imports only the first frame as one or multiple static meshes */
	StaticMesh,
	/** Imports the Alembic file as flipbook and matrix animated objects */
	GeometryCache UMETA(DisplayName = "Geometry Cache (Experimental)"),
	/** Imports the Alembic file as a skeletal mesh containing base poses as morph targets and blending between them to achieve the correct animation frame */
	Skeletal
};

USTRUCT(Blueprintable)
struct FDooAbcCompressionSettings
{
public:
	GENERATED_USTRUCT_BODY()
	FDooAbcCompressionSettings()
		: bMergeMeshes(true),
		  bBakeMatrixAnimation(false){};
	// 合网格
	bool bMergeMeshes;
	// 将矩阵动画合并为顶点动画
	bool bBakeMatrixAnimation;
};

UENUM(Blueprintable)
enum struct EDooAlembicSamplingType : uint8
{
	/** 根据导入的数据对动画采样*/
	PerFrame,
	/** 以帧步长确定的给定间隔对动画进行采样*/
	PerXFrames UMETA(DisplayName = "Per X Frames"),
	/** 按时间步长确定的给定间隔对动画进行采样*/
	PerTimeStep
};

USTRUCT(Blueprintable)
struct FDooAbcSamplingSettings
{
public:
	GENERATED_USTRUCT_BODY()
	FDooAbcSamplingSettings(){};

	/*采样类型*/
	EDooAlembicSamplingType SamplingType;
	/*帧步长*/
	int32 FrameSteps;
	/*时间步长*/
	float TimeSteps;
	/*帧开始*/
	int32 FrameStart;
	/*帧结束*/
	int32 FrameEnd;
	/*跳过空帧*/
	bool bSkipEmpty;
};

USTRUCT(Blueprintable)
struct FDooAbcNormalGenerationSettings
{
public:
	GENERATED_USTRUCT_BODY()
	FDooAbcNormalGenerationSettings()
		: bForceOneSmoothingGroupPerObject(false),
		  HardEdgeAngleThreshold(0),
		  bRecomputeNormals(false),
		  bIgnoreDegenerateTriangles(true){};
	// 每对象强制计算法线而不是使用平滑组
	bool bForceOneSmoothingGroupPerObject;
	// 硬边阈值 将1视为全部硬边 0为全部软边
	float HardEdgeAngleThreshold;
	// 是否强制计算法线
	bool bRecomputeNormals;
	// 确定计算法线时是否忽略退化三角形
	bool bIgnoreDegenerateTriangles;
};

USTRUCT(Blueprintable)
struct FDooAbcMaterialSettings
{
public:
	GENERATED_USTRUCT_BODY()
	FDooAbcMaterialSettings()
		: bCreateMaterials(false),
		  bFindMaterials(true){};
	// 创建材质
	bool bCreateMaterials;
	// 寻找材质
	bool bFindMaterials;
};

UENUM(Blueprintable)
enum class EDooAbcConversionPreset : uint8
{
	Maya UMETA(DisplayName = "Autodesk Maya"),
	Max UMETA(DisplayName = "Autodesk 3ds Max"),
	Custom UMETA(DisplayName = "Custom Settings")
};

USTRUCT(Blueprintable)
struct FDooAbcConversionSettings
{
public:
	GENERATED_USTRUCT_BODY()
	FDooAbcConversionSettings()
		: Preset(EDooAbcConversionPreset::Maya),
		  bFlipU(false),
		  bFlipV(true),
		  Scale(FVector{1.0f, -1.0f, -1.0f}),
		  Rotation(FVector::ZeroVector){};

	//当前设置
	EDooAbcConversionPreset Preset;

	//反转u,v坐标
	bool bFlipU;
	bool bFlipV;

	//缩放
	FVector Scale;
	//旋转
	FVector Rotation;
};

USTRUCT(Blueprintable)
struct FDooAbcGeometryCacheSettings
{
public:
	GENERATED_USTRUCT_BODY()
	FDooAbcGeometryCacheSettings()
		: bFlattenTracks(true),
		  bApplyConstantTopologyOptimizations(false),
		  bCalculateMotionVectorsDuringImport(false),
		  bOptimizeIndexBuffers(false),
		  CompressedPositionPrecision(0.01f),
		  CompressedTextureCoordinatesNumberOfBits(10){};
	// 合并网格
	bool bFlattenTracks;
	// 强制预处理器只做一次优化, 而不是在预处理器的时候决定, 在拓扑不变的情况下运动模糊始终有效, 只可能出现一些问题
	bool bApplyConstantTopologyOptimizations;
	// 在导入期间进行运动矢量的计算, 会增加文件大小
	bool bCalculateMotionVectorsDuringImport;
	// 优化每个唯一帧的索引缓冲区, 以便在GPU上实现更好的缓存一致性, 非常耗时, 建议关闭
	bool bOptimizeIndexBuffers;
	// 用来压缩顶点位置的精度, 越低越好, 越高损失越多
	float CompressedPositionPrecision;
	// 用来压缩纹理的坐标精度, 高就好, 低就差
	int32 CompressedTextureCoordinatesNumberOfBits;
};

UCLASS(Blueprintable)
class UAbcDoodleImportSettings : public UObject
{
public:
	GENERATED_UCLASS_BODY()
	UAbcDoodleImportSettings();

	static UAbcDoodleImportSettings *Get();

	//导入类型
	EDooAlembicImportType ImportType;

	// 法线设置
	FDooAbcNormalGenerationSettings NormalGenerationSettings;

	// 材料设置
	FDooAbcMaterialSettings MaterialSettings;

	// 压缩设置
	FDooAbcCompressionSettings CompressionSettings;

	// 几何缓存设置
	FDooAbcGeometryCacheSettings GeometryCacheSettings;

	// 转换坐标轴
	FDooAbcConversionSettings ConversionSettings;

	//采样设置
	FDooAbcSamplingSettings SamplingSettings;
	bool bReimport;
	int32 NumThreads;
};
