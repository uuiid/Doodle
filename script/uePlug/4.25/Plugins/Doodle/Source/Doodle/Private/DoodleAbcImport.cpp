#include "DoodleAbcImport.h"

#include "DoodleAlemblcCacheAsset.h"

#include "PackageTools.h"
#include "ObjectTools.h"
#include "GeometryCacheComponent.h"
#include "GeometryCacheCodecV1.h"
#include "GeometryCacheTrackStreamable.h"


#include "AbcFile.h"
#include "AbcImportSettings.h"
#include "AbcImportLogger.h"

#include "Misc/ScopedSlowTask.h"

FDoodleAbcImport::FDoodleAbcImport()
{
}

FDoodleAbcImport::~FDoodleAbcImport()
{
}

UDoodleAlemblcCacheAsset * FDoodleAbcImport::ImportAsDoodleGeometryCache(UObject * InParent, EObjectFlags Flags)
{
	UDoodleAlemblcCacheAsset *GeometryCache = CreateObjInstance(InParent,
		InParent != GetTransientPackage() ? FPaths::GetBaseFilename(InParent->GetName()):FPaths::GetBaseFilename(AbcFile->GetFilePath()) + "_" + FGuid::NewGuid().ToString(), 
		Flags);

	if(GeometryCache)//指针判空,  测试有效
	{
		//在这里我们查看是否有几何缓存在编辑器中
		//存在的话我们需要取消注册
		//防止在导入过程中出现失败和崩溃
		TArray<TUniquePtr<FComponentReregisterContext>> ReregisterContexts;
		for (TObjectIterator<UGeometryCacheComponent> Cachelt; Cachelt ;++Cachelt)
		{
			if (Cachelt->GetGeometryCache() == GeometryCache) {
				ReregisterContexts.Add(MakeUnique<FComponentReregisterContext>(*Cachelt));
			}
		}

		//如果是重新导入, 还需要清除注册
		GeometryCache->ClearForReimporting();
		
		//加载后被的默认材料加以准备, 在没有找到材料的情况下可以导入
		auto defaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		check(defaultMaterial);
		uint32 MaterialOffsert = 0;
			
		//添加轨道
		const int32 NumPolyMeshes = AbcFile->GetNumPolyMeshes();
		if (NumPolyMeshes != 0) {
			TArray<UGeometryCacheTrackStreamable*> tracks;
			TArray<FAbcPolyMesh *> importPolyMeshes;
			TArray<int32> materialOffsets;

			//是否包含异构网格
			//基本上就是查看是否是异构拓扑
			const bool bContainsHeterogeneousMeshes = AbcFile->ContainsHeterogeneousMeshes();

			if (ImportSettings->GeometryCacheSettings.bApplyConstantTopologyOptimizations && bContainsHeterogeneousMeshes)
			{
				UE_LOG(LogTemp, Warning, TEXT("Unable to enforce constant topology optimizations as the imported tracks contain topology varying data."));
			}

			if (ImportSettings->GeometryCacheSettings.bFlattenTracks)
			{
				//添加解码器和轨道器
				auto Codec = NewObject<UGeometryCacheCodecV1>(GeometryCache, FName(*FString(TEXT("Flattened_Codec"))), RF_Public);
				Codec->InitializeEncoder(ImportSettings->GeometryCacheSettings.CompressedPositionPrecision,
					ImportSettings->GeometryCacheSettings.CompressedTextureCoordinatesNumberOfBits);
				auto Track = NewObject<UGeometryCacheTrackStreamable>(GeometryCache, FName(*FString(TEXT("Flattened_Track"))), RF_Public);
				Track->BeginCoding(Codec, 
					ImportSettings->GeometryCacheSettings.bApplyConstantTopologyOptimizations && !bContainsHeterogeneousMeshes,
					ImportSettings->GeometryCacheSettings.bCalculateMotionVectorsDuringImport,
					ImportSettings->GeometryCacheSettings.bOptimizeIndexBuffers);
				tracks.Add(Track);
				
				//制作显示进度条
				FScopedSlowTask SlowTask((ImportSettings->SamplingSettings.FrameEnd + 1) - ImportSettings->SamplingSettings.FrameStart,
					FText::FromString(FString(TEXT("importing Frames"))));
				SlowTask.MakeDialog(true);

				const TArray<FString>               & uniqueFaceSetNames = AbcFile->GetUniqueFaceSetNames();
				const TArray<FAbcPolyMesh*> & polyMesh                    = AbcFile->GetPolyMeshes();

				const int32 numTracks = tracks.Num();
				int32  PreviousNumVertices = 0;

				TFunction<void(int32, FAbcFile*)> CallBack = [this, &tracks,]() {};
			}
		}
	}
	return nullptr;
}

UDoodleAlemblcCacheAsset * FDoodleAbcImport::CreateObjInstance(UObject *& InParent, const FString & ObjectName, const EObjectFlags Flags)
{
	//父包放置的新网络
	UPackage * package = nullptr;
	FString NewPackageName;

	//创建一个软甲包
	NewPackageName = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetName() + TEXT("/") + ObjectName);
	NewPackageName = UPackageTools::SanitizePackageName(NewPackageName);//替换软件包中的无效字符

	package = CreatePackage(nullptr, *NewPackageName);

	//好像是去除objname的非法字符
	const FString SanitizedObjectName = ObjectTools::SanitizeObjectName(ObjectName);

	//从这里开始我们测试是否在导入的地方存在包
	auto ExistingTypedObject =  FindObject<UDoodleAlemblcCacheAsset>(package, *SanitizedObjectName);
	auto ExistingObject = FindObject<UObject>(package, *SanitizedObjectName);
	
	if (ExistingTypedObject != nullptr)//指针测空 不空就释放渲染资源
	{
		ExistingTypedObject->PreEditChange(nullptr);
	}
	else if (ExistingObject != nullptr)
	{
		//释放资源后删除对象
		const bool bDeleteSucceeded = ObjectTools::DeleteSingleObject(ExistingObject);

		if (bDeleteSucceeded) {
			//强制垃圾回收, 创建一个干净的环境进行导入
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

			//创建一个新的包
			package = CreatePackage(nullptr, *NewPackageName);
			InParent = package;
		}
		else // 删除失败了 GG 返回空指针
		{
			return nullptr;
		}
	}

	return  NewObject<UDoodleAlemblcCacheAsset>(package, FName(*SanitizedObjectName), Flags | RF_Public);
}
