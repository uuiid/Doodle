#include "DoodleCreateLevel.h"

//// 创建world
#include "AssetToolsModule.h"
#include "EditorLevelLibrary.h"
#include "Factories/WorldFactory.h"
#include "IAssetTools.h"
#include "LevelSequence.h"
#include "Modules/ModuleManager.h"
/// 定序器使用
#include "MovieSceneToolHelpers.h"
#include "SequencerSettings.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Misc/OutputDeviceNull.h"

//// 保存操作使用
#include "FileHelpers.h"
/// 相机导入
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "MovieSceneObjectBindingID.h"
/// @brief 我们使用c++ 辅助调用蓝图类的头文件
#include "UDoodleImportUilt.h"

/// 资产注册表
#include "AssetRegistry/AssetRegistryModule.h"
/// 编译蓝图
#include "Kismet2/KismetEditorUtilities.h"
/// 编辑器脚本
#include "EditorAssetLibrary.h"

/// 自动导入类需要
#include "AssetImportTask.h"
/// 导入我们的自定义数据
#include "DoodleAssetImportData.h"
/// 检查包名称存在
#include "Misc/PackageName.h"

/// 生成骨架网格体
#include "Animation/SkeletalMeshActor.h"
/// 动画
#include "Animation/AnimationAsset.h"

/// json需要
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
/// 几何缓存体使用
#include "GeometryCache.h"
/// 导入的fbx动画
#include "Animation/AnimSequence.h"
/// 几何缓存actor 导入
#include "GeometryCacheActor.h"
#include "GeometryCacheComponent.h"

/// 电影 Section
#include "MovieSceneSection.h"
/// 我们自定义的工具
#include "DoodleImportUiltEditor.h"
/// 导入相机的设置
#include "MovieSceneToolsUserSettings.h"

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1)
/// 关卡编辑器子系统
#include "LevelEditorSubsystem.h"
#endif
namespace doodle {
bool init_ue4_project::load_all_blueprint() {
  UE_LOG(LogTemp, Log, TEXT("Loading Asset Registry..."));
  FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
  AssetRegistryModule.Get().SearchAllAssets(/*bSynchronousSearch =*/true);
  UE_LOG(LogTemp, Log, TEXT("Finished Loading Asset Registry."));

  UE_LOG(LogTemp, Log, TEXT("Gathering All Blueprints From Asset Registry..."));

  return AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), blueprint_list, true);
}

bool init_ue4_project::build_all_blueprint() {
  for (auto &&i : blueprint_list) {
    FString const AssetPath = i.ObjectPath.ToString();
    UE_LOG(LogTemp, Log, TEXT("Loading and Compiling: '%s'..."), *AssetPath);

    UBlueprint *l_b =
        Cast<UBlueprint>(
            StaticLoadObject(
                i.GetClass(),
                nullptr,
                *AssetPath,
                nullptr,
                LOAD_NoWarn | LOAD_DisableCompileOnLoad
            )
        );

    if (l_b != nullptr) {
      if (l_b->ParentClass.Get() == UDoodleImportUilt::StaticClass())
        FKismetEditorUtilities::CompileBlueprint(l_b, EBlueprintCompileOptions::SkipGarbageCollection);
    }
  }
  return true;
}

bool init_ue4_project::create_level(const FString &in_path) {
  p_save_level_path = in_path;
  auto &l_ass_tool  = FModuleManager::Get()
                         .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                         .Get();
  UE_LOG(LogTemp, Log, TEXT("关卡路径 %s"), *(FPaths::GetPath(in_path) / FPaths::GetBaseFilename(in_path)));
  if (!FPackageName::DoesPackageExist(in_path)) {
    for (TObjectIterator<UClass> it{}; it; ++it) {
      if (it->IsChildOf(UFactory::StaticClass())) {
        if (it->GetName() == "LevelSequenceFactoryNew") {
          p_level_ = l_ass_tool.CreateAsset(FPaths::GetBaseFilename(in_path), FPaths::GetPath(in_path), ULevelSequence::StaticClass(), it->GetDefaultObject<UFactory>());
        }
      }
    }
  } else {
    p_level_ = LoadObject<ULevelSequence>(nullptr, *in_path);
  }

  if (p_level_ != nullptr)
    p_save_level_path = p_level_->GetPathName();

  return p_level_ != nullptr;
}
bool init_ue4_project::create_world(const FString &in_path) {
  p_save_world_path = in_path;
  auto &l_ass_tool  = FModuleManager::Get()
                         .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                         .Get();
  UE_LOG(LogTemp, Log, TEXT("世界路径 %s"), *(FPaths::GetPath(in_path) / FPaths::GetBaseFilename(in_path)));

  if (!FPackageName::DoesPackageExist(in_path)) {
    p_world_ = l_ass_tool.CreateAsset(
        FPaths::GetBaseFilename(in_path), FPaths::GetPath(in_path), UWorld::StaticClass(),
        UWorldFactory::StaticClass()->GetDefaultObject<UFactory>()
    );
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27
    UEditorLevelLibrary::LoadLevel(in_path);
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1)
    ULevelEditorSubsystem *LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
    LevelEditorSubsystem->LoadLevel(in_path);
#endif
  } else {
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27
    UEditorLevelLibrary::LoadLevel(in_path);
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1)
    ULevelEditorSubsystem *LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
    LevelEditorSubsystem->LoadLevel(in_path);
