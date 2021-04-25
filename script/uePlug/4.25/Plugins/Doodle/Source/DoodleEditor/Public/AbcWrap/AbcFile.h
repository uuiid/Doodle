#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumClassFlags.h"
#include "AbcWarpHeader.h"
#include "IAbcObject.h"

class UAbcDoodleImportSettings;
class UMaterialInterface;
class IMeshUtilities;

namespace doodle
{

	class IAbcObject;
	class IAbcPolyMesh;
	class IAbcTransform;

	class FAbcFile
	{
		Alembic::AbcCoreFactory::IFactory Factory;
		Alembic::AbcCoreFactory::IFactory::CoreType CompressionType;
		Alembic::Abc::IArchive Archive;
		Alembic::Abc::IObject TopObject;

		IAbcObject *RootObj;
		// 这个是产生的abc包装类
		std::vector<std::shared_ptr<IAbcObject>> Objects;
		std::vector<std::shared_ptr<IAbcPolyMesh>> PolyMeshes;
		std::vector<std::shared_ptr<IAbcTransform>> Transforms;

		//起始帧和结束帧
		int32 MinFreameIndex;
		int32 MaxFreameIndex;

		//总帧数
		int32 NumFrames;
		//帧率
		float FPS;
		//每秒帧数(从顶部obj检索)
		int32 FramesPerSecond;

		//材质
		std::map<FString, UMaterialInterface *> MaterialMap;

		//边界框
		FBoxSphereBounds ArchiveBounds;

		//最大时间和最小时间
		float MinTime;
		float MaxTime;
		float ImportLengthOffset;
		//时间长度
		float ImportLength;

		//材料集名称
		TArray<FString> materalName;
		const FString FilePath;

		//mesh工具
		IMeshUtilities *MeshUtilities;

		void TraverseAbcHierarchy(const Alembic::Abc::IObject &InObject, IAbcObject *InParent);

	public:
		FAbcFile(const FString file);
		~FAbcFile();

		//打开文件
		bool Open();
		//! 这里所有的方法都要创建
		const FString GetFilePath() const { return {}; };
		//导入文件
		bool Import(UAbcDoodleImportSettings *InImportSettings) { return {}; };
		//获得最大帧
		const int32 GetMaxFrameIndex() const { return {}; };
		const float GetImportTimeOffset() const { return {}; };
		const float GetImportLength() const { return {}; };
		//获得几何体
		const TArray<IAbcPolyMesh *> &GetPolyMeshes() const
		{
			static TArray<IAbcPolyMesh *> t;
			return t;
		};
		const TArray<IAbcTransform *> &GetTransforms() const
		{
			static TArray<IAbcTransform *> t;
			return t;
		};
		//返回abc文件中的几何体数量
		const int32 GetNumPolyMeshes() const { return {}; };
		//查看是否是异构拓扑
		const bool ContainsHeterogeneousMeshes() const { return {}; };
		//获得材质名称
		const TArray<FString> &GetUniqueFaceSetNames() const { return materalName; };
		/*返回面集材质名称的材质 , 找不到返回null*/
		UMaterialInterface **GetMaterialByName(const FString &InMaterialName) { return {}; };

		void ProcessFrames(TFunctionRef<void(int32, FAbcFile *)> InCallback, const EFrameReadFlags InFlags){};
	};
}
