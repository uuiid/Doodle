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
#include "AbcImportSettings.h"
#include "AlembicImportFactory.h"
#include "CineCameraComponent.h"
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
#include "GroomCacheImportOptions.h"
#include "InterchangeGenericAssetsPipeline.h"
#include "ObjectTools.h"
#include "SkeletonTreeBuilder.h"


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


		return {NumType<Alembic::AbcGeom::IPolyMesh>(L_Archive.getTop()), NumType<Alembic::AbcGeom::ICurves>(L_Archive.getTop())};
	}
} // namespace


void UDoodleBaseImport::ImportFileFbx(const FDoodleListViewData& In_Path)
{
	UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
	L_Data->GroupName = TEXT("doodle import");
	L_Data->Filenames.Add(In_Path.Path);
	L_Data->DestinationPath = In_Path.ImportPathDir;
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
	if (In_Path.Skeleton)
	{
		K_FBX_F->ImportUI->Skeleton = In_Path.Skeleton;
		K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_Animation;
		K_FBX_F->ImportUI->bImportMesh = false;
	}
	else
	{
		K_FBX_F->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
		K_FBX_F->ImportUI->bImportAsSkeletal = true;
		K_FBX_F->ImportUI->bImportMesh = true;
	}
	const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UAnimSequence* L_AnimSequence{};
	USkeletalMesh* L_SkeletalMesh{};
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (TArray<UObject*> L_Objs = AssetToolsModule.Get().ImportAssetsAutomated(L_Data); !L_Objs.IsEmpty())
	{
		for (UObject* L_Obj : L_Objs)
		{
			if (L_Obj->IsA(UAnimSequence::StaticClass()))
			{
				L_AnimSequence = CastChecked<UAnimSequence>(L_Obj);
				L_SkeletalMesh = L_AnimSequence->GetSkeleton()->FindCompatibleMesh();
			}
			else if (L_Obj->IsA(USkeletalMesh::StaticClass()))
			{
				L_SkeletalMesh = CastChecked<USkeletalMesh>(L_Obj);
				TArray<FAssetData> OutAssetData;
				IAssetRegistry::Get()->GetAssetsByPath(FName(*In_Path.ImportPathDir), OutAssetData, false);
				for (const FAssetData& Asset : OutAssetData)
				{
					EditorAssetSubsystem->SaveLoadedAsset(Asset.GetAsset());
					if (UAnimSequence* Anim = Cast<UAnimSequence>(Asset.GetAsset()); Anim && Anim->GetSkeleton() == L_SkeletalMesh->GetSkeleton())
					{
						L_AnimSequence = Anim;
						break;
					}
				}
			}
		}
	}

	if (!LevelSequenceMap.Contains(In_Path)) return;
	if (!L_AnimSequence) return;
	if (!L_SkeletalMesh) return;

	L_AnimSequence->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(
		L_AnimSequence, TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression")
	);
	if (L_AnimSequence->IsDataModelValid())
	{
		L_AnimSequence->CompressCommandletVersion = 0;
		L_AnimSequence->ClearAllCachedCookedPlatformData();
		L_AnimSequence->CacheDerivedDataForCurrentPlatform();
	}


	auto L_ShotLevel = LevelSequenceMap[In_Path].LevelSequenceWorld;
	auto L_ShotSequence = LevelSequenceMap[In_Path].LevelSequence;
	ASkeletalMeshActor* L_Actor =
		L_ShotLevel->SpawnActor<ASkeletalMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	L_Actor->SetActorLabel(L_SkeletalMesh->GetName());
	L_Actor->GetSkeletalMeshComponent()->SetSkeletalMesh(L_SkeletalMesh);
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
	UMovieSceneSection* AnimSection = L_MovieSceneSkeletalAnim->AddNewAnimationOnRow(In_Path.StartTime * FrameTick, L_AnimSequence, -1);
	AnimSection->SetPreRollFrames(50 * FrameTick);
	L_Actor->Modify();
	L_ShotLevel->Modify();
	EditorAssetSubsystem->SaveLoadedAssets({L_ShotSequence, L_ShotLevel});
	EditorAssetSubsystem->SaveDirectory(In_Path.ImportPathDir);
}


ULevelSequence* UDoodleBaseImport::CreateLevelSequence(const FString& InCreatePath, const FFrameNumber& In_End) const
{
	ULevelSequence* L_ShotSequence = LoadObject<ULevelSequence>(nullptr, *InCreatePath);

	// 创建定序器
	// ULevelSequenceFactoryNew* L_ShotSequenceFactory = ULevelSequenceFactoryNew::;
	if (!L_ShotSequence)
	{
		const FString L_Package_Name = UPackageTools::SanitizePackageName(InCreatePath);
		UPackage* L_Pkg = CreatePackage(*L_Package_Name);
		TArray<UPackage*> L_TopLevelPackages{L_Pkg};
		UPackageTools::HandleFullyLoadingPackages(L_TopLevelPackages, LOCTEXT("CreateANewObject", "Create a new object"));
		L_ShotSequence = NewObject<ULevelSequence>(
			L_Pkg, FName(*FPaths::GetBaseFilename(InCreatePath)), RF_Public | RF_Standalone | RF_Transactional
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
	}


	// 设置定序器属性
	L_ShotSequence->GetMovieScene()->SetDisplayRate(Rate);
	L_ShotSequence->GetMovieScene()->SetTickResolutionDirectly(TickRate);

	L_ShotSequence->GetMovieScene()->Modify();

	/// 设置范围
	FFrameNumber offset{50};
	L_ShotSequence->GetMovieScene()->SetWorkingRange((Start - 30 - offset) / Rate, (In_End + 30) / Rate);
	L_ShotSequence->GetMovieScene()->SetViewRange((Start - 30 - offset) / Rate, (In_End + 30) / Rate);
	L_ShotSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{
		(Start - offset) * FrameTick,
		(In_End + 1) * FrameTick
	}, true);
	return L_ShotSequence;
}