#endif
    p_world_ = GEditor->GetEditorWorldContext().World();
  }
  if (p_world_ != nullptr)
    p_save_world_path = p_world_->GetPathName();

  return p_world_ != nullptr;
}
bool init_ue4_project::set_level_info(int32 in_start, int32 in_end) {
  check(p_level_);
  start_frame = in_start;
  end_frame   = in_end;

  auto l_eve  = CastChecked<ULevelSequence>(p_level_);
  if (l_eve->GetMovieScene()->GetCameraCutTrack() != nullptr)
    return true;

  l_eve->GetMovieScene()->SetDisplayRate(FFrameRate{25, 1});
  l_eve->GetMovieScene()->SetTickResolutionDirectly(FFrameRate{25, 1});
  l_eve->GetMovieScene()->Modify();

  /// 设置范围
  l_eve->GetMovieScene()->SetWorkingRange((in_start - 10) / 25, (in_end + 10) / 25);
  l_eve->GetMovieScene()->SetViewRange((in_start - 10) / 25, (in_end + 10) / 25);
  l_eve->GetMovieScene()->SetPlaybackRange(
      TRange<FFrameNumber>{in_start, in_end}, true
  );
  l_eve->Modify();
  // l_eve->GetMovieScene()->FindMasterTrack<>()

  ACineCameraActor *l_cam = GWorld->SpawnActor<ACineCameraActor>();
  // GEditor->Bluep;
  // StaticLoadObject

  /**
   * @brief 使用ue4反射添加相机轨道什么的
   *
   */
  UDoodleImportUilt::Get()->create_camera(
      l_eve,
      l_cam
  );

  // 设置相机属性
  l_cam->GetCineCameraComponent()->Filmback.SensorHeight = 20.25;
  l_cam->GetCineCameraComponent()->Filmback.SensorWidth  = 36.0;
  l_cam->GetCineCameraComponent()->FocusSettings.FocusMethod =
      ECameraFocusMethod::Disable;

  for (auto &&i : l_eve->MovieScene->GetAllSections()) {
    i->SetStartFrame(TRangeBound<FFrameNumber>{});
    i->SetEndFrame(TRangeBound<FFrameNumber>{});
    i->SetRange(TRange<FFrameNumber>{in_start, in_end});
    i->Modify();
  }
  // l_cam->ConditionalBeginDestroy();
  return true;
}
bool init_ue4_project::save() {
  UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
  return true;
}
void init_ue4_project::tmp() {
  init_ue4_project{}.import_ass_data(R"(E:\Users\TD\Documents\Unreal_Projects\doodle_plug_dev_4.27\test_file\doodle_import_data_main.json)");
  init_ue4_project{}.import_ass_data(R"(E:\Users\TD\Documents\Unreal_Projects\doodle_plug_dev_4.27\test_file\doodle_import_data_main2.json)");
  // save();

  /// test2
  // load_all_blueprint();
  // build_all_blueprint();
  // create_world(TEXT("/Game/tmp/1/world_"));
  // create_level(TEXT("/Game/tmp/1/level_"));
  // set_level_info(1001, 1096);
  // auto l_load_sk = LoadObject<UAnimSequence>(p_level_, TEXT("/Game/tmp/1/ch_/RJ_EP029_SC008_AN_Ch001D_Rig_LX_1001-1096"));
  // auto l_load_geo = LoadObject<UGeometryCache>(p_level_, TEXT("/Game/tmp/1/ch_/RJ_EP053_SC056_AN_Ch115A_Rig_jfm_1000-1119"));
  // TArray<UAnimSequence *> l_load_sks{};
  // l_load_sks.Add(l_load_sk);
  // obj_add_level(l_load_sks);
  // TArray<UGeometryCache *> l_load_geos{};
  // l_load_geos.Add(l_load_geo);
  // obj_add_level(l_load_geos);
  // camera_fbx_to_level(TEXT("D:/Autodesk/RJ_EP029_SC008_AN_camera_1001-1096.fbx"));
}

