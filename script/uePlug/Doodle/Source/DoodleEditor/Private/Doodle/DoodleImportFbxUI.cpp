#include "DoodleImportFbxUI.h"

#include "Widgets/SCanvas.h"
// 目录选择器
#include "Widgets/Input/SDirectoryPicker.h"
// 文件选择器
#include "AssetRegistry/IAssetRegistry.h"
#include "Widgets/Input/SFilePathPicker.h"
// 我们自己的多路径文件选择器
#include "Doodle/FilePathsPicker.h"
// 组合框
#include "Components/ComboBoxString.h"
// fbx读取需要
#include "FbxImporter.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "fbxsdk.h"
#include "fbxsdk/scene/geometry/fbxnode.h"
#include "fbxsdk/core/base/fbxtime.h"

// 读写文件
#include "Misc/FileHelper.h"
// 元数据
#include "UObject/MetaData.h"
// 算法
#include "Algo/AllOf.h"
/// 自动导入类需要
#include "AssetImportTask.h"

/// 正则
#include "Internationalization/Regex.h"
/// 一般的导入任务设置
#include "AssetImportTask.h"
/// 导入模块
#include "AssetToolsModule.h"
/// 导入fbx需要
#include "Animation/AnimBoneCompressionSettings.h"  // 压缩骨骼设置
#include "Factories/Factory.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/ImportSettings.h"
#include "Factories/MaterialImportHelpers.h"
/// 进度框
#include "Misc/ScopedSlowTask.h"
/// 属性按钮
#include "PropertyCustomizationHelpers.h"
/// 内容游览器模块
#include "ContentBrowserModule.h"
/// 内容游览器
#include "IContentBrowserSingleton.h"
/// 导入abc
#include "AbcImportSettings.h"
/// 编辑器笔刷效果
#include "EditorStyleSet.h"

/// 导入相机需要的头文件
#include "Camera/CameraComponent.h"  // 相机组件
#include "CineCameraActor.h"         // 相机
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequence.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneToolsUserSettings.h"          // 导入相机设置
#include "Sections/MovieSceneCameraCutSection.h"  // 相机剪切
#include "SequencerUtilities.h"                   // 创建相机

// 创建world
#include "AssetToolsModule.h"
#include "EditorLevelLibrary.h"
#include "Factories/WorldFactory.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "LevelEditorSubsystem.h"
#include "LevelSequence.h"
#include "Modules/ModuleManager.h"

// 导入abc
#include "AlembicImportFactory.h"
#include "Framework/Notifications/NotificationManager.h"  //通知管理类
#include "LevelEditorViewport.h"                          //编辑器视口
#include "Tracks/MovieSceneCameraCutTrack.h"              //处理对电影场景中CameraCut属性的操作。
#include "TransformData.h"                            //存储关于转换的信息，以便向转换部分添加键。
#include "Widgets/Notifications/SNotificationList.h"  // 编辑器通知

// 自定义导入abc
#include "Animation/SkeletalMeshActor.h"  // 骨骼actor
#include "AssetRegistry/AssetRegistryModule.h"
#include "CineCameraActor.h"  // 相机
#include "Doodle/Abc/DoodleAbcImportSettings.h"
#include "Doodle/Abc/DoodleAlembicImportFactory.h"
#include "EditorAssetLibrary.h"  // save asset
#include "EditorLevelUtils.h"
#include "EngineAnalytics.h"  // 分析
#include "Factories/LevelFactory.h"
#include "GeometryCache.h"       // 几何缓存
#include "GeometryCacheActor.h"  // 几何缓存actor
#include "GeometryCacheComponent.h"
#include "LevelSequenceActor.h"              // 序列actor
#include "LevelSequencePlayer.h"             // 播放序列
#include "MovieSceneGeometryCacheTrack.h"    // 几何缓存轨道
#include "MovieSceneToolsProjectSettings.h"  // 定序器项目设置
#include "PackageHelperFunctions.h"          // 保存包
#include "Sections/MovieSceneLevelVisibilitySection.h"
#include "Sections/MovieSceneSpawnSection.h"  // 生成段
#include "SequencerTools.h"                   // 序列工具
#include "Alembic/Abc/ArchiveInfo.h"
#include "Subsystems/EditorActorSubsystem.h"  // 创建actor
#include "Subsystems/EditorAssetSubsystem.h"
#include "Tracks/MovieSceneLevelVisibilityTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"  // 骨骼动画轨道
#include "Tracks/MovieSceneSpawnTrack.h"              // 生成轨道
#include "Bindings/MovieSceneSpawnableBinding.h"
#include "Bindings/MovieSceneSpawnableActorBinding.h"
#include "Alembic/AbcCoreFactory/IFactory.h"
#include "Alembic/Abc/IArchive.h"
#include "Alembic/Abc/ArchiveInfo.h"
#include "Alembic/AbcCoreOgawa/All.h"
#include "Alembic/AbcGeom/All.h"
#include "Alembic/Abc/IObject.h"
#define LOCTEXT_NAMESPACE "SDoodleImportFbxUI"
const FName SDoodleImportFbxUI::UIName{TEXT("DoodleImportFbxUI")};


namespace
{
	template <typename T>
	int32_t NumType(const Alembic::Abc::IObject& In_Object)
	{
		if (!In_Object) return 0;

		int32_t L_Num{};
		auto L_Header = In_Object.getHeader();
		const auto L_MetaData = In_Object.getMetaData();
		const auto L_EnumChilden = In_Object.getNumChildren();
		if (T::matches(L_MetaData))
			++L_Num;

		for (uint32 L_ChildIndex = 0; L_ChildIndex < L_EnumChilden; ++L_ChildIndex)
		{
			L_Num += NumType<T>(In_Object.getChild(L_ChildIndex));
		}
		return L_Num;
	}


	TTuple<int32_t, int32_t> GetAlembicMeshNumAndCurveNum(const FString& In_File_Path)
	{
		if (FPaths::GetExtension(In_File_Path, true) != ".abc") return {0, 0};

		Alembic::AbcCoreFactory::IFactory L_Factory{};
		Alembic::AbcCoreFactory::IFactory::CoreType L_CoreType{};
		Alembic::Abc::IArchive L_Archive = L_Factory.getArchive(TCHAR_TO_UTF8(*In_File_Path), L_CoreType);
		if (!L_Archive.valid()) return {0, 0};


		return {NumType<Alembic::AbcGeom::ICurves>(L_Archive.getTop()), NumType<Alembic::AbcGeom::IPolyMesh>(L_Archive.getTop())};
	}
} // namespace

FString UDoodleBaseImportData::GetImportPath(const FString& In_Path_Prefix) const
{
	FString L_Path = FString::Printf(TEXT("/Game/Shot/ep%.4d/%s%.3d_sc%.3d%s"), Eps, *In_Path_Prefix, Eps, Shot, *ShotAb);
	return L_Path;
}

void UDoodleBaseImportData::GenStartAndEndTime()
{
	const FRegexPattern L_Reg_Time_Pattern{LR"(_(\d+)-(\d+))"};
	FRegexMatcher L_Reg_Time{L_Reg_Time_Pattern, FPaths::GetBaseFilename(ImportPath)};
	StartTime = {1000};
	EndTime = {1001};

	if (L_Reg_Time.FindNext() && L_Reg_Time.GetEndLimit() > 2)
	{
		StartTime = FCString::Atoi64(*L_Reg_Time.GetCaptureGroup(1));
		EndTime = FCString::Atoi64(*L_Reg_Time.GetCaptureGroup(2));
	}
	const FRegexPattern L_Reg_Ep_Pattern{LR"((ep|EP|Ep)_?(\d+))"};

	if (FRegexMatcher L_Reg_Ep{L_Reg_Ep_Pattern, ImportPath}; L_Reg_Ep.FindNext())
	{
		Eps = FCString::Atoi64(*L_Reg_Ep.GetCaptureGroup(2));
	}

	const FRegexPattern L_Reg_ScPattern{LR"((sc|SC|Sc)_?(\d+)([a-zA-Z])?)"};

	if (FRegexMatcher L_Reg_Sc{L_Reg_ScPattern, ImportPath}; L_Reg_Sc.FindNext())
	{
		Shot = FCString::Atoi64(*L_Reg_Sc.GetCaptureGroup(2));
		if (L_Reg_Sc.GetEndLimit() > 3)
		{
			ShotAb = L_Reg_Sc.GetCaptureGroup(3).ToUpper();
		}
	}
}

