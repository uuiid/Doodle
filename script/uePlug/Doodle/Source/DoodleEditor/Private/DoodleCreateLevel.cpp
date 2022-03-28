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
/// json需要
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
/// 几何缓存体使用
#include "GeometryCache.h"

namespace doodle
{
  bool init_ue4_project::load_all_blueprint()
  {
    UE_LOG(LogTemp, Log, TEXT("Loading Asset Registry..."));
    FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
    AssetRegistryModule.Get().SearchAllAssets(/*bSynchronousSearch =*/true);
    UE_LOG(LogTemp, Log, TEXT("Finished Loading Asset Registry."));

    UE_LOG(LogTemp, Log, TEXT("Gathering All Blueprints From Asset Registry..."));

    return AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(),
                                                      blueprint_list, true);
  }

  bool init_ue4_project::build_all_blueprint()
  {

    for (auto &&i : blueprint_list)
    {
      FString const AssetPath = i.ObjectPath.ToString();
      UE_LOG(LogTemp, Log, TEXT("Loading and Compiling: '%s'..."), *AssetPath);

      UBlueprint *l_b =
          Cast<UBlueprint>(
              StaticLoadObject(
                  i.GetClass(),
                  nullptr,
                  *AssetPath,
                  nullptr,
                  LOAD_NoWarn | LOAD_DisableCompileOnLoad));

      if (l_b != nullptr)
      {
        if (l_b->ParentClass.Get() == UDoodleImportUilt::StaticClass())
          FKismetEditorUtilities::CompileBlueprint(l_b,
                                                   EBlueprintCompileOptions::SkipGarbageCollection);
      }
    }
    return true;
  }

  bool init_ue4_project::create_level(const FString &in_path)
  {
    p_save_level_path = in_path;
    auto &l_ass_tool = FModuleManager::Get()
                           .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                           .Get();
    UE_LOG(LogTemp, Log, TEXT("关卡路径 %s"), *(FPaths::GetPath(in_path) / FPaths::GetBaseFilename(in_path)));
    if (!FPackageName::DoesPackageExist(in_path))
    {
      for (TObjectIterator<UClass> it{}; it; ++it)
      {
        if (it->IsChildOf(UFactory::StaticClass()))
        {
          if (it->GetName() == "LevelSequenceFactoryNew")
          {
            p_level_ = l_ass_tool.CreateAsset(FPaths::GetBaseFilename(in_path), FPaths::GetPath(in_path),
                                              ULevelSequence::StaticClass(),
                                              it->GetDefaultObject<UFactory>());
          }
        }
      }
    }
    else
    {
      p_level_ = LoadObject<ULevelSequence>(nullptr, *in_path);
    }

    if (p_level_ != nullptr)
      p_save_level_path = p_level_->GetPathName();

    return p_level_ != nullptr;
  }
  bool init_ue4_project::create_world(const FString &in_path)
  {
    p_save_world_path = in_path;
    auto &l_ass_tool = FModuleManager::Get()
                           .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                           .Get();
    UE_LOG(LogTemp, Log, TEXT("世界路径 %s"), *(FPaths::GetPath(in_path) / FPaths::GetBaseFilename(in_path)));

    if (!FPackageName::DoesPackageExist(in_path))
    {
      p_world_ = l_ass_tool.CreateAsset(
          FPaths::GetBaseFilename(in_path), FPaths::GetPath(in_path), UWorld::StaticClass(),
          UWorldFactory::StaticClass()->GetDefaultObject<UFactory>());
      UEditorLevelLibrary::LoadLevel(in_path);
    }
    else
    {
      UEditorLevelLibrary::LoadLevel(in_path);
      p_world_ = GWorld;
    }
    if (p_world_ != nullptr)
      p_save_world_path = p_world_->GetPathName();

    return p_world_ != nullptr;
  }
  bool init_ue4_project::set_level_info(int32 in_start, int32 in_end)
  {
    check(p_level_);

    auto l_eve = CastChecked<ULevelSequence>(p_level_);
    if (l_eve->GetMovieScene()->GetCameraCutTrack() != nullptr)
      return true;

    l_eve->GetMovieScene()->SetDisplayRate(FFrameRate{25, 1});
    l_eve->GetMovieScene()->SetTickResolutionDirectly(FFrameRate{25, 1});
    l_eve->GetMovieScene()->Modify();

    ///设置范围
    l_eve->GetMovieScene()->SetWorkingRange((in_start - 10) / 25,
                                            (in_end + 10) / 25);
    l_eve->GetMovieScene()->SetViewRange((in_start - 10) / 25,
                                         (in_end + 10) / 25);
    l_eve->GetMovieScene()->SetPlaybackRange(
        TRange<FFrameNumber>{in_start, in_end}, true);
    l_eve->Modify();

    ACineCameraActor *l_cam = GWorld->SpawnActor<ACineCameraActor>();
    // GEditor->Bluep;
    // StaticLoadObject

    /**
     * @brief 使用ue4反射添加相机轨道什么的
     *
     */
    UDoodleImportUilt::Get()->create_camera(
        l_eve,
        l_cam);

    // 设置相机属性
    l_cam->GetCineCameraComponent()->Filmback.SensorHeight = 20.25;
    l_cam->GetCineCameraComponent()->Filmback.SensorWidth = 36.0;
    l_cam->GetCineCameraComponent()->FocusSettings.FocusMethod =
        ECameraFocusMethod::Disable;

    for (auto &&i : l_eve->MovieScene->GetAllSections())
    {
      i->SetStartFrame(TRangeBound<FFrameNumber>{});
      i->SetEndFrame(TRangeBound<FFrameNumber>{});
      i->SetRange(TRange<FFrameNumber>{in_start, in_end});
      i->Modify();
    }

    return true;
  }
  bool init_ue4_project::save()
  {

    UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
    return true;
  }
  void init_ue4_project::tmp()
  {
    import_ass_data(R"(E:\Users\TD\Documents\Unreal_Projects\doodle_plug_dev_4.27\test_file\doodle_import_data_main.json)",
                    GetTransientPackage());
    save();
  }

  bool init_ue4_project::import_ass_data(const FString &in_path, UObject *Outer)
  {
    /// 加载蓝图
    load_all_blueprint();
    build_all_blueprint();

    if (!FPaths::FileExists(in_path))
      return false;

    TArray<UAssetImportTask *> ImportDataList{};

    { /// 解码导入的json数据
      TArray<FDoodleAssetImportData> import_setting_list;
      FString k_json_str;
      FDoodleAssetImportDataGroup l_data_list{};
      if (FFileHelper::LoadFileToString(k_json_str, *in_path))
      {
        UE_LOG(LogTemp, Log, TEXT("开始读取json配置文件"));
        UE_LOG(LogTemp, Log, TEXT("开始测试 数组"));
        if (FJsonObjectConverter::JsonObjectStringToUStruct<
                FDoodleAssetImportDataGroup>(k_json_str, &l_data_list, CPF_None,
                                             CPF_None))
        {
          import_setting_list = l_data_list.groups;
        }
      }
      UE_LOG(LogTemp, Log, TEXT("开始直接读取字符串作为json"));
      if (FJsonObjectConverter::JsonObjectStringToUStruct<
              FDoodleAssetImportDataGroup>(in_path, &l_data_list, CPF_None,
                                           CPF_None))
      {
        import_setting_list = l_data_list.groups;
      }

      if (l_data_list.groups.Num() == 0)
      {
        return false;
      }

      create_world(l_data_list.world_path);
      create_level(l_data_list.level_path);
      set_level_info(l_data_list.start_frame, l_data_list.end_frame);

      for (auto &i : import_setting_list)
      {
        UE_LOG(LogTemp, Log, TEXT("开始开始创建导入配置"));
        switch (i.import_type)
        {
        case EDoodleImportType::Abc:
        case EDoodleImportType::Fbx:
          ImportDataList.Add(i.get_input(Outer));
          break;
        case EDoodleImportType::Camera:
        default:
          break;
        }
      }

      UE_LOG(LogTemp, Log, TEXT("开始导入文件"));
      FAssetToolsModule &AssetToolsModule =
          FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
      TArray<FString> import_Paths{};
      AssetToolsModule.Get().ImportAssetTasks(ImportDataList);
      UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
      for (auto &ImportData : ImportDataList)
      {

        if (ImportData->Result.Num() > 0 || ImportData->ImportedObjectPaths.Num() > 0)
        {
          for (FString &i : ImportData->ImportedObjectPaths)
          {
            FString pr{i};
            import_Paths.AddUnique(i);
            UE_LOG(LogTemp, Log, TEXT("导入完成 %s"), *pr);
          }
        }
        else
        {
          UE_LOG(LogTemp, Error, TEXT("导入失败 %s"), *ImportData->Filename);
        }
      }
      TArray<UObject *> import_obj{};
      for (FString &i : import_Paths)
      {
        auto l_load = LoadObject<UObject>(p_level_, *i);
        import_obj.Add(l_load);
      }

      TArray<UGeometryCache *> l_geo = this->filter_by_type<UGeometryCache>(import_obj_list);
      TArray<USkeletalMesh *> l_skin = this->filter_by_type<USkeletalMesh>(import_obj_list);

      // ASkeletalMeshActor *l_actor = GWorld->SpawnActor<ASkeletalMeshActor>();
      // l_actor->GetSkeletalMeshComponent()->SetSkeletalMesh()
    }
    return true;
  }
} // namespace doodle