bool init_ue4_project::import_ass_data(const FString &in_path) {
  /// 加载蓝图
  load_all_blueprint();
  build_all_blueprint();

  if (!FPaths::FileExists(in_path))
    return false;

  TArray<UAssetImportTask *> ImportDataList{};

  {  /// 解码导入的json数据
    TArray<FDoodleAssetImportData> import_setting_list;
    FString k_json_str;
    FDoodleAssetImportDataGroup l_data_list{};
    if (FFileHelper::LoadFileToString(k_json_str, *in_path)) {
      UE_LOG(LogTemp, Log, TEXT("开始读取json配置文件"));
      UE_LOG(LogTemp, Log, TEXT("开始测试 数组"));
      if (FJsonObjectConverter::JsonObjectStringToUStruct<
              FDoodleAssetImportDataGroup>(k_json_str, &l_data_list, CPF_None, CPF_None)) {
        import_setting_list = l_data_list.groups;
      }
    }
    UE_LOG(LogTemp, Log, TEXT("开始直接读取字符串作为json"));
    if (FJsonObjectConverter::JsonObjectStringToUStruct<
            FDoodleAssetImportDataGroup>(in_path, &l_data_list, CPF_None, CPF_None)) {
      import_setting_list = l_data_list.groups;
    }

    if (l_data_list.groups.Num() == 0) {
      return false;
    }

    create_world(l_data_list.world_path);
    create_level(l_data_list.level_path);
    set_level_info(l_data_list.start_frame, l_data_list.end_frame);

    for (auto &i : import_setting_list) {
      UE_LOG(LogTemp, Log, TEXT("开始开始创建导入配置"));
      switch (i.import_type) {
        case EDoodleImportType::abc:
        case EDoodleImportType::fbx:
          ImportDataList.Add(i.get_input(GetTransientPackage()));
          break;
        case EDoodleImportType::camera:
          camera_fbx_to_level(i.import_file_path);
          break;
        default:
          break;
      }
    }

    UE_LOG(LogTemp, Log, TEXT("开始导入文件"));
    FAssetToolsModule &AssetToolsModule =
        FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<FString> import_Paths{};
    AssetToolsModule.Get().ImportAssetTasks(ImportDataList);
    for (auto &ImportData : ImportDataList) {
      if (ImportData->Result.Num() > 0 || ImportData->ImportedObjectPaths.Num() > 0) {
        for (FString &i : ImportData->ImportedObjectPaths) {
          FString pr{i};
          import_Paths.AddUnique(i);
          UE_LOG(LogTemp, Log, TEXT("导入完成 %s"), *pr);
        }
      } else {
        UE_LOG(LogTemp, Error, TEXT("导入失败 %s"), *ImportData->Filename);
      }
    }
    UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

    TArray<UObject *> import_obj{};
    for (FString &i : import_Paths) {
      auto l_load = LoadObject<UObject>(p_level_, *i);
      if (l_load != nullptr)
        import_obj.Add(l_load);
    }

    TArray<UGeometryCache *> l_geo = this->filter_by_type<UGeometryCache>(import_obj);
    TArray<UAnimSequence *> l_anim = this->filter_by_type<UAnimSequence>(import_obj);
    this->obj_add_level(l_geo);
    this->obj_add_level(l_anim);
    this->save();
  }
  return true;
}

