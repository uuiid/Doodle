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

namespace doodle {
bool init_ue4_project::create_world(const FString& in_path,
                                    const FString& in_name) {
  auto& l_ass_tool = FModuleManager::Get()
                         .LoadModuleChecked<FAssetToolsModule>("AssetTools")
                         .Get();

  // UFactory::StaticClass()->GetDefaultSubobjects()
  for (TObjectIterator<UClass> it{}; it; ++it) {
    if (it->IsChildOf(UFactory::StaticClass())) {
      if (it->GetName() == "ULevelSequenceFactoryNew") {
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
  l_eve->MovieScene->SetDisplayRate(FFrameRate{25, 1});
  ///设置范围
  l_eve->MovieScene->SetPlaybackRange(TRange<FFrameNumber>{in_start, in_end},
                                      true);
  l_eve->MovieScene->SetWorkingRange(in_start - 10, in_end + 10);
  l_eve->MovieScene->SetViewRange(in_start - 10, in_end + 10);

  return false;
}
void init_ue4_project::tmp() {
  create_world(TEXT("/Game/tmp/test"), TEXT("doodle_test_word"));
  create_level(TEXT("/Game/tmp/test"), TEXT("doodle_test_level"));
  set_level_info(1001, 1200);
}
}  // namespace doodle
