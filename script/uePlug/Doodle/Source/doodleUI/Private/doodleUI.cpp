#include "DoodleUI.h"

#include "Doodle/AiArrayGeneration.h"
#include "Doodle/AiArrayGenerationMove.h"
#include "Doodle/AiArrayGenerationMoveSpline.h"
#include "Doodle/DoodleTimeDilationActor.h"
#include "DoodleAiSplineCrowd.h"
#include "DoodleCopySpline.h"
#include "DoodleDirectionalLightDome.h"
#include "DoodleMatrixLight.h"
#include "DoodleSurroundMesh.h"
#include "Editor/UnrealEdEngine.h"
#include "IPlacementModeModule.h"
#include "UnrealEdGlobals.h"
#include "fireLight.h"

static const FName doodleTabName("doodleUI");
#define LOCTEXT_NAMESPACE "FdoodleUIModule"

void FdoodleUIModule::StartupModule() {
  if (GUnrealEd) {
    Map_Lists               = GUnrealEd->GetProjectDefaultMapTemplates();
    FTemplateMapInfo& l_ref = Map_Lists.Emplace_GetRef();
    l_ref.Map               = TEXT("/Doodle/lock_dev/main_lock_dev.main_lock_dev");
    GUnrealEd->OnGetTemplateMapInfos().BindLambda([this]() -> const TArray<FTemplateMapInfo>& { return Map_Lists; });
  }
  // 在我们这里添加自定义放置类
  FPlacementCategoryInfo info{LOCTEXT("doodle", "doodle"), "DoodleCategoryInfo", TEXT("Adoodle"), 55, true};
  IPlacementModeModule::Get().RegisterPlacementCategory(info);
  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle, MakeShareable(new FPlaceableItem(
                             nullptr, FAssetData{AfireLight::StaticClass()}
                         ))
  );

  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleDirectionalLightDome::StaticClass()}
      ))
  );

  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleCopySpline::StaticClass()}
      ))
  );
  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleMatrixLight::StaticClass()}
      ))
  );
  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleSurroundMeshActor::StaticClass()}
      ))
  );

  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleAiSplineCrowd::StaticClass()}
      ))
  );

  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleAiArrayGeneration::StaticClass()}
      ))
  );
  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleAiArrayGenerationMove::StaticClass()}
      ))
  );
  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle,
      MakeShareable(new FPlaceableItem(
          nullptr, FAssetData{ADoodleAiArrayGenerationMoveSpline::StaticClass()}
      ))
  );
  IPlacementModeModule::Get().RegisterPlaceableItem(
      info.UniqueHandle, MakeShareable(new FPlaceableItem(nullptr, FAssetData{ADoodleTimeDilation::StaticClass()}))
  );
}

void FdoodleUIModule::ShutdownModule() {
  // 我们的卸载函数
  if (IPlacementModeModule::IsAvailable()) {
    IPlacementModeModule::Get().UnregisterPlacementCategory(
        "DoodleCategoryInfo"
    );
  }
}

IMPLEMENT_MODULE(FdoodleUIModule, doodleUI)