FString UDoodleBaseImportData::GetPathPrefix(const FString& In_Path)
{
	FString L_Prefix{};
	int32 L_Index = INDEX_NONE;
	FString L_FileName = FPaths::GetBaseFilename(In_Path);
	if (L_FileName.FindChar('_', L_Index))
	{
		L_FileName.LeftChopInline(L_FileName.Len() - L_Index);
		L_Prefix = L_FileName;
	}
	return L_Prefix;
}

void UDoodleFbxImport_1::GenPathPrefix()
{
	GenStartAndEndTime();
	const FString L_String = FString::Format(
		TEXT("Import_{0}/FbxI_{0}_{1}"),
		TArray<FStringFormatArg>{
			FStringFormatArg{StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()))},
			FStringFormatArg{FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))}
		}
	);
	ImportPathDir = GetImportPath(ImportUI->GetPathPrefix()) / L_String;
}

void UDoodleFbxImport_1::ImportFile()
{
	UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
	L_Data->GroupName = TEXT("doodle import");
	L_Data->Filenames.Add(ImportPath);
	L_Data->DestinationPath = ImportPathDir;
	L_Data->bReplaceExisting = true;
	L_Data->bSkipReadOnly = true;
	L_Data->bReplaceExisting = true;

	UFbxFactory* K_FBX_F = NewObject<UFbxFactory>(L_Data);
	L_Data->Factory = K_FBX_F;

	K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
	K_FBX_F->ImportUI->OriginalImportType = FBXIT_SkeletalMesh;
	K_FBX_F->ImportUI->bImportAsSkeletal = true;
	K_FBX_F->ImportUI->bCreatePhysicsAsset = true;
	K_FBX_F->ImportUI->bImportMesh = true;
	K_FBX_F->ImportUI->bImportAnimations = true;
	K_FBX_F->ImportUI->bImportRigidMesh = true;
	K_FBX_F->ImportUI->bImportMaterials = false;
	K_FBX_F->ImportUI->bImportTextures = false;
	K_FBX_F->ImportUI->bResetToFbxOnMaterialConflict = false;

	K_FBX_F->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
	K_FBX_F->ImportUI->SkeletalMeshImportData->bUseT0AsRefPose = false;
	K_FBX_F->ImportUI->bAutomatedImportShouldDetectType = false;
	K_FBX_F->ImportUI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
	K_FBX_F->ImportUI->AnimSequenceImportData->bImportBoneTracks = true;
	K_FBX_F->ImportUI->bAllowContentTypeImport = true;
	K_FBX_F->ImportUI->TextureImportData->MaterialSearchLocation = EMaterialSearchLocation::UnderRoot;
	if (SkinObj)
	{
		if (OnlyAnim)
		{
			K_FBX_F->ImportUI->Skeleton = SkinObj;
			K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_Animation;
			K_FBX_F->ImportUI->bImportMesh = false;
		}
		else
		{
			K_FBX_F->ImportUI->Skeleton = SkinObj;
			K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
			K_FBX_F->ImportUI->bImportAsSkeletal = true;
			K_FBX_F->ImportUI->bImportMesh = true;
		}
	}
	else
	{
		K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
		K_FBX_F->ImportUI->bImportAsSkeletal = true;
		K_FBX_F->ImportUI->bImportMesh = true;
	}
	const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (TArray<UObject*> L_Objs = AssetToolsModule.Get().ImportAssetsAutomated(L_Data); !L_Objs.IsEmpty())
	{
		if (L_Objs.Top()->IsA<USkeletalMesh>())
		{
			FARFilter LFilter{};
			LFilter.bIncludeOnlyOnDiskAssets = false;
			LFilter.bRecursivePaths = true;
			LFilter.bRecursiveClasses = true;
			LFilter.PackagePaths.Add(FName{ImportPathDir});
			USkeletalMesh* L_Sk = CastChecked<USkeletalMesh>(L_Objs.Top());
			IAssetRegistry::Get()->EnumerateAssets(LFilter, [this, L_Sk](const FAssetData& InAss) -> bool
			{
				//-----------------------
				UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
				EditorAssetSubsystem->SaveLoadedAsset(InAss.GetAsset());
				if (UAnimSequence* L_Anim = Cast<UAnimSequence>(InAss.GetAsset());
					L_Anim && L_Anim->GetSkeleton() == L_Sk->GetSkeleton())
				{
					AnimSeq = L_Anim;
					SkeletalMesh = L_Sk;
					L_Anim->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(
						L_Anim, TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression")
					);
					if (L_Anim->IsDataModelValid())
					{
						L_Anim->CompressCommandletVersion = 0;
						L_Anim->ClearAllCachedCookedPlatformData();
						L_Anim->CacheDerivedDataForCurrentPlatform();
					}
				}
				return true;
			});
		}
		else
		{
			UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
			EditorAssetSubsystem->SaveLoadedAssets(L_Objs);
			for (UObject* L_Obj : L_Objs)
			{
				if (UAnimSequence* L_Seq = Cast<UAnimSequence>(L_Obj))
				{
					AnimSeq = L_Seq;
					L_Seq->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(
						L_Seq, TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression")
					);
					if (L_Seq->IsDataModelValid())
					{
						L_Seq->CompressCommandletVersion = 0;
						L_Seq->ClearAllCachedCookedPlatformData();
						L_Seq->CacheDerivedDataForCurrentPlatform();
					}
				}
			}
			if (SkinObj)
			{
				FARFilter LFilter{};
				LFilter.bIncludeOnlyOnDiskAssets = false;
				LFilter.bRecursivePaths = true;
				LFilter.bRecursiveClasses = true;
				LFilter.PackagePaths.Add(FName{FPaths::GetPath(SkinObj->GetPackage()->GetLoadedPath().GetPackageName())});
				LFilter.PackagePaths.Add(FName{ImportPathDir});
				IAssetRegistry::Get()->EnumerateAssets(LFilter, [this](const FAssetData& InAss) -> bool
				{
					//-----------------------
					UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
					EditorAssetSubsystem->SaveLoadedAsset(InAss.GetAsset());
					if (USkeletalMesh* L_Sk = Cast<USkeletalMesh>(InAss.GetAsset()); L_Sk && L_Sk->GetSkeleton() == SkinObj)
					{
						SkeletalMesh = L_Sk;
						return false;
					}
					return true;
				});
			}
		}
	}
}

