#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

namespace doodle
{

    class AbcCompressionSettings
    {
    public:
        AbcCompressionSettings()
            : bMergeMeshes(true),
              bBakeMatrixAnimation(false){};
        // 合网格
        bool bMergeMeshes;
        // 将矩阵动画合并为顶点动画
        bool bBakeMatrixAnimation;
    };

    class AbcNormalGenerationSettings
    {
    public:
        AbcNormalGenerationSettings()
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

    class AbcMaterialSettings
    {
    public:
        AbcMaterialSettings()
            : bCreateMaterials(false),
              bFindMaterials(true){};
        // 创建材质
        bool bCreateMaterials;
        // 寻找材质
        bool bFindMaterials;
    };

    enum class AbcConversionPreset : uint8
    {
        Maya,
        Max,
        Custom
    };

    class AbcConversionSettings
    {
    public:
        AbcConversionSettings()
            : Preset(AbcConversionPreset::Maya),
              bFlipU(false),
              bFlipV(true),
              Scale(FVector{1.0f, -1.0f, -1.0f}),
              Rotation(FVector::ZeroVector){};

        //当前设置
        AbcConversionPreset Preset;

        //反转u,v坐标
        bool bFlipU;
        bool bFlipV;

        //缩放
        FVector Scale;
        //旋转
        FVector Rotation;
    };

    class AbcGeometryCacheSettings
    {
    public:
        AbcGeometryCacheSettings()
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

    class AbcImportSettings
    {
    public:
        AbcImportSettings()
            : NormalGenerationSettings(),
              MaterialSettings(),
              CompressionSettings(),
              GeometryCacheSettings(),
              ConversionSettings(),
              bReimport(),
              NumThreads(){};

        // 法线设置
        AbcNormalGenerationSettings NormalGenerationSettings;

        // 材料设置
        AbcMaterialSettings MaterialSettings;

        // 压缩设置
        AbcCompressionSettings CompressionSettings;

        // 几何缓存设置
        AbcGeometryCacheSettings GeometryCacheSettings;

        // 转换坐标轴
        AbcConversionSettings ConversionSettings;

        bool bReimport;
        int32 NumThreads;
    };
}