void UDoodleBaseImport::ImportFileCamera(const FDoodleListViewData& In_Path)
{
	/// ==== Phase 1: Init ====
	FFrameNumber L_End{1200};

	FScopedSlowTask L_Task_Scoped{6.0f, LOCTEXT("Import_CAm", "导入camera")};
	L_Task_Scoped.MakeDialog();

	L_Task_Scoped.EnterProgressFrame(
		1, FText::Format(
			LOCTEXT("Import_ImportingCameraFile1", "导入 \"{0}\"..."),
			FText::FromString(FPaths::GetBaseFilename(In_Path.Path))
		)
	);

	/// ==== Phase 2: Create / Load World ====
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	L_Task_Scoped.EnterProgressFrame(
		1, FText::Format(LOCTEXT("Import_ImportingCameraFile7", "检查关卡\"{0}\"..."), FText::FromString(In_Path.ImportPathDir))
	);

	UWorld* L_ShotLevel{};
	{
		FString PackageName = GetWorldPath(In_Path);
		L_ShotLevel = LoadObject<UWorld>(nullptr, *PackageName);
		if (!L_ShotLevel)
		{
			const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
			const FString BaseFileName = FPaths::GetBaseFilename(PackageName);

			FAssetToolsModule& AssetToolsModule =
				FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			L_ShotLevel = CastChecked<UWorld>(
				AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), NewObject<UWorldFactory>()));

			EditorAssetSubsystem->SaveLoadedAsset(L_ShotLevel, false);
			L_ShotLevel = LoadObject<UWorld>(nullptr, *PackageName);
		}
	}

	/// ==== Phase 3: Configure FBX Import Settings & Open FBX ====
	UMovieSceneUserImportFBXSettings* L_ImportFBXSettings = GetMutableDefault<UMovieSceneUserImportFBXSettings>();
	FFBXInOutParameters InOutParams;

	L_ImportFBXSettings->bMatchByNameOnly = false;
	L_ImportFBXSettings->bCreateCameras = false;
	L_ImportFBXSettings->bReplaceTransformTrack = true;
	L_ImportFBXSettings->bReduceKeys = false;

	if (!MovieSceneToolHelpers::ReadyFBXForImport(In_Path.Path, L_ImportFBXSettings, InOutParams))
		return;


	/// ==== Phase 4: Read FBX Time Range ====
	UnFbx::FFbxImporter* L_FbxImporter = UnFbx::FFbxImporter::GetInstance();
	L_FbxImporter->ImportFromFile(*In_Path.Path, FPaths::GetExtension(In_Path.Path));
	ON_SCOPE_EXIT
		{
			L_FbxImporter->ReleaseScene();
		};
	fbxsdk::FbxTimeSpan L_Fbx_Time = L_FbxImporter->GetAnimationTimeSpan(
		L_FbxImporter->Scene->GetRootNode(), L_FbxImporter->Scene->GetCurrentAnimationStack()
	);
	L_End = (int32)L_Fbx_Time.GetStop().GetFrameCount(fbxsdk::FbxTime::ePAL);
	UE_LOG(LogTemp, Log, TEXT("fbx time %d -> %d"), Start.Value, L_End.Value);

	/// ==== Phase 5: Create / Load LevelSequence ====
	L_Task_Scoped.EnterProgressFrame(
		1,
		FText::Format(LOCTEXT("Import_ImportingCameraFile2", "检查定序器 \"{0}\"..."), FText::FromString(In_Path.ImportPathDir))
	);

	ULevelSequence* L_ShotSequence = LoadObject<ULevelSequence>(nullptr, *In_Path.ImportPathDir);
	if (!L_ShotSequence)
	{
		L_ShotSequence = CreateLevelSequence(In_Path.ImportPathDir, L_End);
	}

	L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile3", "设置定序器以及相机 ..."));

	/// ==== Phase 6: Add Tracks (Visibility + Sub) ====
	// Add / replace LevelVisibility track
	{
		UMovieSceneLevelVisibilityTrack* Track =
			L_ShotSequence->GetMovieScene()->FindTrack<UMovieSceneLevelVisibilityTrack>();
		L_ShotSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(Track));
		Track = L_ShotSequence->GetMovieScene()->AddTrack<UMovieSceneLevelVisibilityTrack>();
		UMovieSceneLevelVisibilitySection* Section =
			CastChecked<UMovieSceneLevelVisibilitySection>(Track->CreateNewSection());
		Section->SetRange(L_ShotSequence->GetMovieScene()->GetPlaybackRange());
		Track->AddSection(*Section);
	}

	// Add / replace Sub track
	{
		UMovieSceneSubTrack* Track = L_ShotSequence->GetMovieScene()->FindTrack<UMovieSceneSubTrack>();
		L_ShotSequence->GetMovieScene()->RemoveTrack(*Cast<UMovieSceneTrack>(Track));
		Track = L_ShotSequence->GetMovieScene()->AddTrack<UMovieSceneSubTrack>();
		UMovieSceneSubSection* Section = CastChecked<UMovieSceneSubSection>(Track->CreateNewSection());
		Section->SetRange(L_ShotSequence->GetMovieScene()->GetPlaybackRange());
		Track->AddSection(*Section);
	}

	L_ShotSequence->Modify();

	/// ==== Phase 7: Spawn Camera Actor & Bind to Sequence ====
	ALevelSequenceActor* L_LevelSequenceActor{};

	// Scope: camera setup, evaluation, import
	{
		UMovieScene* L_Move = L_ShotSequence->GetMovieScene();
		ACineCameraActor* L_CameraActor{};
		UMovieSceneTrack* L_Task = L_ShotSequence->GetMovieScene()->GetCameraCutTrack();

		ULevelSequencePlayer* L_LevelSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
			GWorld->PersistentLevel, L_ShotSequence, FMovieSceneSequencePlaybackSettings{}, L_LevelSequenceActor
		);
		FGuid L_CamGuid;
		FMovieSceneSequenceID L_CamSequenceID{};

		// First import: create camera and bindings
		if (!L_Task)
		{
			FActorSpawnParameters SpawnParams{};
			SpawnParams.ObjectFlags &= ~RF_Transactional;
			if (auto* Actor = CastChecked<ACineCameraActor>(L_ShotLevel->SpawnActor<ACineCameraActor>(SpawnParams)))
			{
				L_CameraActor = Actor;

				UMovieScene* L_MoveScene = L_ShotSequence->GetMovieScene();

				// Bind possessable to level
				L_CamGuid = L_MoveScene->AddPossessable(L_CameraActor->GetName(), L_CameraActor->GetClass());
				L_ShotSequence->BindPossessableObject(L_CamGuid, *L_CameraActor, L_ShotLevel);

				// Create spawnable binding
				const FMovieSceneBindingReference* BindingRef =
					L_ShotSequence->GetBindingReferences()->GetReference(L_CamGuid, 0);
				UMovieSceneCustomBinding* Bind =
					NewObject<UMovieSceneSpawnableActorBinding>()->CreateCustomBindingFromBinding(
						*BindingRef, L_CameraActor, *L_MoveScene
					);
				Bind->SetupDefaults(nullptr, BindingRef->ID, *L_MoveScene);

				// Camera cut section
				MovieSceneToolHelpers::CreateCameraCutSectionForCamera(L_MoveScene, L_CamGuid, Start);

				// Spawn track (always active)
				UMovieSceneSpawnTrack* SpawnTrack = L_MoveScene->AddTrack<UMovieSceneSpawnTrack>(L_CamGuid);
				UMovieSceneSpawnSection* SpawnSection =
					CastChecked<UMovieSceneSpawnSection>(SpawnTrack->CreateNewSection());
				SpawnSection->GetChannel().Reset();
				SpawnSection->GetChannel().SetDefault(true);
				SpawnTrack->AddSection(*SpawnSection);
			}

			L_Task = L_ShotSequence->GetMovieScene()->GetCameraCutTrack();
		}

		if (!L_Task)
			return;

		/// ==== Phase 8: Force Evaluate to Spawn Camera ====
		{
			L_LevelSequencePlayer->Play();
			if (!FSlateApplication::IsInitialized())
			{
				GEngine->UpdateTimeAndHandleMaxTickRate();
				GEngine->Tick(FApp::GetDeltaTime(), false);
			}
		}

		/// ==== Phase 9: Find Camera Actor & GUID ====
		for (auto&& Section : L_Task->GetAllSections())
		{
			FMovieSceneObjectBindingID BindID =
				Cast<UMovieSceneCameraCutSection>(Section)->GetCameraBindingID();
			L_CamGuid = BindID.GetGuid();
			L_CamSequenceID = BindID.GetRelativeSequenceID();

			if (L_CameraActor)
				break;

			for (auto&& Obj : L_LevelSequencePlayer->FindBoundObjects(BindID.GetGuid(), BindID.GetRelativeSequenceID()))
			{
				if (Obj.Get()->IsA<ACineCameraActor>())
				{
					L_CameraActor = CastChecked<ACineCameraActor>(Obj.Get());
					break;
				}
			}
			if (L_CameraActor)
				break;
		}

		FString CamLabel = L_CameraActor->GetActorNameOrLabel();
		UE_LOG(LogTemp, Log, TEXT("camera name %s, guid %s"), *CamLabel, *L_CamGuid.ToString());

		/// ==== Phase 10: Import FBX Camera Data ====
		TMap<FGuid, FString> CameraMap{{L_CamGuid, CamLabel}};

		L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile6", "导入FBX相机数据..."));

		MovieSceneToolHelpers::ImportFBXCameraToExisting(
			L_FbxImporter, L_ShotSequence, L_LevelSequencePlayer, L_CamSequenceID, CameraMap, false, false
		);
		MovieSceneToolHelpers::ImportFBXIfReady(
			GWorld, L_ShotSequence, L_LevelSequencePlayer, L_CamSequenceID, CameraMap, L_ImportFBXSettings, InOutParams
		);

		/// ==== Phase 11: Post-Process Settings ====
		L_CameraActor->GetCineCameraComponent()->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
		L_CameraActor->GetCineCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount = true;
		L_CameraActor->GetCineCameraComponent()->PostProcessSettings.MotionBlurAmount = 0.1f;
		L_CameraActor->MarkPackageDirty();
	}

	/// ==== Phase 12: Cleanup & Save ====
	L_LevelSequenceActor->Destroy();
	if (!FSlateApplication::IsInitialized())
	{
		GEngine->UpdateTimeAndHandleMaxTickRate();
		GEngine->Tick(FApp::GetDeltaTime(), false);
	}
	LevelSequenceMap.Add(In_Path, FDoodleBaseImportValuePair{L_ShotSequence, L_ShotLevel});
	UEditorAssetLibrary::SaveAsset(L_ShotSequence->GetPathName());
}


