#include "AbcWrap/DoodleAbcImport.h"

#include "AbcWrap/DoodleGeometryCacheBones.h"
#include "AbcWrap/DoodleAbcImporterUtilities.h"

#include "PackageTools.h"
#include "ObjectTools.h"

//几何缓存
#include "GeometryCache.h"
#include "GeometryCacheComponent.h"
#include "GeometryCacheCodecV1.h"
#include "GeometryCacheTrackStreamable.h"

//自定义abc导入
#include "AbcWrap/DoodleAbcImportSetting.h"
#include "AbcWrap/AbcFile.h"
#include "AbcWrap/IAbcPolyMesh.h"
#include "AbcWrap/IAbcTransform.h"
#include "AbcWrap/DoodleAbcAssetImportData.h"

//进度条
#include "Misc/ScopedSlowTask.h"
//材质
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
FDoodleAbcImport::FDoodleAbcImport()
{
}

FDoodleAbcImport::~FDoodleAbcImport()
{
	delete AbcFile;
}

const bool FDoodleAbcImport::OpenAbcFileForImport(const FString InFilePath)
{
	AbcFile = new doodle::FAbcFile(InFilePath);
	return AbcFile->Open();
}

const uint32 FDoodleAbcImport::GetEndFrameIndex() const
{
	return (AbcFile != nullptr) ? FMath::Max(AbcFile->GetMaxFrameIndex() - 1, 1) : 0;
}

const bool FDoodleAbcImport::ImportTrackData(const int32 InNumThreads, UAbcDoodleImportSettings *InImportSettings)
{
	ImportSettings = InImportSettings;
	ImportSettings->NumThreads = InNumThreads;

	return AbcFile->Import(ImportSettings);
}

const uint32 FDoodleAbcImport::GetNumMeshTracks() const
{
	return (AbcFile != nullptr) ? AbcFile->GetNumPolyMeshes() : 0;
}

