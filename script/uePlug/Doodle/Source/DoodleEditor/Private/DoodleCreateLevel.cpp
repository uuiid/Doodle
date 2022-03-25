#include "DoodleCreateLevel.h"

/**
 * 创建world
 */
#include "AssetToolsModule.h"
#include "EditorLevelLibrary.h"
#include "Factories/WorldFactory.h"
#include "IAssetTools.h"
#include "LevelSequence.h"
#include "Modules/ModuleManager.h"
/**
 * 定序器使用
 */
#include "MovieSceneToolHelpers.h"
#include "SequencerSettings.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Misc/OutputDeviceNull.h"

/**
 * 保存操作使用
 *
 */
#include "FileHelpers.h"
/**
 * 相机导入
 */
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "MovieSceneObjectBindingID.h"

namespace doodle
{
  bool init_ue4_project::create_world(const FString &in_path,
                                      const FString &in_name)
  {
    auto &l_ass_tool = FModuleManager::Get()
                           .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                           .Get();

    // UFactory::StaticClass()->GetDefaultSubobjects()
    for (TObjectIterator<UClass> it{}; it; ++it)
    {
      if (it->IsChildOf(UFactory::StaticClass()))
      {
        if (it->GetName() == "LevelSequenceFactoryNew")
        {
          p_level_ = l_ass_tool.CreateAsset(in_name, in_path,
                                            ULevelSequence::StaticClass(),
                                            it->GetDefaultObject<UFactory>());
        }
      }
    }

    return p_level_ != nullptr;
  }
  bool init_ue4_project::create_level(const FString &in_path,
                                      const FString &in_name)
  {
    auto &l_ass_tool = FModuleManager::Get()
                           .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                           .Get();

    p_world_ = l_ass_tool.CreateAsset(
        in_name, in_path, UWorld::StaticClass(),
        UWorldFactory::StaticClass()->GetDefaultObject<UFactory>());
    UEditorLevelLibrary::LoadLevel(in_path / in_name);
    return p_world_ != nullptr;
  }
  bool init_ue4_project::set_level_info(int32 in_start, int32 in_end)
  {
    check(p_level_);

    auto l_eve = CastChecked<ULevelSequence>(p_level_);
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

    /// 绑定轨道
    // ACineCameraActor *l_cam = GWorld->SpawnActor<ACineCameraActor>();
    // l_eve->GetMovieScene()->AddMasterTrack();

    // l_eve->GetMovieScene()->AddTrack();

    // FGuid l_cam_poss_Guid =
    //     l_eve->MovieScene->AddPossessable(l_cam->GetName(), l_cam->GetClass());
    // l_eve->BindPossessableObject(l_cam_poss_Guid, *l_cam, p_world_);

    // // 添加相机绑定
    // // 创建相机轨道
    // MovieSceneToolHelpers::CreateCameraCutSectionForCamera(
    //     l_eve->MovieScene, l_cam_poss_Guid, FFrameNumber{in_end - in_start});

    // { /// 添加子组件
    //   UCineCameraComponent *l_cam_com = l_cam->GetCineCameraComponent();
    //   FGuid l_guid = l_eve->MovieScene->AddPossessable(l_cam_com->GetName(), l_cam_com->GetClass());
    //   l_eve->MovieScene->FindPossessable(l_guid)->SetParent(l_cam_poss_Guid);
    //   l_eve->BindPossessableObject(l_guid, *l_cam_com, l_cam);
    // }
    // { /// 添加当前焦距

    // }

    // 设置相机属性
    // l_cam->GetCineCameraComponent()->Filmback.SensorHeight = 20.25;
    // l_cam->GetCineCameraComponent()->Filmback.SensorWidth = 36.0;
    // l_cam->GetCineCameraComponent()->FocusSettings.FocusMethod =
    //     ECameraFocusMethod::Disable;

    // auto l_cam_task = l_eve->MovieScene->GetCameraCutTrack();

    // for (auto &&i : l_cam_task->GetAllSections())
    // {
    //   i->SetRange(TRange<FFrameNumber>{in_start, in_end});
    //   i->Modify();
    // }

    return false;
  }
  bool init_ue4_project::save()
  {
    // UEditorLoadingAndSavingUtils::SaveMap(CastChecked<UWorld>(p_world_),
    //                                      p_save_world_path);
    UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

    return false;
  }
  void init_ue4_project::tmp()
  {
    // UE_LOG(LogTemp, Log, TEXT("工厂名称 %s"), *(it->GetName()));

    create_world(TEXT("/Game/tmp/test"), TEXT("doodle_test_word"));
    create_level(TEXT("/Game/tmp/test"), TEXT("doodle_test_level"));
    set_level_info(1001, 1200);
    save();
  }
} // namespace doodle