void UDoodleBaseImport::ImportFileAbc(const FDoodleListViewData& In_Path)
{
	UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
	L_Data->GroupName = TEXT("doodle import");
	L_Data->Filenames.Add(In_Path.Path);
	L_Data->DestinationPath = In_Path.ImportPathDir;
	L_Data->bReplaceExisting = true;
	L_Data->bSkipReadOnly = true;
	L_Data->bReplaceExisting = true;

	UAlembicImportFactory* k_abc_f = NewObject<UAlembicImportFactory>(L_Data);
	L_Data->Factory = k_abc_f;
	UAbcImportSettings* k_abc_stting = NewObject<UAbcImportSettings>(L_Data);
	k_abc_f->ImportSettings = k_abc_stting;

	k_abc_stting->ImportType = EAlembicImportType::GeometryCache; // 导入为几何缓存
	k_abc_stting->ConversionSettings.bFlipV = true;
	k_abc_stting->ConversionSettings.Scale.X = 1.0;
	k_abc_stting->ConversionSettings.Scale.Y = -1.0;
	k_abc_stting->ConversionSettings.Scale.Z = 1.0;
	k_abc_stting->ConversionSettings.Rotation.X = 90.0;
	k_abc_stting->ConversionSettings.Rotation.Y = 0.0;
	k_abc_stting->ConversionSettings.Rotation.Z = 0.0;
	k_abc_stting->MaterialSettings.bFindMaterials = true;

	k_abc_stting->GeometryCacheSettings.bFlattenTracks = true; // 合并轨道
	k_abc_stting->SamplingSettings.bSkipEmpty = false; // 跳过空白帧
	k_abc_stting->SamplingSettings.FrameStart = In_Path.StartTime; // 开始帧
	k_abc_stting->SamplingSettings.FrameEnd = In_Path.EndTime; // 结束帧
	k_abc_stting->SamplingSettings.FrameSteps = 1; // 帧步数

	const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<UObject*> L_Geos = AssetToolsModule.Get().ImportAssetsAutomated(L_Data);
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	EditorAssetSubsystem->SaveLoadedAssets(L_Geos);
	UGeometryCache* L_GeometryCache{};
	if (!L_Geos.IsEmpty())
		L_GeometryCache = CastChecked<UGeometryCache>(L_Geos.Top());

	if (!LevelSequenceMap.Contains(In_Path)) return;
	if (!L_GeometryCache) return;

	auto L_ShotLevel = LevelSequenceMap[In_Path].LevelSequenceWorld;
	auto L_ShotSequence = LevelSequenceMap[In_Path].LevelSequence;

	AGeometryCacheActor* L_Actor =
		L_ShotLevel->SpawnActor<AGeometryCacheActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	L_Actor->SetActorLabel(L_GeometryCache->GetName());
	L_Actor->GetGeometryCacheComponent()->SetGeometryCache(L_GeometryCache);
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
		L_MovieSceneGeoTrack->AddNewAnimation(In_Path.StartTime * FrameTick, L_Actor->GetGeometryCacheComponent());
	AnimSection->SetPreRollFrames(50 * FrameTick);
	L_Actor->Modify();
	L_ShotLevel->Modify();
	EditorAssetSubsystem->SaveLoadedAssets({L_ShotSequence, L_ShotLevel});
}