void UDoodleFbxImport_1::AssembleScene()
{
	if (CameraImport && CameraImport->FirstImport && AnimSeq && SkeletalMesh)
	{
		ULevelSequence* L_ShotSequence = LoadObject<ULevelSequence>(nullptr, *CameraImport->ImportPathDir);
		if (!L_ShotSequence)
		{
			UE_LOG(LogTemp, Log, TEXT("序列 %s 未能加载"), *CameraImport->ImportPathDir);
			return;
		}
		//-------------------------------------------
		FString Dir = CameraImport->ImportPathDir + TEXT("_LV");
		UWorld* L_ShotLevel = LoadObject<UWorld>(nullptr, *Dir);
		if (L_ShotLevel)
		{
			ASkeletalMeshActor* L_Actor =
				L_ShotLevel->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
			L_Actor->SetActorLabel(SkeletalMesh->GetName());
			L_Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkeletalMesh);
			//---------------------
			UMovieScene* L_MoveScene = L_ShotSequence->GetMovieScene();
			const FGuid L_GUID = L_MoveScene->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
			L_ShotSequence->BindPossessableObject(L_GUID, *L_Actor, L_ShotLevel);
			L_ShotSequence->Modify();
			//-----------------------
			UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = L_MoveScene->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
			UMovieSceneSpawnSection* L_MovieSceneSpawnSection =
				CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
			L_MovieSceneSpawnSection->GetChannel().Reset();
			L_MovieSceneSpawnSection->GetChannel().SetDefault(true);
			L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
			UMovieSceneSkeletalAnimationTrack* L_MovieSceneSkeletalAnim =
				L_MoveScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(L_GUID);
			UMovieSceneSection* AnimSection = L_MovieSceneSkeletalAnim->AddNewAnimationOnRow(StartTime, AnimSeq, -1);
			AnimSection->SetPreRollFrames(50);
			L_Actor->Modify();
			L_ShotLevel->Modify();
			UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
			EditorAssetSubsystem->SaveLoadedAssets({L_ShotSequence, L_ShotLevel});
		}
	}
}

bool UDoodleFbxImport_1::FindSkeleton(const TArray<FDoodleUSkeletonData_1> In_Skeleton) { return false; }

void UDoodleFbxCameraImport_1::GenPathPrefix()
{
	Path_Prefix = ImportUI->GetPathPrefix();
	FString L_Folder = GetImportPath(ImportUI->GetPathPrefix());
	FString L_Base = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *ImportUI->GetPathPrefix().ToUpper(), Eps, Shot, *ShotAb);
	switch (ImportUI->GetPathSuffix())
	{
	case EImportSuffix::Lig:
		L_Folder /= "Import_";
		L_Folder += StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()));
		L_Base += TEXT("_");
		L_Base += StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()));
		break;
	case EImportSuffix::Vfx:
		L_Folder /= "Import_";
		L_Folder += StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()));
		L_Base += TEXT("_");
		L_Base += StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()));
		break;
	case EImportSuffix::End:
		break;
	default:
		break;
	}

	ImportPathDir = L_Folder / L_Base;
}