UDoodleGeometryCacheBones *FDoodleAbcImport::ImportAsDoodleGeometryCache(UObject *InParent, EObjectFlags Flags)
{
	UDoodleGeometryCacheBones *GeometryCache = CreateObjInstance<UDoodleGeometryCacheBones>(InParent,
																							InParent != GetTransientPackage() ? FPaths::GetBaseFilename(InParent->GetName()) : FPaths::GetBaseFilename(AbcFile->GetFilePath()) + "_" + FGuid::NewGuid().ToString(),
																							Flags);

	if (GeometryCache) //指针判空,  测试有效
	{
		//除此以外我们还要对内部进行判空
		if (!GeometryCache->p_GeometryCache)
		{
			GeometryCache->p_GeometryCache = NewObject<UGeometryCache>(GeometryCache);
		}
		if (!GeometryCache->p_GeometryCache_curve)
		{
			GeometryCache->p_GeometryCache_curve = NewObject<UDoodleGeometryCacheCurveAsset>(GeometryCache);
		}

		//在这里我们查看是否有几何缓存在编辑器中
		//存在的话我们需要取消注册
		//防止在导入过程中出现失败和崩溃
		TArray<TUniquePtr<FComponentReregisterContext>> ReregisterContexts;
		for (TObjectIterator<UGeometryCacheComponent> Cachelt; Cachelt; ++Cachelt)
		{
			if (Cachelt->GetGeometryCache() == GeometryCache->p_GeometryCache)
			{
				ReregisterContexts.Add(MakeUnique<FComponentReregisterContext>(*Cachelt));
			}
		}

		//如果是重新导入, 还需要清除注册
		GeometryCache->p_GeometryCache->ClearForReimporting();

		//加载后被的默认材料加以准备, 在没有找到材料的情况下可以导入
		auto defaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		check(defaultMaterial);
		uint32 MaterialOffsert = 0;

		//添加轨道
		const int32 NumPolyMeshes = AbcFile->GetNumPolyMeshes();
		if (NumPolyMeshes != 0)
		{
			TArray<UGeometryCacheTrackStreamable *> tracks;
			TArray<doodle::IAbcPolyMesh *> importPolyMeshes;
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
				auto Codec = NewObject<UGeometryCacheCodecV1>(GeometryCache->p_GeometryCache, FName(*FString(TEXT("Flattened_Codec"))), RF_Public);
				Codec->InitializeEncoder(ImportSettings->GeometryCacheSettings.CompressedPositionPrecision,
										 ImportSettings->GeometryCacheSettings.CompressedTextureCoordinatesNumberOfBits);
				auto Track = NewObject<UGeometryCacheTrackStreamable>(GeometryCache->p_GeometryCache, FName(*FString(TEXT("Flattened_Track"))), RF_Public);
				Track->BeginCoding(Codec,
								   ImportSettings->GeometryCacheSettings.bApplyConstantTopologyOptimizations && !bContainsHeterogeneousMeshes,
								   ImportSettings->GeometryCacheSettings.bCalculateMotionVectorsDuringImport,
								   ImportSettings->GeometryCacheSettings.bOptimizeIndexBuffers);
				tracks.Add(Track);

				//制作显示进度条
				FScopedSlowTask SlowTask((ImportSettings->SamplingSettings.FrameEnd + 1) - ImportSettings->SamplingSettings.FrameStart,
										 FText::FromString(FString(TEXT("importing Frames"))));
				SlowTask.MakeDialog(true);

				const TArray<FString> &uniqueFaceSetNames = AbcFile->GetUniqueFaceSetNames();
				const TArray<doodle::IAbcPolyMesh *> &polyMesh = AbcFile->GetPolyMeshes();

				const int32 numTracks = tracks.Num();
				int32 PreviousNumVertices = 0;

				TFunction<void(int32, doodle::FAbcFile *)> CallBack =
					[this, &tracks, &SlowTask, &uniqueFaceSetNames, &polyMesh, &PreviousNumVertices](int32 FrameIndex, doodle::FAbcFile *InAbcFile) {
						FGeometryCacheMeshData meshData;
						bool bConstantTopology = true;

						DoodleAbcImporterUtilities::MergePolyMeshesToMeshData(FrameIndex,
																			  ImportSettings->SamplingSettings.FrameStart,
																			  polyMesh,
																			  uniqueFaceSetNames,
																			  meshData,
																			  PreviousNumVertices,
																			  bConstantTopology);
						//这个恒定拓扑暂时为true;
						///这边是一个重要的地方
						tracks[0]->AddMeshSample(meshData,
												 (polyMesh[0]->GetTimeForFrameIndex(FrameIndex) - InAbcFile->GetImportTimeOffset()),
												 bConstantTopology);

						if (IsInGameThread())
						{
							SlowTask.EnterProgressFrame(1.0f);
						}
					};
				ImportSettings->SamplingSettings.FrameStart;

				AbcFile->ProcessFrames(CallBack, doodle::EFrameReadFlags::ApplyMatrix);

				//现在我们开始寻找材料
				for (const FString &FaceSetName : uniqueFaceSetNames)
				{
					auto mat = DoodleAbcImporterUtilities::RetrieveMaterial(*AbcFile, FaceSetName, InParent, Flags);

					GeometryCache->p_GeometryCache->Materials.Add((mat != nullptr) ? mat : defaultMaterial);
					//这一部分是我添加的材质部分
					GeometryCache->p_GeometryCache_curve->materalName.Add(FaceSetName);

					if (mat != UMaterial::GetDefaultMaterial(MD_Surface))
					{
						mat->PostEditChange();
					}
				}

				///从这里开始就是我自己的添加骨骼物体的地方了
				TFunction<void(int32, doodle::FAbcFile *)> tanFun =
					[&GeometryCache, &SlowTask](int32 frameIndex, doodle::FAbcFile *InAbcFile) {
						for (auto tran : InAbcFile->GetTransforms())
						{
							auto matx = tran->GetMatrix(frameIndex);													//这里我们获得旋转矩阵
							auto timeFrame = tran->GetTimeForFrameIndex(frameIndex) - InAbcFile->GetImportTimeOffset(); //这里我们获得帧对应的时间(并减去时间偏移)

							auto name = tran->GetName();
							//设置显示名称
							auto smartName = FSmartName{};
							smartName.DisplayName = FName(tran->GetName());
							//寻找曲线
							FTransformCurve *curve = GeometryCache->p_GeometryCache_curve->tranAnm.FindByPredicate(
								[name](FTransformCurve &tran) -> bool { return tran.Name.DisplayName.ToString() == name; });
							FTransform Transform{matx};
							//设置曲线
							if (curve != nullptr)
							{
								curve->TranslationCurve.FloatCurves[0].AddKey(timeFrame, Transform.GetTranslation().X, false);
								curve->TranslationCurve.FloatCurves[1].AddKey(timeFrame, Transform.GetTranslation().Y, false);
								curve->TranslationCurve.FloatCurves[2].AddKey(timeFrame, Transform.GetTranslation().Z, false);
								//rot
								curve->RotationCurve.FloatCurves[0].AddKey(timeFrame, Transform.GetRotation().X, true);
								curve->RotationCurve.FloatCurves[1].AddKey(timeFrame, Transform.GetRotation().Y, true);
								curve->RotationCurve.FloatCurves[2].AddKey(timeFrame, Transform.GetRotation().Z, true);
							}
							else
							{
								FTransformCurve k_curve{smartName, AACF_DefaultCurve};
								//tran
								k_curve.TranslationCurve.FloatCurves[0].AddKey(timeFrame, Transform.GetTranslation().X, false);
								k_curve.TranslationCurve.FloatCurves[1].AddKey(timeFrame, Transform.GetTranslation().Y, false);
								k_curve.TranslationCurve.FloatCurves[2].AddKey(timeFrame, Transform.GetTranslation().Z, false);
								//rot
								k_curve.RotationCurve.FloatCurves[0].AddKey(timeFrame, Transform.GetRotation().X, true);
								k_curve.RotationCurve.FloatCurves[1].AddKey(timeFrame, Transform.GetRotation().Y, true);
								k_curve.RotationCurve.FloatCurves[2].AddKey(timeFrame, Transform.GetRotation().Z, true);

								GeometryCache->p_GeometryCache_curve->tranAnm.Add(k_curve);
							}
						}
						if (IsInGameThread())
						{
							SlowTask.EnterProgressFrame(1.0f);
						}
					};

				AbcFile->ProcessFrames(tanFun, doodle::EFrameReadFlags::ApplyMatrix);
			}
			else
			{
			}

			//这里是干什么的我也不知道
			TArray<FMatrix> mats;
			mats.Add(FMatrix::Identity);
			mats.Add(FMatrix::Identity);
			for (UGeometryCacheTrackStreamable *Track : tracks)
			{
				TArray<float> matTimes;
				matTimes.Add(0.0f);
				matTimes.Add(AbcFile->GetImportLength() + AbcFile->GetImportTimeOffset());
				Track->SetMatrixSamples(mats, matTimes);

				Track->EndCoding();
				GeometryCache->p_GeometryCache->AddTrack(Track);
			}
		}

		//将abc文件的持续时间定义为整个时间的最长的那一部分
		float MaxDuration = 0.0f;
		for (auto Track : GeometryCache->p_GeometryCache->Tracks)
		{
			MaxDuration = FMath::Max(MaxDuration, Track->GetDuration());
		}
		for (auto Track : GeometryCache->p_GeometryCache->Tracks)
		{
			Track->SetDuration(MaxDuration);
		}
		//将起始帧和结束帧存在缓存中
		GeometryCache->p_GeometryCache->SetFrameStartEnd(ImportSettings->SamplingSettings.FrameStart, ImportSettings->SamplingSettings.FrameEnd);

		//更新所有缓存组件, 并填充数据
		for (TObjectIterator<UGeometryCacheComponent> cachel; cachel; ++cachel)
		{
			cachel->OnObjectReimported(GeometryCache->p_GeometryCache);
		}
	}
	return GeometryCache;
}