void ImportFile_xgen()
{
	// UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
	// L_Data->GroupName = TEXT("doodle import");
	// L_Data->Filenames.Add(ImportPath);
	// L_Data->DestinationPath = ImportPathDir;
	// L_Data->bReplaceExisting = true;
	// L_Data->bSkipReadOnly = true;
	// L_Data->bReplaceExisting = true;
	//
	//
	// TArray<UClass*> L_ImportUiltClasses;
	// GetDerivedClasses(UFactory::StaticClass(),
	// 	L_ImportUiltClasses);
	// for (auto&& i : L_ImportUiltClasses)
	// {
	// 	// i->GetName();
	// 	UE_LOG(LogTemp, Log, TEXT("get class name %s"), *i->GetName());
	// 	if (i->GetName() == "HairStrandsFactory")
	// 		L_Data->Factory = DuplicateObject(i->GetDefaultObject<UFactory>(), L_Data);
	// }
	// if (!L_Data->Factory) return;
	// TArray<FString> L_String{};
	// bool bImportWasCancelled = false;
	// auto L_Name = ObjectTools::SanitizeObjectName(FPaths::GetBaseFilename(ImportPath));
	// FString PackageName = ObjectTools::SanitizeInvalidChars(FPaths::Combine(*ImportPathDir, *L_Name), INVALID_LONGPACKAGE_CHARACTERS);
	// UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	// if (!EditorAssetSubsystem->DoesDirectoryExist(ImportPathDir))
	// {
	// 	EditorAssetSubsystem->MakeDirectory(ImportPathDir);
	// }
	//
	//
	// UPackage* Pkg = CreatePackage(*PackageName);
	// Pkg->MarkAsFullyLoaded();
	// Pkg->SetIsExternallyReferenceable(true);
	// if (!ensure(Pkg))
	// {
	// 	// Failed to create the package to hold this asset for some reason
	// 	return;
	// }
	// L_Data->Factory->GetSupportedFileExtensions(L_String);
	//
	// L_Data->Factory->AssetImportTask = NewObject<UAssetImportTask>(L_Data);
	// UGroomCacheImportOptions* L_Opt = NewObject<UGroomCacheImportOptions>(L_Data->Factory->AssetImportTask);
	// L_Data->Factory->AssetImportTask->Options = L_Opt;
	// L_Data->Factory->AssetImportTask->bAutomated = true;
	// L_Opt->ImportSettings.FrameStart = StartTime;
	// L_Opt->ImportSettings.FrameEnd = EndTime;
	// L_Opt->ImportSettings.ConversionSettings.Rotation.X = -90.0;
	// L_Opt->ImportSettings.ConversionSettings.Rotation.Z = 180.0;
	// L_Opt->ImportSettings.ConversionSettings.Scale.X = -1.0;
	// L_Opt->ImportSettings.bImportGroomCache = true;
	// L_Data->Factory->ImportObject(L_Data->Factory->ResolveSupportedClass(), Pkg, FName{*L_Name}, RF_Public | RF_Standalone | RF_Transactional,
	// 	ImportPath, nullptr, bImportWasCancelled);
	// const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	//
	// TArray<UObject*> L_Geos = AssetToolsModule.Get().ImportAssetsAutomated(L_Data);
	// UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	// EditorAssetSubsystem->SaveLoadedAssets(L_Geos);
}