void UDoodleFbxCameraImport_1::ImportFile()
{
	const FFrameRate L_Rate{25, 1};
	constexpr FFrameNumber L_Start{1001};
	FFrameNumber L_End{1200};

	FScopedSlowTask L_Task_Scoped{6.0f, LOCTEXT("Import_CAm", "导入camera")};
	L_Task_Scoped.MakeDialog();

	auto& L_AssTool = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	L_Task_Scoped.EnterProgressFrame(
		1, FText::Format(
			LOCTEXT("Import_ImportingCameraFile1", "导入 \"{0}\"..."),
			FText::FromString(FPaths::GetBaseFilename(ImportPath))
		)
	);

	// 打开fbx
	UMovieSceneUserImportFBXSettings* L_ImportFBXSettings = GetMutableDefault<UMovieSceneUserImportFBXSettings>();
	FFBXInOutParameters InOutParams;
	// 修改一下设置
	L_ImportFBXSettings->bMatchByNameOnly = false;
	L_ImportFBXSettings->bCreateCameras = false;
	L_ImportFBXSettings->bReplaceTransformTrack = true;
	L_ImportFBXSettings->bReduceKeys = false;
	// 这里使用包装导入
	if (!MovieSceneToolHelpers::ReadyFBXForImport(ImportPath, L_ImportFBXSettings, InOutParams))
	{
		return;
	}

	// 已经打开的fbx, 直接获取, 是一个单例
	UnFbx::FFbxImporter* L_FbxImporter = UnFbx::FFbxImporter::GetInstance();
	L_FbxImporter->ImportFromFile(*ImportPath, FPaths::GetExtension(ImportPath));
	ON_SCOPE_EXIT { L_FbxImporter->ReleaseScene(); };
	fbxsdk::FbxTimeSpan L_Fbx_Time = L_FbxImporter->GetAnimationTimeSpan(
		L_FbxImporter->Scene->GetRootNode(), L_FbxImporter->Scene->GetCurrentAnimationStack()
	);
	// 获取结束帧
	L_End = (int32)L_Fbx_Time.GetStop().GetFrameCount(fbxsdk::FbxTime::ePAL);
	UE_LOG(LogTemp, Log, TEXT("fbx time %d -> %d"), L_Start.Value, L_End.Value);

	L_Task_Scoped.EnterProgressFrame(
		1,
		FText::Format(LOCTEXT("Import_ImportingCameraFile2", "检查定序器 \"{0}\"..."), FText::FromString(ImportPathDir))
	);

	ULevelSequence* L_ShotSequence = LoadObject<ULevelSequence>(nullptr, *ImportPathDir);

	// 创建定序器
	// ULevelSequenceFactoryNew* L_ShotSequenceFactory = ULevelSequenceFactoryNew::;
	if (!L_ShotSequence)
	{
		// #define DOODLE_TEST_1

		const FString L_Package_Name = UPackageTools::SanitizePackageName(ImportPathDir);
		// if (!L_AssTool.)

		UPackage* L_Pkg = CreatePackage(*L_Package_Name);
		{
			TArray<UPackage*> L_TopLevelPackages{L_Pkg};
			UPackageTools::HandleFullyLoadingPackages(L_TopLevelPackages, LOCTEXT("CreateANewObject", "Create a new object"));
			L_ShotSequence = NewObject<ULevelSequence>(
				L_Pkg, FName(*FPaths::GetBaseFilename(ImportPathDir)), RF_Public | RF_Standalone | RF_Transactional
			);
			// FGCObjectScopeGuard DontGCFactory(L_ShotSequence);
			L_ShotSequence->Initialize();
			const UMovieSceneToolsProjectSettings* ProjectSettings = GetDefault<UMovieSceneToolsProjectSettings>();
			FFrameRate TickResolution = L_ShotSequence->GetMovieScene()->GetTickResolution();
			L_ShotSequence->GetMovieScene()->SetPlaybackRange(
				(ProjectSettings->DefaultStartTime * TickResolution).FloorToFrame(),
				(ProjectSettings->DefaultDuration * TickResolution).FloorToFrame().Value
			);
			// L_ShotSequence->Modify();

			// Notify the asset registry
			IAssetRegistry::GetChecked().AssetCreated(L_ShotSequence);

			// analytics create record
			if (FEngineAnalytics::IsAvailable())
			{
				TArray<FAnalyticsEventAttribute> Attribs;
				Attribs.Add(FAnalyticsEventAttribute(TEXT("AssetType"), ULevelSequence::StaticClass()->GetName()));
				Attribs.Add(FAnalyticsEventAttribute(TEXT("Duplicated"), TEXT("No")));

				FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.CreateAsset"), Attribs);
			}

			// Mark the package dirty...
			L_Pkg->MarkPackageDirty();
			SavePackageHelper(L_Pkg, L_Package_Name);
			FirstImport = true;
		}
	}
	L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile3", "设置定序器以及相机 ..."));

	// 设置定序器属性
	L_ShotSequence->GetMovieScene()->SetDisplayRate(L_Rate);
	L_ShotSequence->GetMovieScene()->SetTickResolutionDirectly(L_Rate);
	L_ShotSequence->GetMovieScene()->Modify();

	/// 设置范围
	FFrameNumber offset{50};
	L_ShotSequence->GetMovieScene()->SetWorkingRange((L_Start - 30 - offset) / L_Rate, (L_End + 30) / L_Rate);
	L_ShotSequence->GetMovieScene()->SetViewRange((L_Start - 30 - offset) / L_Rate, (L_End + 30) / L_Rate);
	L_ShotSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start - offset, L_End + 1}, true);
	//-------Add Visibility Track
	UMovieSceneLevelVisibilityTrack* NewTrack =
		L_ShotSequence->GetMovieScene()->FindTrack<UMovieSceneLevelVisibilityTrack>();
	L_ShotSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(NewTrack));
	NewTrack = L_ShotSequence->GetMovieScene()->AddTrack<UMovieSceneLevelVisibilityTrack>();
	UMovieSceneLevelVisibilitySection* NewSection =
		CastChecked<UMovieSceneLevelVisibilitySection>(NewTrack->CreateNewSection());
	TRange<FFrameNumber> SectionRange = L_ShotSequence->GetMovieScene()->GetPlaybackRange();
	NewSection->SetRange(SectionRange);
	NewTrack->AddSection(*NewSection);
	//-------Add Sub
	UMovieSceneSubTrack* NewTrack1 = L_ShotSequence->GetMovieScene()->FindTrack<UMovieSceneSubTrack>();
	L_ShotSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(NewTrack1));
	NewTrack1 = L_ShotSequence->GetMovieScene()->AddTrack<UMovieSceneSubTrack>();
	UMovieSceneSubSection* NewSection1 = CastChecked<UMovieSceneSubSection>(NewTrack1->CreateNewSection());
	NewSection1->SetRange(SectionRange);
	NewTrack1->AddSection(*NewSection1);
	//------------
	L_ShotSequence->Modify();
	ALevelSequenceActor* L_LevelSequenceActor{};

	{
		UMovieScene* L_Move = L_ShotSequence->GetMovieScene();
		ACineCameraActor* L_CameraActor{};
		// 相机task
		UMovieSceneTrack* L_Task = L_ShotSequence->GetMovieScene()->GetCameraCutTrack();

		ULevelSequencePlayer* L_LevelSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
			GWorld->PersistentLevel, L_ShotSequence, FMovieSceneSequencePlaybackSettings{}, L_LevelSequenceActor
		);
		FGuid L_CamGuid;
		FMovieSceneSequenceID L_CamSequenceID{};
		if (!L_Task)
		{
			UWorld* L_World = GCurrentLevelEditingViewportClient ? GCurrentLevelEditingViewportClient->GetWorld() : nullptr;
			FActorSpawnParameters L_ActorSpawnParameters{};
			L_ActorSpawnParameters.ObjectFlags &= ~RF_Transactional;
			if (auto L_Actor = CastChecked<ACineCameraActor>(L_World->SpawnActor<ACineCameraActor>(L_ActorSpawnParameters)))
			{
				L_CameraActor = L_Actor;

				UMovieScene* L_MoveScene = L_ShotSequence->GetMovieScene();
				// TValueOrError<FNewSpawnable, FText> L_Result = dynamic_cast<IMovieScenePlayer*>(L_LevelSequencePlayer)
				//                                                    ->GetSpawnRegister()
				//                                                    .CreateNewSpawnableType(*L_Actor, *L_MoveScene);

				// if (!L_Result.IsValid()) {
				//   FNotificationInfo Info(L_Result.GetError());
				//   Info.ExpireDuration = 3.0f;
				//   FSlateNotificationManager::Get().AddNotification(Info);
				// }
				// FNewSpawnable& L_NewSpawnable = L_Result.GetValue();
				UMovieSceneSpawnableBindingBase;


				L_CamGuid = L_MoveScene->AddPossessable(L_CameraActor->GetName(), L_CameraActor->GetClass());
				L_ShotSequence->BindPossessableObject(L_CamGuid, *L_CameraActor, L_World);

				const FMovieSceneBindingReference* L_MovieSceneBindingReference = L_ShotSequence->GetBindingReferences()->GetReference(L_CamGuid, 0);
				UMovieSceneCustomBinding* L_Bind = NewObject<UMovieSceneSpawnableActorBinding>()->CreateCustomBindingFromBinding(
					*L_MovieSceneBindingReference, L_CameraActor, *L_MoveScene);
				L_Bind->SetupDefaults(nullptr, L_MovieSceneBindingReference->ID, *L_MoveScene);


				// L_Task                   = CastChecked<UMovieSceneCameraCutTrack>(
				//          L_MoveScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass())
				//      );
				//      UMovieSceneCameraCutSection* L_MovieSceneCameraCutSection =
				//          CastChecked<UMovieSceneCameraCutSection>(L_Task->CreateNewSection());
				//      L_MovieSceneCameraCutSection->SetCameraGuid(L_CamGuid);
				MovieSceneToolHelpers::CreateCameraCutSectionForCamera(L_MoveScene, L_CamGuid, L_Start);
				UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = L_MoveScene->AddTrack<UMovieSceneSpawnTrack>(L_CamGuid);
				UMovieSceneSpawnSection* L_MovieSceneSpawnSection =
					CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
				L_MovieSceneSpawnSection->GetChannel().Reset();
				L_MovieSceneSpawnSection->GetChannel().SetDefault(true);
				L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
				// L_Actor->Destroy();
			}
			// FSequencerUtilities::MakeNewSpawnable()

			L_Task = L_ShotSequence->GetMovieScene()->GetCameraCutTrack();
		}

		if (!L_Task) return;
		{
			// 对于这个序列的强制评估, 使相机生成
			L_LevelSequencePlayer->Play();
			if (!FSlateApplication::IsInitialized())
			{
				GEngine->UpdateTimeAndHandleMaxTickRate();
				// Tick the engine.
				GEngine->Tick(FApp::GetDeltaTime(), false);
			}
		}

		for (auto&& L_Section : L_Task->GetAllSections())
		{
			FMovieSceneObjectBindingID L_BindID = Cast<UMovieSceneCameraCutSection>(L_Section)->GetCameraBindingID();
			L_CamGuid = L_BindID.GetGuid();
			L_CamSequenceID = L_BindID.GetRelativeSequenceID();
			// L_CameraActor                       = Cast<ACineCameraActor>(
			//     Cast<UMovieSceneCameraCutSection>(L_Section)->GetFirstCamera(*L_LevelSequencePlayer, L_CamSequenceID)
			//);
			if (L_CameraActor)
			{
				break;
			}
			for (auto&& i : L_LevelSequencePlayer->FindBoundObjects(L_BindID.GetGuid(), L_BindID.GetRelativeSequenceID()))
			{
				if (i.Get()->IsA<ACineCameraActor>())
				{
					L_CameraActor = CastChecked<ACineCameraActor>(i.Get());
					break;
				}
			}
			if (L_CameraActor)
			{
				break;
			}
		}
		FString L_CamLiable = L_CameraActor->GetActorNameOrLabel();

		UE_LOG(LogTemp, Log, TEXT("camera name %s"), *L_CamLiable); // 寻找相机id
		UE_LOG(LogTemp, Log, TEXT("guid %s"), *L_CamGuid.ToString());

		TMap<FGuid, FString> L_Map{};
		L_Map.Add(L_CamGuid, L_CamLiable);

		L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile6", "开始导入帧 ..."));

		// 正式开始导入
		MovieSceneToolHelpers::ImportFBXCameraToExisting(
			L_FbxImporter, L_ShotSequence, L_LevelSequencePlayer, L_CamSequenceID, L_Map, false, false
		);
		bool bValid = MovieSceneToolHelpers::ImportFBXIfReady(
			GWorld, L_ShotSequence, L_LevelSequencePlayer, L_CamSequenceID, L_Map, L_ImportFBXSettings, InOutParams
		);
	}
	L_LevelSequenceActor->Destroy();
	if (!FSlateApplication::IsInitialized())
	{
		// 对于这个序列的强制评估, 使相机生成
		GEngine->UpdateTimeAndHandleMaxTickRate();
		// Tick the engine.
		GEngine->Tick(FApp::GetDeltaTime(), false);
	}

	UEditorAssetLibrary::SaveAsset(L_ShotSequence->GetPathName());
	// FText::FromString(ImportPathDir)));
	//    //UObject* object = AssetToolsModule.Get().CreateAsset(L_Name.ToString(), PackagePath, ULevel::StaticClass(),
	//    Factory);
	//    FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	//--------------------------------生成关卡
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	L_Task_Scoped.EnterProgressFrame(
		1, FText::Format(LOCTEXT("Import_ImportingCameraFile7", "检查关卡\"{0}\"..."), FText::FromString(ImportPathDir))
	);
	FString PackageName = UPackageTools::SanitizePackageName(ImportPathDir) + TEXT("_LV");
	UWorld* L_ShotLevel = LoadObject<UWorld>(nullptr, *PackageName);
	if (!L_ShotLevel)
	{
		// UPackage* Pkg = CreatePackage(*PackageName);
		// Pkg->FullyLoad();
		// Pkg->MarkPackageDirty();
		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
		const FString BaseFileName = FPaths::GetBaseFilename(PackageName);

		FAssetToolsModule& AssetToolsModule =
			FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		L_ShotLevel = CastChecked<UWorld>(
			AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), NewObject<UWorldFactory>()));
		// FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		// AssetRegistryModule.Get().AssetCreated(L_ShotLevel);

		//------------------------
		// L_ShotLevel->Modify();
		EditorAssetSubsystem->SaveLoadedAsset(L_ShotLevel, false);
		// TryCollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
		L_ShotLevel = LoadObject<UWorld>(nullptr, *PackageName);
	}
	//---------------

	const FString LigFolder1 = FString::Printf(TEXT("/Game/Shot/ep%.4d/map/Light_File/level"), Eps);
	const FString LigFolder2 = FString::Printf(TEXT("/Game/Shot/ep%.4d/map/Light_File/other"), Eps);
	FString LigFolder3 = GetImportPath(Path_Prefix) / FString::Printf(
		TEXT("%s_EP%.3d_SC%.3d%s"), *Path_Prefix.ToUpper(), Eps, Shot, *ShotAb);
	switch (Path_Suffix)
	{
	case EImportSuffix::Lig:
		if (!EditorAssetSubsystem->DoesDirectoryExist(LigFolder1))
		{
			EditorAssetSubsystem->MakeDirectory(LigFolder1);
		}
		if (!EditorAssetSubsystem->DoesDirectoryExist(LigFolder2))
		{
			EditorAssetSubsystem->MakeDirectory(LigFolder2);
		}
		if (!EditorAssetSubsystem->DoesAssetExist(LigFolder3))
		{
			EditorAssetSubsystem->DuplicateAsset(ImportPathDir, LigFolder3);
			EditorAssetSubsystem->SaveAsset(LigFolder3, false);
		}
		LigFolder3 += TEXT("_Zong");
		if (UWorld* L_Level = LoadObject<UWorld>(nullptr, *LigFolder3); !L_Level)
		{
			const FString PackagePath = FPackageName::GetLongPackagePath(LigFolder3);
			const FString BaseFileName = FPaths::GetBaseFilename(LigFolder3);
			FAssetToolsModule& AssetToolsModule =
				FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			L_Level = CastChecked<UWorld>(
				AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), NewObject<UWorldFactory>()));

			EditorAssetSubsystem->SaveLoadedAsset(L_Level, false);
		}
		break;
	case EImportSuffix::Vfx:
		{
			FString LongImportPathDir = FPackageName::GetLongPackagePath(ImportPathDir);
			FString FolderPath = FPaths::Combine(LongImportPathDir, TEXT("Vfx"));
			if (!EditorAssetSubsystem->DoesDirectoryExist(FolderPath))
			{
				EditorAssetSubsystem->MakeDirectory(FolderPath);
			}
		}
		break;
	case EImportSuffix::End:
		break;
	default:
		break;
	}

	//--------------
	if (auto L_Name = ImportUI->GetUserFolderName();
		L_Name != TEXT("")
	)
	{
		FString LongImportPathDir = FPackageName::GetLongPackagePath(ImportPathDir);
		FString AbovePath = FPaths::GetPath(LongImportPathDir);
		FString FolderPath = AbovePath / "Vfx" / L_Name;
		if (!EditorAssetSubsystem->DoesDirectoryExist(FolderPath))
		{
			EditorAssetSubsystem->MakeDirectory(FolderPath);
		}
	}
	L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile6", "开始导入帧 ..."));
}

