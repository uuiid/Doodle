#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumClassFlags.h"
#include "AbcWarpHeader.h"

class UAbcImportSettings;
class UMaterialInterface;
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
		std::vector<FString> materalName;
		const FString FilePath;

		//mesh工具
		IMeshUtilities* MeshUtilities;

		void TraverseAbcHierarchy(const Alembic::Abc::IObject &InObject, IAbcObject *InParent);

	public:
		FAbcFile(const FString file);
		~FAbcFile();

		//打开文件
		bool openFile();

		//导入文件
		bool Import();

		void ProcessFrames(TFunctionRef<void(int32, FAbcFile *)> InCallback);
	};
}