void UDoodleBaseImport::ImportFile(const FDoodleListViewData& In_Path)
{
	switch (In_Path.IsCamera)
	{
	case FDoodleImportType::Fbx:
	case FDoodleImportType::AbcGeoCache:
		if (!LevelSequenceMap.Contains(In_Path))
			LoadLevelSequenceAndWorld(In_Path);
		break;
	default:
		break;
	}
	switch (In_Path.IsCamera)
	{
	case FDoodleImportType::Fbx:
		ImportFileFbx(In_Path);
		break;
	case FDoodleImportType::AbcHair:
		ImportFileAbc(In_Path);
		break;
	case FDoodleImportType::Camera:
		ImportFileCamera(In_Path);
		break;
	case FDoodleImportType::AbcGeoCache:
	case FDoodleImportType::End:
	default:
		break;
	}
	CreateOtherData(In_Path);
}

void UDoodleBaseImport::LoadLevelSequenceAndWorld(const FDoodleListViewData& In_Path)
{
	auto L_LevelSequence = GetLevelPath(In_Path);
	ULevelSequence* L_ShotSequence = LoadObject<ULevelSequence>(nullptr, *In_Path.ImportPathDir);
	auto L_World = GetWorldPath(In_Path);
	UWorld* L_ShotLevel = LoadObject<UWorld>(nullptr, *L_World);
	if (L_ShotSequence && L_ShotLevel)
		LevelSequenceMap.Add(In_Path, FDoodleBaseImportValuePair{L_ShotSequence, L_ShotLevel});
}

FDoodleListViewData UDoodleBaseImport::ParseFiles(const FString& InPath)
{
	FDoodleListViewData L_Result{};
	L_Result.Path = InPath;
	FString BaseName = FPaths::GetBaseFilename(InPath);
	if (FPaths::GetExtension(InPath).ToLower() == TEXT("abc"))
		L_Result.IsCamera = FDoodleImportType::AbcGeoCache;
	else if (FPaths::GetExtension(InPath).ToLower() == TEXT("fbx"))
	{
		if (BaseName.Find("_camera_") != INDEX_NONE)
			L_Result.IsCamera = FDoodleImportType::Camera;
		else
			L_Result.IsCamera = FDoodleImportType::Fbx;
	}
	if (int32 L_Index = INDEX_NONE; BaseName.FindChar('_', L_Index))
		L_Result.ProjectName = BaseName.LeftChop(BaseName.Len() - L_Index);

	const FRegexPattern L_Reg_Time_Pattern{LR"(_(\d+)-(\d+))"};
	FRegexMatcher L_Reg_Time{L_Reg_Time_Pattern, BaseName};
	L_Result.StartTime = {1000};
	L_Result.EndTime = {1001};
	if (L_Reg_Time.FindNext() && L_Reg_Time.GetEndLimit() > 2)
	{
		L_Result.StartTime = FCString::Atoi64(*L_Reg_Time.GetCaptureGroup(1));
		L_Result.EndTime = FCString::Atoi64(*L_Reg_Time.GetCaptureGroup(2));
	}
	const FRegexPattern L_Reg_Ep_Pattern{LR"((ep|EP|Ep)_?(\d+))"};

	if (FRegexMatcher L_Reg_Ep{L_Reg_Ep_Pattern, InPath}; L_Reg_Ep.FindNext())
	{
		L_Result.Eps = FCString::Atoi64(*L_Reg_Ep.GetCaptureGroup(2));
	}

	const FRegexPattern L_Reg_ScPattern{LR"((sc|SC|Sc)_?(\d+)([a-zA-Z])?)"};
	if (FRegexMatcher L_Reg_Sc{L_Reg_ScPattern, InPath}; L_Reg_Sc.FindNext())
	{
		L_Result.Shot = FCString::Atoi64(*L_Reg_Sc.GetCaptureGroup(2));
		if (L_Reg_Sc.GetEndLimit() > 3)
		{
			L_Result.ShotAb = L_Reg_Sc.GetCaptureGroup(3).ToUpper();
		}
	}
	switch (L_Result.IsCamera)
	{
	case FDoodleImportType::Fbx:
	case FDoodleImportType::AbcGeoCache:
	case FDoodleImportType::AbcHair:
		L_Result.ImportPathDir = GetImportPath(L_Result);
		break;
	case FDoodleImportType::Camera:
		L_Result.ImportPathDir = GetLevelPath(L_Result);
		break;
	case FDoodleImportType::End:
		break;
	}
	check(!L_Result.ImportPathDir.IsEmpty());
	return L_Result;
}


FString UDoodleLightImport::GetImportPath(const FDoodleParseFileImportData& In_Path) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_Path.ProjectName.ToUpper(), In_Path.Eps,
		In_Path.Shot,
		*In_Path.ShotAb);

	switch (In_Path.IsCamera)
	{
	case FDoodleImportType::Fbx:
		return FString::Printf(
			TEXT("/Game/Shot/ep%.4d/%s/Import_Lig/Fbx_%s"),
			In_Path.Eps, *L_BaseName, *FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))
		);
	case FDoodleImportType::AbcHair:
	case FDoodleImportType::AbcGeoCache:
		return FString::Printf(
			TEXT("/Game/Shot/ep%.4d/%s/Import_Lig/Abc_%s"),
			In_Path.Eps, *L_BaseName, *FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))
		);
	case FDoodleImportType::Camera:
	case FDoodleImportType::End:
	default:
		return {};
	}
}