void UDoodleFbxCameraImport_1::AssembleScene()
{
}

void UDoodleAbcImport_1::GenPathPrefix()
{
	FString L_String = FString::Format(
		TEXT("AbcI_{0}_{1}"),
		TArray<FStringFormatArg>{
			FStringFormatArg{StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()))},
			FStringFormatArg{FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))}
		}
	);
	ImportPathDir = GetImportPath(ImportUI->GetPathPrefix()) / L_String;
}

void UDoodleAbcImport_1::ImportFile()
{
	UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
	L_Data->GroupName = TEXT("doodle import");
	L_Data->Filenames.Add(ImportPath);
	L_Data->DestinationPath = ImportPathDir;
	L_Data->bReplaceExisting = true;
	L_Data->bSkipReadOnly = true;
	L_Data->bReplaceExisting = true;

	UDoodleAbcImportFactory* k_abc_f = NewObject<UDoodleAbcImportFactory>(L_Data);
	L_Data->Factory = k_abc_f;
	UDoodleAbcImportSettings* k_abc_stting = NewObject<UDoodleAbcImportSettings>(L_Data);
	k_abc_f->ImportSettings = k_abc_stting;

	k_abc_stting->ImportType = EDoodleAlembicImportType::GeometryCache; // 导入为几何缓存
	k_abc_stting->ConversionSettings.bFlipV = true;
	k_abc_stting->ConversionSettings.Scale.X = 1.0;
	k_abc_stting->ConversionSettings.Scale.Y = -1.0;
	k_abc_stting->ConversionSettings.Scale.Z = 1.0;
	k_abc_stting->ConversionSettings.Rotation.X = 90.0;
	k_abc_stting->ConversionSettings.Rotation.Y = 0.0;
	k_abc_stting->ConversionSettings.Rotation.Z = 0.0;

	k_abc_stting->GeometryCacheSettings.bFlattenTracks = true; // 合并轨道
	k_abc_stting->SamplingSettings.bSkipEmpty = true; // 跳过空白帧
	k_abc_stting->SamplingSettings.FrameStart = StartTime; // 开始帧
	k_abc_stting->SamplingSettings.FrameEnd = EndTime; // 结束帧
	k_abc_stting->SamplingSettings.FrameSteps = 1; // 帧步数

	const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<UObject*> L_Geos = AssetToolsModule.Get().ImportAssetsAutomated(L_Data);
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	EditorAssetSubsystem->SaveLoadedAssets(L_Geos);
	if (!L_Geos.IsEmpty())
	{
		GeometryCache = Cast<UGeometryCache>(L_Geos.Top());
	}
}

