#include "DoodleUI.h"

#include "DoodleConfigLightActor.h"
#include "DoodleCopySpline.h"
#include "DoodleDirectionalLightDome.h"
#include "IPlacementModeModule.h"
#include "DoodleMatrixLight.h"
#include "fireLight.h"
#include "DoodleSurroundMesh.h"

static const FName doodleTabName("doodleUI");
#define LOCTEXT_NAMESPACE "FdoodleUIModule"

void FdoodleUIModule::StartupModule() {
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
          nullptr, FAssetData{ADoodleConfigLightActor::StaticClass()}
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