FString UDoodleLightImport::GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_LevelSequenceKey.ProjectName.ToUpper(), In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);

	return FString::Printf(
		TEXT("/Game/Shot/ep%.4d/%s/%s"),
		In_LevelSequenceKey.Eps, *L_BaseName, *L_BaseName
	);
}

FString UDoodleLightImport::GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_LevelSequenceKey.ProjectName.ToUpper(), In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);

	return FString::Printf(
		TEXT("/Game/Shot/ep%.4d/%s/%s_Zong"),
		In_LevelSequenceKey.Eps, *L_BaseName, *L_BaseName
	);
}

void UDoodleLightImport::CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	const FString LigFolder1 = FString::Printf(TEXT("/Game/Shot/ep%.4d/map/Light_File/level"), In_LevelSequenceKey.Eps);
	const FString LigFolder2 = FString::Printf(TEXT("/Game/Shot/ep%.4d/map/Light_File/other"), In_LevelSequenceKey.Eps);

	if (!EditorAssetSubsystem->DoesDirectoryExist(LigFolder1))
	{
		EditorAssetSubsystem->MakeDirectory(LigFolder1);
	}
	if (!EditorAssetSubsystem->DoesDirectoryExist(LigFolder2))
	{
		EditorAssetSubsystem->MakeDirectory(LigFolder2);
	}

	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"),
		*In_LevelSequenceKey.ProjectName.ToUpper(),
		In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);
	FString L_Folder = FString::Printf(TEXT("/Game/Shot/ep%.4d/%s/Import_Lig"),
		In_LevelSequenceKey.Eps, *L_BaseName
	);
	FString LigFolder3 = L_Folder / L_BaseName;
	if (!EditorAssetSubsystem->DoesAssetExist(LigFolder3))
	{
		ULevelSequence* L_LevelSequence = CreateLevelSequence(LigFolder3, In_LevelSequenceKey.EndTime);
		EditorAssetSubsystem->SaveAsset(LigFolder3, false);
	}
	// 创建对应的 world
	LigFolder3 += TEXT("_LV");
	if (!EditorAssetSubsystem->DoesAssetExist(LigFolder3))
	{
		const FString PackagePath = FPackageName::GetLongPackagePath(LigFolder3);
		const FString BaseFileName = FPaths::GetBaseFilename(LigFolder3);
		FAssetToolsModule& AssetToolsModule =
			FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UWorld* L_Level = CastChecked<UWorld>(
			AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), NewObject<UWorldFactory>()));

		EditorAssetSubsystem->SaveLoadedAsset(L_Level, false);
	}
}


FString UDoodleVfxImport::GetImportPath(const FDoodleParseFileImportData& In_Path) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_Path.ProjectName.ToUpper(), In_Path.Eps,
		In_Path.Shot,
		*In_Path.ShotAb);

	switch (In_Path.IsCamera)
	{
	case FDoodleImportType::Fbx:
		return FString::Printf(
			TEXT("/Game/Shot/ep%.4d/%s/Import_Vfx/Fbx_%s"),
			In_Path.Eps, *L_BaseName, *FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))
		);
	case FDoodleImportType::AbcHair:
	case FDoodleImportType::AbcGeoCache:
		return FString::Printf(
			TEXT("/Game/Shot/ep%.4d/%s/Import_Vfx/Abc_%s"),
			In_Path.Eps, *L_BaseName, *FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))
		);
	case FDoodleImportType::Camera:
	case FDoodleImportType::End:
	default:
		return {};
	}
}

FString UDoodleVfxImport::GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_LevelSequenceKey.ProjectName.ToUpper(), In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);

	return FString::Printf(
		TEXT("/Game/Shot/ep%.4d/%s/Import_WB/%s"),
		In_LevelSequenceKey.Eps, *L_BaseName, *L_BaseName
	);
}

FString UDoodleVfxImport::GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_LevelSequenceKey.ProjectName.ToUpper(), In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);

	return FString::Printf(
		TEXT("/Game/Shot/ep%.4d/%s/Import_WB/%s_LV"),
		In_LevelSequenceKey.Eps, *L_BaseName, *L_BaseName
	);
}

void UDoodleVfxImport::CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	if (auto L_Name = ImportUI->GetUserFolderName();
		L_Name != TEXT("")
	)
	{
		UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
		FString L_Path = FString::Printf(
			TEXT("/Game/Shot/ep%.4d/Vfx/%s "),
			In_LevelSequenceKey.Eps, *L_Name.ToLower()
		);
		if (!EditorAssetSubsystem->DoesDirectoryExist(L_Path))
		{
			EditorAssetSubsystem->MakeDirectory(L_Path);
		}
	}
}


FString UDoodleWbImport::GetImportPath(const FDoodleParseFileImportData& In_Path) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_Path.ProjectName.ToUpper(), In_Path.Eps,
		In_Path.Shot,
		*In_Path.ShotAb);

	switch (In_Path.IsCamera)
	{
	// return FString::Printf(
	// 	TEXT("/Game/Shot/ep%.4d/%s/%s_Zong"),
	// 	In_Path.Eps, *L_BaseName, *L_BaseName
	// );
	case FDoodleImportType::Fbx:
		return FString::Printf(
			TEXT("/Game/Shot/ep%.4d/%s/Import_WB/Fbx_%s"),
			In_Path.Eps, *L_BaseName, *FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))
		);
	case FDoodleImportType::AbcHair:
	case FDoodleImportType::AbcGeoCache:
		return FString::Printf(
			TEXT("/Game/Shot/ep%.4d/%s/Import_WB/Abc_%s"),
			In_Path.Eps, *L_BaseName, *FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))
		);
	case FDoodleImportType::Camera:
	case FDoodleImportType::End:
	default:
		return {};
	}
}