UDoodleGeometryCacheBones *FDoodleAbcImport::ReimportAsDoodleGeometryCache(UDoodleGeometryCacheBones *GeometryCache)
{
	UDoodleGeometryCacheBones *cache = ImportAsDoodleGeometryCache(GeometryCache->GetOuter(), RF_Public | RF_Standalone);
	return cache;
}

void FDoodleAbcImport::UpdateAssetImportData(UDoodleAbcAssetImportData *AssetImportData)
{
	AssetImportData->TrackNames.Empty();
	const TArray<doodle::IAbcPolyMesh *> &polyMeshes = AbcFile->GetPolyMeshes();
	for (const auto polyMesh : polyMeshes)
	{
		if (polyMesh->bShouldImport)
		{
			AssetImportData->TrackNames.Add(polyMesh->GetName());
		}
	}
	AssetImportData->SamplingSettings = ImportSettings->SamplingSettings;
}

void FDoodleAbcImport::RetrieveAssetImportData(UDoodleAbcAssetImportData *ImportData)
{
	bool bAnySetForImport = false;

	for (auto polyMesh : AbcFile->GetPolyMeshes())
	{
		if (ImportData->TrackNames.Contains(polyMesh->GetName()))
		{
			polyMesh->bShouldImport = true;
			bAnySetForImport = true;
		}
		else
		{
			polyMesh->bShouldImport = false;
		}
	}

	if (!bAnySetForImport)
	{
		for (auto polymesh : AbcFile->GetPolyMeshes())
		{
			polymesh->bShouldImport = true;
		}
	}
}

template <typename T>
T *FDoodleAbcImport::CreateObjInstance(UObject *&InParent, const FString &ObjectName, const EObjectFlags Flags)
{
	//父包放置的新网络
	UPackage *package = nullptr;
	FString NewPackageName;

	//创建一个软甲包
	NewPackageName = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetName() + TEXT("/") + ObjectName);
	NewPackageName = UPackageTools::SanitizePackageName(NewPackageName); //替换软件包中的无效字符

	package = CreatePackage(nullptr, *NewPackageName);

	//好像是去除objname的非法字符
	const FString SanitizedObjectName = ObjectTools::SanitizeObjectName(ObjectName);

	//从这里开始我们测试是否在导入的地方存在包
	auto ExistingTypedObject = FindObject<T>(package, *SanitizedObjectName);
	auto ExistingObject = FindObject<UObject>(package, *SanitizedObjectName);

	if (ExistingTypedObject != nullptr) //指针测空 不空就释放渲染资源
	{
		ExistingTypedObject->PreEditChange(nullptr);
	}
	else if (ExistingObject != nullptr)
	{
		//释放资源后删除对象
		const bool bDeleteSucceeded = ObjectTools::DeleteSingleObject(ExistingObject);

		if (bDeleteSucceeded)
		{
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

	return NewObject<T>(package, FName(*SanitizedObjectName), Flags | RF_Public);
}