bool init_ue4_project::obj_add_level(const TArray<UGeometryCache *> in_obj) {
  check(p_level_);

  auto *l_eve  = CastChecked<ULevelSequence>(p_level_);
  auto *l_tool = UDoodleImportUilt::Get();
  for (auto &i : in_obj) {
    AGeometryCacheActor *l_actor   = GWorld->SpawnActor<AGeometryCacheActor>();
    UGeometryCacheComponent *l_com = l_actor->GetGeometryCacheComponent();
    l_com->SetGeometryCache(i);
    auto l_task = l_tool->add_geo_cache_scene(l_eve, l_actor);
    if (l_task) {
      l_task->SetStartFrame(TRangeBound<FFrameNumber>{});
      l_task->SetEndFrame(TRangeBound<FFrameNumber>{});
      l_task->SetRange(TRange<FFrameNumber>{(int32)start_frame, (int32)end_frame});
      l_task->Modify();
    }
    l_actor->ConditionalBeginDestroy();
  }

  return false;
}
bool init_ue4_project::obj_add_level(const TArray<UAnimSequence *> in_obj) {
  check(p_level_);

  auto *l_eve  = CastChecked<ULevelSequence>(p_level_);
  auto *l_tool = UDoodleImportUilt::Get();
  for (auto &i : in_obj) {
    ASkeletalMeshActor *l_actor   = GWorld->SpawnActor<ASkeletalMeshActor>();
    USkeletalMeshComponent *l_com = l_actor->GetSkeletalMeshComponent();
    USkeleton *l_sk               = i->GetSkeleton();
    auto l_sk_mesh_path           = l_sk->GetPathName().Replace(TEXT("_Skeleton"), TEXT(""));
    auto l_sk_mesh                = LoadObject<USkeletalMesh>(i, *l_sk_mesh_path);
    if (l_sk_mesh) {
      l_com->SetSkeletalMesh(l_sk_mesh);
      l_com->SetAnimationMode(EAnimationMode::AnimationSingleNode);
      l_com->SetAnimation(i);
      auto l_task = l_tool->add_skin_scene(l_eve, l_actor, i);
      if (l_task) {
        l_task->SetStartFrame(TRangeBound<FFrameNumber>{});
        l_task->SetEndFrame(TRangeBound<FFrameNumber>{});
        l_task->SetRange(TRange<FFrameNumber>{(int32)start_frame, (int32)end_frame});
        l_task->Modify();
      }
    }
    l_actor->ConditionalBeginDestroy();
  }
  return false;
}

bool init_ue4_project::has_obj(const UObject *in_obj) {
  check(p_level_);

  auto *l_eve  = CastChecked<ULevelSequence>(p_level_);
  auto *l_move = l_eve->GetMovieScene();
  for (auto i = 0; i < l_move->GetSpawnableCount(); i++) {
    FMovieSceneSpawnable &l_f = l_move->GetSpawnable(i);
    if (l_f.GetObjectTemplate()->GetPathName() == in_obj->GetPathName()) {
      return true;
    }
  }
  return false;
}

bool init_ue4_project::camera_fbx_to_level(const FString &in_fbx_path) {
  check(p_level_);

  auto *l_eve  = CastChecked<ULevelSequence>(p_level_);
  auto *l_move = l_eve->GetMovieScene();
  FGuid l_cam_guid{};
  for (auto i = 0; i < l_move->GetSpawnableCount(); i++) {
    if (l_move->GetSpawnable(i).GetObjectTemplate()->GetClass()->IsChildOf(ACameraActor::StaticClass())) {
      l_cam_guid = l_move->GetSpawnable(i).GetGuid();
      break;
    }
  }

  for (auto i = 0; i < l_move->GetPossessableCount(); i++) {
    if (l_move->GetPossessable(i).GetPossessedObjectClass()->IsChildOf(ACameraActor::StaticClass())) {
      l_cam_guid = l_move->GetPossessable(i).GetGuid();
      break;
    }
  }

  if (l_cam_guid.IsValid()) {
    auto *l_setting                   = NewObject<UMovieSceneUserImportFBXSettings>(p_world_);
    l_setting->bMatchByNameOnly       = false;
    l_setting->bForceFrontXAxis       = false;
    l_setting->bConvertSceneUnit      = true;
    l_setting->ImportUniformScale     = 1;
    l_setting->bCreateCameras         = false;
    l_setting->bReplaceTransformTrack = false;
    l_setting->bReduceKeys            = false;
    l_setting->ReduceKeysTolerance    = 0;

    UDoodleImportUiltEditor::Get()->add_camera_fbx_scene(
        Cast<UWorld>(p_world_),
        l_eve,
        l_setting,
        in_fbx_path,
        l_cam_guid
    );
  }

  return true;
  // UnFbx::FFbxImporter *FbxImporter = UnFbx::FFbxImporter::GetInstance();
}
}  // namespace doodle