FString UDoodleWbImport::GetLevelPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_LevelSequenceKey.ProjectName.ToUpper(), In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);

	return FString::Printf(
		TEXT("/Game/Shot/ep%.4d/%s/Import_WB/%s"),
		In_LevelSequenceKey.Eps, *L_BaseName, *L_BaseName
	);
}

FString UDoodleWbImport::GetWorldPath(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"), *In_LevelSequenceKey.ProjectName.ToUpper(), In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);

	return FString::Printf(
		TEXT("/Game/Shot/ep%.4d/%s/Import_WB/%s_LV"),
		In_LevelSequenceKey.Eps, *L_BaseName, *L_BaseName
	);
}

void UDoodleWbImport::CreateOtherData(const FDoodleLevelSequenceKey& In_LevelSequenceKey) const
{
	UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	FString L_BaseName = FString::Printf(TEXT("%s_EP%.3d_SC%.3d%s"),
		*In_LevelSequenceKey.ProjectName.ToUpper(),
		In_LevelSequenceKey.Eps,
		In_LevelSequenceKey.Shot,
		*In_LevelSequenceKey.ShotAb);
	FString L_Folder = FString::Printf(TEXT("/Game/Shot/ep%.4d/%s"),
		In_LevelSequenceKey.Eps, *L_BaseName
	);
	FString LigFolder3 = L_Folder / L_BaseName;
	if (!EditorAssetSubsystem->DoesAssetExist(LigFolder3))
	{
		ULevelSequence* L_LevelSequence = CreateLevelSequence(LigFolder3, In_LevelSequenceKey.EndTime);
		EditorAssetSubsystem->SaveAsset(LigFolder3, false);
	}
	// 创建对应的 world
	LigFolder3 += TEXT("_Zong");
	if (!EditorAssetSubsystem->DoesAssetExist(LigFolder3))
	{
		const FString PackagePath = FPackageName::GetLongPackagePath(LigFolder3);
		const FString BaseFileName = FPaths::GetBaseFilename(LigFolder3);
		FAssetToolsModule& AssetToolsModule =
			FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UWorld* L_Level = CastChecked<UWorld>(
			AssetToolsModule.Get().CreateAsset(BaseFileName, PackagePath, UWorld::StaticClass(), NewObject<UWorldFactory>()));

		EditorAssetSubsystem->SaveLoadedAsset(L_Level, false);
	}
}


class SDoodleImportUiItem final : public SMultiColumnTableRow<SDoodleImportFbxUI::FImportDataType>
{
public:
	SLATE_BEGIN_ARGS(SDoodleImportUiItem)
			: _ItemShow()
		{
		}

		SLATE_ARGUMENT(SDoodleImportFbxUI::FImportDataType, ItemShow)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		ItemShow = InArgs._ItemShow;
		ItemType = StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<int64>(ItemShow->IsCamera));

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
		if (ColumnName == TEXT("Import_Project"))
		{
			return SNew(STextBlock).Text(FText::FromString(ItemShow->ProjectName));
		}
		if (ColumnName == TEXT("Import_File")) // 判断列名为Fbx File，次名称在创建View时，通过SHeaderRow::Column指定
		{
			return SNew(STextBlock).Text(FText::FromString(ItemShow->Path));
		}

		if (ColumnName == TEXT("Time_Ranges"))
		{
			return SNew(STextBlock)
				.Text_Lambda([this]() -> FText
					{
						return FText::FromString(FString::Printf(TEXT("%d - %d"), ItemShow->StartTime, ItemShow->EndTime));
					});
		}
		if (ColumnName == TEXT("Import_Path_Dir"))
		{
			return SNew(STextBlock).Text(FText::FromString(ItemShow->ImportPathDir));
		}
		if (ColumnName == TEXT("Ep_And_Shot"))
		{
			return SNew(STextBlock)
				.Text(FText::FromString(
					FString::Printf(TEXT("EP: %d SC: %d%s"), static_cast<int>(ItemShow->Eps), static_cast<int>(ItemShow->Shot),
						*ItemShow->ShotAb)
				));
		}
		if (ColumnName == TEXT("Skeleton_Path"))
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
								TEXT("%s"), *(ItemShow->Skeleton != nullptr
									? ItemShow->Skeleton->GetPackage()->GetPathName()
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
			ItemShow->Skeleton = Cast<USkeleton>(L_It->GetAsset());
		}
	}

	void DoodleBrowse()
	{
		if (ItemShow->Skeleton != nullptr)
		{
			FContentBrowserModule& L_ContentBrowserModle =
				FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			L_ContentBrowserModle.Get().SyncBrowserToAssets(TArray<FAssetData>{FAssetData{ItemShow->Skeleton}});
		}
	}

	void DoodleReset()
	{
		ItemShow->Skeleton = nullptr;
	}

private:
	SDoodleImportFbxUI::FImportDataType ItemShow;

	FString ItemType{};
};

void SDoodleImportFbxUI::Construct(const FArguments& Arg)
{
	const FSlateFontInfo Font = FAppStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));
	ImportCore = NewObject<UDoodleLightImport>(GetTransientPackage(), NAME_None, RF_Transient);
#if PLATFORM_WINDOWS
	const FString FileFilterText = TEXT("fbx and abc |*.fbx;*.abc|fbx (*.fbx)|*.fbx|abc (*.abc)|*.abc");
#else
	const FString FileFilterText = FString::Printf(TEXT("%s"), *FileFilterType.ToString());