void UDoodleAbcImport_1::AssembleScene()
{
	if (CameraImport && CameraImport->FirstImport && GeometryCache)
	{
		ULevelSequence* L_ShotSequence = LoadObject<ULevelSequence>(nullptr, *CameraImport->ImportPathDir);
		if (!L_ShotSequence)
		{
			UE_LOG(LogTemp, Log, TEXT("序列 %s 未能加载"), *CameraImport->ImportPathDir);
			return;
		}
		//--------------------------------
		FString Dir = CameraImport->ImportPathDir + TEXT("_LV");
		UWorld* L_ShotLevel = LoadObject<UWorld>(nullptr, *Dir);
		if (L_ShotLevel)
		{
			AGeometryCacheActor* L_Actor =
				L_ShotLevel->SpawnActor<AGeometryCacheActor>(FVector::ZeroVector, FRotator::ZeroRotator);
			L_Actor->SetActorLabel(GeometryCache->GetName());
			L_Actor->GetGeometryCacheComponent()->SetGeometryCache(GeometryCache);
			//---------------------------------
			UMovieScene* L_MoveScene = L_ShotSequence->GetMovieScene();
			const FGuid L_GUID = L_MoveScene->AddPossessable(L_Actor->GetActorLabel(), L_Actor->GetClass());
			L_ShotSequence->BindPossessableObject(L_GUID, *L_Actor, L_ShotLevel);
			L_ShotSequence->Modify();
			//-----------------------------------
			UMovieSceneSpawnTrack* L_MovieSceneSpawnTrack = L_MoveScene->AddTrack<UMovieSceneSpawnTrack>(L_GUID);
			UMovieSceneSpawnSection* L_MovieSceneSpawnSection =
				CastChecked<UMovieSceneSpawnSection>(L_MovieSceneSpawnTrack->CreateNewSection());
			L_MovieSceneSpawnSection->GetChannel().Reset();
			L_MovieSceneSpawnSection->GetChannel().SetDefault(true);
			L_MovieSceneSpawnTrack->AddSection(*L_MovieSceneSpawnSection);
			//------------------------------
			UMovieSceneGeometryCacheTrack* L_MovieSceneGeoTrack =
				L_MoveScene->AddTrack<UMovieSceneGeometryCacheTrack>(L_GUID);
			UMovieSceneSection* AnimSection =
				L_MovieSceneGeoTrack->AddNewAnimation(StartTime, L_Actor->GetGeometryCacheComponent());
			AnimSection->SetPreRollFrames(50);
			L_Actor->Modify();
			L_ShotLevel->Modify();
			UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
			EditorAssetSubsystem->SaveLoadedAssets({L_ShotSequence, L_ShotLevel});
		}
	}
}

void UDoodleXgenImport_1::GenPathPrefix()
{
	FString L_String = FString::Format(
		TEXT("XgenI_{0}_{1}"),
		TArray<FStringFormatArg>{
			FStringFormatArg{StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(ImportUI->GetPathSuffix()))},
			FStringFormatArg{FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))}
		}
	);
	ImportPathDir = GetImportPath(ImportUI->GetPathPrefix()) / L_String;
}

void UDoodleXgenImport_1::ImportFile()
{
}

void UDoodleXgenImport_1::AssembleScene()
{
}


class SDoodleImportUiItem final : public SMultiColumnTableRow<SDoodleImportFbxUI::UDoodleBaseImportDataPtrType>
{
public:
	SLATE_BEGIN_ARGS(SDoodleImportUiItem)
			: _ItemShow()
		{
		}

		SLATE_ARGUMENT(SDoodleImportFbxUI::UDoodleBaseImportDataPtrType, ItemShow)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		ItemShow = InArgs._ItemShow;
		ItemType = TEXT("onoe");
		if (auto L_FBX = Cast<UDoodleFbxImport_1>(InArgs._ItemShow))
		{
			ItemShowFBX = L_FBX;
		}
		else if (Cast<UDoodleFbxCameraImport_1>(InArgs._ItemShow))
		{
			ItemType = TEXT("导入的相机");
		}
		else if (Cast<UDoodleAbcImport_1>(InArgs._ItemShow))
		{
			ItemType = TEXT("abc文件");
		}

		FSuperRowType::Construct(FSuperRowType::FArguments().Padding(0), InOwnerTableView);
	}

public: // override SMultiColumnTableRow
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		// SHeaderRow::Column(TEXT("Import_File"))
		// SHeaderRow::Column(TEXT("Ep_And_Shot"))
		// SHeaderRow::Column(TEXT("Time_Ranges"))
		// SHeaderRow::Column(TEXT("Skeleton_Path"))
		// SHeaderRow::Column(TEXT("Import_Path_Dir"))

		if (ColumnName == TEXT("Import_File")) // 判断列名为Fbx File，次名称在创建View时，通过SHeaderRow::Column指定
		{
			return SNew(STextBlock).Text(FText::FromString(ItemShow->ImportPath));
		}
		else if (ColumnName == TEXT("Time_Ranges"))
		{
			return SNew(STextBlock)
				.Text_Lambda([this]() -> FText
				{
					return FText::FromString(FString::Printf(TEXT("%d - %d"), ItemShow->StartTime, ItemShow->EndTime));
				});
		}
		else if (ColumnName == TEXT("Import_Path_Dir"))
		{
			return SNew(STextBlock).Text(FText::FromString(ItemShow->ImportPathDir));
		}
		else if (ColumnName == TEXT("Ep_And_Shot"))
		{
			return SNew(STextBlock)
				.Text(FText::FromString(
					FString::Printf(TEXT("EP: %d SC: %d%s"), ItemShow->Eps, ItemShow->Shot, *ItemShow->ShotAb)
				));
		}
		else if (ColumnName == TEXT("Skeleton_Path") && ItemShowFBX)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(1.f)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() -> FText
					{
						return FText::FromString(FString::Printf(
							TEXT("%s"), *(ItemShowFBX->SkinObj != nullptr
								              ? ItemShowFBX->SkinObj->GetPackage()->GetPathName()
								              : FString{TEXT("")})));
					})
				]
				+ SHorizontalBox::Slot() ///  
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot() /// ⬅️, 将选中的给到属性上
					.HAlign(HAlign_Right)
					[
						PropertyCustomizationHelpers::MakeUseSelectedButton(
							FSimpleDelegate::CreateRaw(this, &SDoodleImportUiItem::DoodleUseSelected)) /// 委托转发
					]
					+ SHorizontalBox::Slot() /// 🔍 将属性显示在资产编辑器中
					.HAlign(HAlign_Right)
					[
						PropertyCustomizationHelpers::MakeBrowseButton(
							FSimpleDelegate::CreateRaw(this, &SDoodleImportUiItem::DoodleBrowse)) /// 委托转发
					]
					+ SHorizontalBox::Slot() /// 重置, 将属性给空
					.HAlign(HAlign_Right)
					[
						PropertyCustomizationHelpers::MakeResetButton(
							FSimpleDelegate::CreateRaw(this, &SDoodleImportUiItem::DoodleReset)) /// 委托转发
					]
				];
		}
		else
		{
			return SNew(STextBlock).Text(FText::FromString(ItemType));
		}
	}

	void DoodleUseSelected()
	{
		FContentBrowserModule& L_ContentBrowserModle =
			FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> L_SelectedAss;
		L_ContentBrowserModle.Get().GetSelectedAssets(L_SelectedAss);

		FAssetData* L_It = Algo::FindByPredicate(L_SelectedAss, [](const FAssetData& InAss) -> bool
		{
			return Cast<USkeleton>(InAss.GetAsset()) != nullptr;
		});
		if (L_It != nullptr)
		{
			ItemShowFBX->SkinObj = Cast<USkeleton>(L_It->GetAsset());
		}
	}

	void DoodleBrowse()
	{
		if (ItemShowFBX->SkinObj != nullptr)
		{
			FContentBrowserModule& L_ContentBrowserModle =
				FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			L_ContentBrowserModle.Get().SyncBrowserToAssets(TArray<FAssetData>{FAssetData{ItemShowFBX->SkinObj}});
		}
	}

	void DoodleReset() { ItemShowFBX->SkinObj = nullptr; }

private:
	SDoodleImportFbxUI::UDoodleBaseImportDataPtrType ItemShow;
	UDoodleFbxImport_1* ItemShowFBX{};
	FString ItemType{};
};

void SDoodleImportFbxUI::Construct(const FArguments& Arg)
{
	const FSlateFontInfo Font = FAppStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));

#if PLATFORM_WINDOWS
	const FString FileFilterText = TEXT("fbx and abc |*.fbx;*.abc|fbx (*.fbx)|*.fbx|abc (*.abc)|*.abc");
