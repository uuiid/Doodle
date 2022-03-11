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

namespace doodle {
bool init_ue4_project::create_world(const FString& in_path,
                                    const FString& in_name) {
  auto& l_ass_tool = FModuleManager::Get()
                         .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                         .Get();

  // UFactory::StaticClass()->GetDefaultSubobjects()
  for (TObjectIterator<UClass> it{}; it; ++it) {
    if (it->IsChildOf(UFactory::StaticClass())) {
      if (it->GetName() == "LevelSequenceFactoryNew") {
        p_level_ = l_ass_tool.CreateAsset(in_name, in_path,
                                          ULevelSequence::StaticClass(),
                                          it->GetDefaultObject<UFactory>());
      }
    }
  }

  return p_level_ != nullptr;
}
bool init_ue4_project::create_level(const FString& in_path,
                                    const FString& in_name) {
  auto& l_ass_tool = FModuleManager::Get()
                         .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                         .Get();

  p_world_ = l_ass_tool.CreateAsset(
      in_name, in_path, UWorld::StaticClass(),
      UWorldFactory::StaticClass()->GetDefaultObject<UFactory>());
  UEditorLevelLibrary::LoadLevel(in_path / in_name);
  return p_world_ != nullptr;
}
bool init_ue4_project::set_level_info(int32 in_start, int32 in_end) {
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

  // l_eve->GetMovieScene()->SetSelectionRange(
  //    TRange<FFrameNumber>{in_start, in_end});

  // FActorSpawnParameters SpawnParams;
  // SpawnParams.ObjectFlags &= ~RF_Transactional;
  // FString NewName = MovieSceneHelpers::MakeUniqueSpawnableName(
  //    l_eve->MovieScene,
  //    FName::NameToDisplayString(
  //        ACineCameraActor::StaticClass()->GetFName().ToString(), false));

  // l_eve->GetMovieScene()->AddSpawnable();
  /// 绑定轨道
  ACineCameraActor* l_cam = GWorld->SpawnActor<ACineCameraActor>();
  FGuid l_cam_poss_Guid =
      l_eve->MovieScene->AddPossessable(l_cam->GetName(), l_cam->GetClass());
  l_eve->BindPossessableObject(l_cam_poss_Guid, *l_cam, p_world_);
  // FGuid l_cam_poss_Guid =
  //    l_eve->MovieScene->AddSpawnable(l_cam->GetName(), *l_cam);
  // l_eve->BindPossessableObject(l_cam_poss_Guid, *l_cam, p_world_);
  // 添加相机绑定
  // 创建相机轨道
  MovieSceneToolHelpers::CreateCameraCutSectionForCamera(
      l_eve->MovieScene, l_cam_poss_Guid, FFrameNumber{in_end - in_start});

  // auto l_cam_task = CastChecked<UMovieSceneCameraCutTrack>(
  //    l_eve->GetMovieScene()->GetCameraCutTrack());
  // l_cam_task->AddNewCameraCut();

  // 设置相机属性
  l_cam->GetCineCameraComponent()->Filmback.SensorHeight = 20.25;
  l_cam->GetCineCameraComponent()->Filmback.SensorWidth = 36.0;
  l_cam->GetCineCameraComponent()->FocusSettings.FocusMethod =
      ECameraFocusMethod::Disable;

  auto l_cam_task = l_eve->MovieScene->GetCameraCutTrack();
  for (auto&& i : l_cam_task->GetAllSections()) {
    i->SetRange(TRange<FFrameNumber>{in_start, in_end});
    // i->SetStartFrame(FFrameNumber{in_start});
    // i->SetEndFrame(FFrameNumber{in_start});
    i->Modify();
  }

  // for (TObjectIterator<UClass> it{}; it; ++it) {
  //  if (it->GetName() == "MovieSceneCameraCutTrack") {
  //    // CastChecked<UMovieSceneTrack>(*it)->StaticClass();
  //    l_eve->MovieScene->AddMasterTrack(*it);
  //  }
  //}
  // for (TObjectIterator<UClass> it{}; it; ++it) {
  //  if (it->IsChildOf(UMovieSceneSection::StaticClass())) {
  //    if (it->GetName() == "MovieSceneCameraCutSection") {
  //      l_eve->MovieScene->AddCameraCutTrack(
  //          it->GetDefaultObject<UMovieSceneSection>());
  //      auto l_move = it->GetDefaultObject<UMovieSceneSection>();
  //      auto l_move2 = DuplicateObject<UMovieSceneSection>(l_move,
  //      l_trak); l_trak->AddSection(*l_move2);
  //    }
  //  }
  //}

  return false;
}
bool init_ue4_project::save() {
  // UEditorLoadingAndSavingUtils::SaveMap(CastChecked<UWorld>(p_world_),
  //                                      p_save_world_path);
  UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);

  return false;
}
void init_ue4_project::tmp() {
  // UE_LOG(LogTemp, Log, TEXT("工厂名称 %s"), *(it->GetName()));

  create_world(TEXT("/Game/tmp/test"), TEXT("doodle_test_word"));
  create_level(TEXT("/Game/tmp/test"), TEXT("doodle_test_level"));
  set_level_info(1001, 1200);
  // save();
}
}  // namespace doodle