#endif

	UEnum* L_Enum = StaticEnum<EImportSuffix>();
	for (EImportSuffix L_E : TEnumRange<EImportSuffix>())
	{
		All_Path_Suffix.Add(MakeShared<EImportSuffix>(L_E));
	}

	Department = EImportSuffix::Light;

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
					.Text(LOCTEXT("BinaryPathLabel11", "部门"))
					.ColorAndOpacity(FSlateColor{FLinearColor{1, 0, 0, 1}})
					.Font(Font)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(8.0f)
				[
					///  
					SNew(SComboBox<TSharedPtr<EImportSuffix>>)
					.OptionsSource(&All_Path_Suffix)
					.OnSelectionChanged_Lambda(
						[this](const TSharedPtr<EImportSuffix>& In, ESelectInfo::Type)
							{
								Department = *In;
							})
					.OnGenerateWidget_Lambda(
						[this](const TSharedPtr<EImportSuffix>& In)
							{
								return SNew(STextBlock).Text(
										FText::FromString(StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(*In))));
							})
					.InitiallySelectedItem(All_Path_Suffix[0])
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
							{
								return FText::FromString(StaticEnum<EImportSuffix>()->GetNameStringByValue(static_cast<uint8>(Department)));
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
					.OnCheckStateChanged_Lambda([this](ECheckBoxState In_State)
						{
							this->OnlyCamera = In_State;
						})
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
				SAssignNew(ListImportGui, SListView<SDoodleImportFbxUI::FImportDataType>)


				                                                                         .ListItemsSource(&ListImportData)
				                                                                         .ScrollbarVisibility(EVisibility::All)
				                                                                         .OnGenerateRow_Lambda( // 生成小部件
					                                                                         [](SDoodleImportFbxUI::FImportDataType InItem,
					                                                                            const TSharedRef<STableViewBase>& OwnerTable) ->
					                                                                         TSharedRef<ITableRow>
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
					                                                                         .DefaultLabel(LOCTEXT("Import File", "导入的文件"))

					                                                                         + SHeaderRow::Column(TEXT("Import_Project"))
					                                                                         .FillWidth(0.2f)
					                                                                         .DefaultLabel(LOCTEXT("Import File", "项目"))

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
								i->Skeleton = nullptr;
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
	collector.AddReferencedObject(ImportCore);
}

TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SDoodleImportFbxUI)
		]; // 这里创建我们自己的界面
}

void SDoodleImportFbxUI::SwitchDepartment()
{
	switch (Department)
	{
	case EImportSuffix::Light:
		ImportCore = NewObject<UDoodleLightImport>(GetTransientPackage(), NAME_None, RF_Transient);
		break;
	case EImportSuffix::Vfx:
		ImportCore = NewObject<UDoodleVfxImport>(GetTransientPackage(), NAME_None, RF_Transient);
		break;
	case EImportSuffix::WorldBinding:
		ImportCore = NewObject<UDoodleWbImport>(GetTransientPackage(), NAME_None, RF_Transient);
		break;
	default:
		break;;
	}
	TArray<FString> L_AllPaths{};
	for (auto&& i : ListImportData)
		L_AllPaths.Emplace(i->Path);
	ListImportData.Empty();
	for (auto&& i : L_AllPaths)
		AddFile(i);
	// 优先相机
	ListImportData.StableSort([](const FImportDataType& In_R, const FImportDataType& In_L)
		{
			return In_R->IsCamera > In_L->IsCamera;
		});
	ListImportGui->RebuildList();
}

void SDoodleImportFbxUI::ImportFile()
{
	FScopedSlowTask L_Task_Scoped1{
		static_cast<float>(ListImportData.Num() * 2), LOCTEXT("ImportFile1", "加载 fbx 文件中...")
	};
	L_Task_Scoped1.MakeDialog();
	for (auto&& i : ListImportData)
	{
		if (OnlyCamera == ECheckBoxState::Checked && i->IsCamera != FDoodleImportType::Camera) continue;
		ImportCore->ImportFile(*i);
	}
}


void SDoodleImportFbxUI::AddFile(const FString& In_File)
{
	/// @brief 寻找到相同的就跳过
	if (ListImportData.FindByPredicate([&](const SDoodleImportFbxUI::FImportDataType& In_FBx)
		{
			return In_FBx->ImportPath == In_File;
		}))
		return;

	SDoodleImportFbxUI::FImportDataType L_File = MakeShared<FImportDataType::ElementType>(ImportCore->ParseFiles(In_File));
	ListImportData.Emplace(L_File);
}


// DragBegin
FReply SDoodleImportFbxUI::OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent)
{
	const auto L_Opt = InDragDropEvent.GetOperationAs<FExternalDragOperation>();
	return L_Opt && L_Opt->HasFiles()
		? FReply::Handled()
		: FReply::Unhandled();
}

FReply SDoodleImportFbxUI::OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent)
{
	const auto L_Opt = InDragDropEvent.GetOperationAs<FExternalDragOperation>();

	if (!(L_Opt && L_Opt->HasFiles())) return FReply::Unhandled();

	ListImportData.Empty();

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
	// 优先相机
	ListImportData.StableSort([](const FImportDataType& In_R, const FImportDataType& In_L)
		{
			return In_R->IsCamera > In_L->IsCamera;
		});
	ListImportGui->RebuildList();

	return FReply::Handled();
}

// DragEnd
FString SDoodleImportFbxUI::GetReferencerName() const
{
	return TEXT("SDoodleImportFbxUI");
}

const FString& SDoodleImportFbxUI::GetUserFolderName() const
{
	return UserFolderName;
}

const EImportSuffix& SDoodleImportFbxUI::GetDepartment() const
{
	return Department;
}


#undef LOCTEXT_NAMESPACE