#else
	const FString FileFilterText = FString::Printf(TEXT("%s"), *FileFilterType.ToString());
#endif

	UEnum* L_Enum = StaticEnum<EImportSuffix>();
	for (EImportSuffix L_E : TEnumRange<EImportSuffix>())
	{
		All_Path_Suffix.Add(MakeShared<FString>(L_Enum->GetNameStringByValue(static_cast<uint8>(L_E))));
	}

	Path_Suffix = EImportSuffix::Lig;

	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 0.0f))
		.BorderImage(new FSlateBrush())
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			// 扫描文件目录槽
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BinaryPathLabel",
				              "将文件和文件夹拖入到这个窗口中, 会自动扫描文件夹下后缀为abc和fbx的子文件,并将所有的文件添加到导入列表中.\n同时也会根据拖入的相机以及各种文件生成关卡"))
				.ColorAndOpacity(FSlateColor{FLinearColor{1, 0, 0, 1}})
				.Font(Font)
			]
			// 前缀槽
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BinaryPathLabel1", "项目缩写"))
					.Font(Font)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(8.0f)
				[
					/// 生成的前缀
					SNew(SEditableTextBox)
					.Text_Lambda([this]()-> FText
					{
						return FText::FromString(this->Path_Prefix);
					})
					.OnTextChanged_Lambda([this](const FText& In_Text)
					{
						GenPathPrefix(In_Text.ToString(), Path_Suffix);
					})
					.OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
					{
						GenPathPrefix(In_Text.ToString(), Path_Suffix);
					})
				]
			]
			// 后缀槽
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BinaryPathLabel11", "部门缩写"))
					.ColorAndOpacity(FSlateColor{FLinearColor{1, 0, 0, 1}})
					.Font(Font)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(8.0f)
				[
					///  
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&All_Path_Suffix)
					.OnSelectionChanged_Lambda(
						[this](const TSharedPtr<FString>& In, ESelectInfo::Type)
						{
							GenPathPrefix(Path_Prefix, static_cast<EImportSuffix>(StaticEnum<EImportSuffix>()->GetValueByNameString(*In)));
						})
					.OnGenerateWidget_Lambda(
						[this](const TSharedPtr<FString>& In)
						{
							return SNew(STextBlock).Text(FText::FromString(*In));
						})
					.InitiallySelectedItem(All_Path_Suffix[0])
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(Path_Suffix)));
						})
					]
				]
			]
			// 只导入相机 SCheckBox
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BinaryPathLabel12", "只导入相机"))
					.ColorAndOpacity(FSlateColor{FLinearColor{1, 1, 0, 1}})
					.Font(Font)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(8.0f)
				[
					///  
					SNew(SCheckBox)
					.IsChecked(this->OnlyCamera)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState In_State) { this->OnlyCamera = In_State; })
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("New Folder", "制作人名称"))
					.Font(Font)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(8.0f)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this]()-> FText
					{
						GConfig->GetString(TEXT("DoodleImportFbx"), TEXT("UserFolderName"), UserFolderName, GEngineIni);
						return FText::FromString(UserFolderName);
					})
					.OnTextChanged_Lambda([this](const FText& In_Text)
					{
						UserFolderName = In_Text.ToString();
					})
					.OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type)
					{
						UserFolderName = In_Text.ToString();
						GConfig->SetString(TEXT("DoodleImportFbx"), TEXT("UserFolderName"), *UserFolderName, GEngineIni);
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("importTitle", "导入的fbx 动画文件, 相机, abc文件"))
				.Font(Font)
			]

			/// 主要的列表小部件(Fbx)
			+ SVerticalBox::Slot()
			.FillHeight(3.0f)
			.VAlign(VAlign_Top)
			.Padding(2.0f)
			[
				SAssignNew(ListImportGui, SListView<SDoodleImportFbxUI::UDoodleBaseImportDataPtrType>)
				.ListItemsSource(&ListImportData)
				.ScrollbarVisibility(EVisibility::All)
				.OnGenerateRow_Lambda( // 生成小部件
					[](SDoodleImportFbxUI::UDoodleBaseImportDataPtrType InItem,
					   const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow>
					{
						return SNew(SDoodleImportUiItem, OwnerTable)
							.ItemShow(InItem);
					}
				)
				.SelectionMode(ESelectionMode::Type::Single) //单选
				.HeaderRow ///题头元素
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Import_File"))
					.FillWidth(4.0f)
					[
						SNew(SBorder)
						.Padding(5)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Import File")))
						]
					]
					+ SHeaderRow::Column(TEXT("Ep_And_Shot"))
					.FillWidth(1.0f)
					.DefaultLabel(LOCTEXT("Ep_And_Shot", "集数和镜头"))

					+ SHeaderRow::Column(TEXT("Time_Ranges"))
					.FillWidth(1.0f)
					.DefaultLabel(LOCTEXT("Time Ranges", "时间范围"))

					+ SHeaderRow::Column(TEXT("Skeleton_Path"))
					.FillWidth(4.0f)
					.DefaultLabel(LOCTEXT("Skeleton Path", "骨骼路径"))

					+ SHeaderRow::Column(TEXT("Import_Path_Dir"))
					.FillWidth(2.0f)
					.DefaultLabel(LOCTEXT("Import Path Dir", "导入的目标"))
				)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Clear USkeleton", "Clear USkeleton"))
					.ToolTipText(LOCTEXT("Clear USkeleton Tip", "清除查找的骨骼"))
					.OnClicked_Lambda([this]()
					{
						for (auto&& i : ListImportData)
						{
							if (i->IsA<UDoodleFbxImport_1>())
							{
								Cast<UDoodleFbxImport_1>(i)->SkinObj = nullptr;
							}
						}
						ListImportGui->RebuildList();
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Clear All", "Clear All"))
					.ToolTipText(LOCTEXT("Clear USkeleton Tip", "清除所有"))
					.OnClicked_Lambda([this]()
					{
						ListImportData.Empty();
						ListImportGui->RebuildList();
						return FReply::Handled();
					})
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Search USkeleton Import", "Search USkeleton Direct Import"))
				.ToolTipText(LOCTEXT("Search USkeleton Tip3", "不寻找骨骼, 直接导入 Fbx, 如果已经寻找过则使用寻找的数据"))
				.OnClicked_Lambda([this]()
				{
					ImportFile();
					return FReply::Handled();
				})
			]
		]

	];
}

void SDoodleImportFbxUI::AddReferencedObjects(FReferenceCollector& collector)
{
	collector.AddReferencedObjects(ListImportData);
}

TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SDoodleImportFbxUI)
		]; // 这里创建我们自己的界面
}

bool SDoodleImportFbxUI::IsCamera(const FString& In_File)
{
	auto LPath = FPaths::GetBaseFilename(In_File);

	return LPath.Contains(TEXT("camera"));
}

void SDoodleImportFbxUI::ImportFile()
{
	FScopedSlowTask L_Task_Scoped1{
		static_cast<float>(ListImportData.Num() * 2), LOCTEXT("ImportFile1", "加载 fbx 文件中...")
	};
	L_Task_Scoped1.MakeDialog();
	for (auto&& i : ListImportData)
	{
		L_Task_Scoped1.EnterProgressFrame(1.0f, LOCTEXT("ImportFile2", "导入文件中"));
		i->ImportFile();
	}
	for (auto&& i : ListImportData)
	{
		L_Task_Scoped1.EnterProgressFrame(1.0f, LOCTEXT("ImportFile2", "导入文件中"));
		i->AssembleScene();
	}
}

void SDoodleImportFbxUI::GenPathPrefix(const FString& In_Path_Prefix, EImportSuffix In_Path_Suffix)
{
	Path_Prefix = In_Path_Prefix;
	Path_Suffix = In_Path_Suffix;

	ListImportGui->RebuildList();
}

void SDoodleImportFbxUI::SetFbxOnlyAnim()
{
	TSet<FString> L_Abc_Path{};
	for (auto&& L_Fbx : ListImportData)
	{
		if (FPaths::GetExtension(L_Fbx->ImportPath, true) == TEXT(".abc"))
		{
			FString L_Path = FPaths::GetPath(L_Fbx->ImportPath) / FPaths::GetBaseFilename(L_Fbx->ImportPath);
			L_Path += ".fbx";
			FPaths::NormalizeFilename(L_Path);
			L_Abc_Path.Emplace(L_Path);
		}
	}

	for (auto&& L_Fbx : ListImportData)
	{
		FString L_Path = L_Fbx->ImportPath;
		FPaths::NormalizeFilename(L_Path);
		if (L_Abc_Path.Contains(L_Path))
		{
			if (const auto L_F = Cast<UDoodleFbxImport_1>(L_Fbx))
			{
				L_F->OnlyAnim = false;
			}
		}
	}
}

void SDoodleImportFbxUI::MatchCameraAndFile()
{
	for (auto L_File : ListImportData)
	{
		if (L_File->IsA<UDoodleFbxCameraImport_1>())
		{
			const auto L_Cam = CastChecked<UDoodleFbxCameraImport_1>(L_File);
			for (auto L_File2 : ListImportData)
			{
				if (L_File != L_File2 && L_File->Eps == L_File2->Eps && L_File->Shot == L_File2->Shot &&
					L_File->ShotAb == L_File2->ShotAb)
				{
					if (const auto L_Fbx = Cast<UDoodleFbxImport_1>(L_File2); L_Fbx)
					{
						L_Fbx->CameraImport = L_Cam;
					}
					else if (const auto L_Abc = Cast<UDoodleAbcImport_1>(L_File2); L_Abc)
					{
						L_Abc->CameraImport = L_Cam;
					}
				}
			}
		}
	}
}

void SDoodleImportFbxUI::AddFile(const FString& In_File)
{
	/// @brief 先扫描前缀
	if (this->Path_Prefix.IsEmpty())
		this->Path_Prefix = UDoodleBaseImportData::GetPathPrefix(In_File);


	/// @brief 寻找到相同的就跳过
	if (ListImportData.FindByPredicate([&](const SDoodleImportFbxUI::UDoodleBaseImportDataPtrType& In_FBx)
	{
		return In_FBx->ImportPath == In_File;
	}))
		return;

	SDoodleImportFbxUI::UDoodleBaseImportDataPtrType L_File{};
	/// 扫描fbx 和abc 文件
	if (FPaths::FileExists(In_File) && FPaths::GetExtension(In_File, true) == TEXT(".fbx"))
	{
		if (IsCamera(In_File))
			L_File = ListImportData.Emplace_GetRef(NewObject<UDoodleFbxCameraImport_1>());
		else if (this->OnlyCamera != ECheckBoxState::Checked)
			L_File = ListImportData.Emplace_GetRef(NewObject<UDoodleFbxImport_1>());
	}
	if (this->OnlyCamera != ECheckBoxState::Checked)
		if (FPaths::FileExists(In_File) && FPaths::GetExtension(In_File, true) == TEXT(".abc"))
		{
			auto&& [L_Ploy, L_Curve] = GetAlembicMeshNumAndCurveNum(In_File);
			if (L_Ploy && !L_Curve)
				L_File = ListImportData.Emplace_GetRef(NewObject<UDoodleAbcImport_1>());
			else if (!L_Ploy && L_Curve)
				L_File = ListImportData.Emplace_GetRef(NewObject<UDoodleXgenImport_1>());
		}
	if (L_File)
	{
		L_File->ImportPath = In_File;
		L_File->ImportUI = this;
		L_File->GenStartAndEndTime();
	}
}


// DragBegin
FReply SDoodleImportFbxUI::OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent)
{
	const auto L_Opt = InDragDropEvent.GetOperationAs<FExternalDragOperation>();
	return L_Opt && L_Opt->HasFiles() ? FReply::Handled() : FReply::Unhandled();
}

FReply SDoodleImportFbxUI::OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent)
{
	const auto L_Opt = InDragDropEvent.GetOperationAs<FExternalDragOperation>();

	if (!(L_Opt && L_Opt->HasFiles())) return FReply::Unhandled();

	ListImportData.Empty();
	AllSkinObjs.Empty();
	// 优先扫描内部的sk
	// AllSkinObjs = FDoodleUSkeletonData_1::ListAllSkeletons();

	for (auto&& Path : L_Opt->GetFiles())
	{
		// 目录进行迭代
		if (FPaths::DirectoryExists(Path))
			IFileManager::Get().IterateDirectoryRecursively(*Path, [this](const TCHAR* InPath, bool in_) -> bool
			{
				AddFile(InPath);
				return true;
			});

		else if (FPaths::FileExists(Path))
			AddFile(Path);
	}
	GenPathPrefix(Path_Prefix, Path_Suffix);
	SetFbxOnlyAnim();
	// 优先相机
	ListImportData.StableSort([](const UDoodleBaseImportData& In_R, const UDoodleBaseImportData& In_L)
	{
		return In_R.IsA<UDoodleFbxCameraImport_1>();
	});
	MatchCameraAndFile();
	ListImportGui->RebuildList();

	return FReply::Handled();
}

// DragEnd

TArray<FDoodleUSkeletonData_1> FDoodleUSkeletonData_1::ListAllSkeletons()
{
	FScopedSlowTask L_Task_Scoped{2.0f, LOCTEXT("Import_Fbx2", "扫描所有的Skin")};
	L_Task_Scoped.MakeDialog();
	TArray<FDoodleUSkeletonData_1> L_AllSkinObjs{};

	FARFilter LFilter{};
	LFilter.bIncludeOnlyOnDiskAssets = false;
	LFilter.bRecursivePaths = true;
	LFilter.bRecursiveClasses = true;
	LFilter.ClassPaths.Add(USkeleton::StaticClass()->GetClassPathName());

	IAssetRegistry::Get()->EnumerateAssets(LFilter, [&](const FAssetData& InAss) -> bool
	{
		if (FPaths::IsUnderDirectory(InAss.PackagePath.ToString(), TEXT("/Game/Shot/")))
		{
			return true;
		}
		if (USkeleton* L_SK = Cast<USkeleton>(InAss.GetAsset()))
		{
			FDoodleUSkeletonData_1& L_Ref_Data = L_AllSkinObjs.Emplace_GetRef();
			L_Ref_Data.SkinObj = L_SK;
			for (auto&& L_Item : L_SK->GetReferenceSkeleton().GetRawRefBoneInfo())
				L_Ref_Data.BoneNames.Add(L_Item.ExportName);
		}
		return true;
	});
	const FRegexPattern L_Reg_Ep_Pattern{LR"((SK_)?(\w+)_Skeleton)"};
	for (auto&& L_Sk : L_AllSkinObjs)
	{
		if (FRegexMatcher L_Reg{L_Reg_Ep_Pattern, L_Sk.SkinObj->GetName()}; L_Reg.FindNext())
		{
			FString L_Str = L_Reg.GetCaptureGroup(2);
			L_Sk.SkinTag = L_Str.IsEmpty() ? L_Reg.GetCaptureGroup(1) : L_Str;
		}
	}
	return L_AllSkinObjs;
}

FString SDoodleImportFbxUI::GetReferencerName() const { return TEXT("SDoodleImportFbxUI"); }

const FString& SDoodleImportFbxUI::GetUserFolderName() const
{
	return UserFolderName;
}

const FString& SDoodleImportFbxUI::GetPathPrefix() const
{
	return Path_Prefix;
}

const EImportSuffix& SDoodleImportFbxUI::GetPathSuffix() const
{
	return Path_Suffix;
}


#undef LOCTEXT_NAMESPACE
