#include "DoodleEditor.h"

#include "DoodleCommands.h"
#include "DoodleStyle.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "doodleCopyMaterial.h"
#include "Doodle/DoodleImportFbxUI.h"
// #include "fireLight.h"
// #include "DoodleDirectionalLightDome.h"
// #include "DoodleCopySpline.h"
// #include "ContentBrowserAssetDataSource.h"
// #include "IPlacementModeModule.h"
// #include "AssetRegistry/IAssetRegistry.h"
// #include "AssetRegistry/AssetRegistryModule.h"

static const FName doodleTabName("doodleEditor");
#define LOCTEXT_NAMESPACE "FdoodleEditorModule"

void FdoodleEditorModule::StartupModule() {
  FdoodleStyle::Initialize();
  FdoodleStyle::ReloadTextures();

  FDoodleCommands::Register();

  PluginCommands = MakeShareable(new FUICommandList);
  /// @brief 注册命令
  PluginCommands->MapAction(
      FDoodleCommands::Get().OpenPluginWindow,
      FExecuteAction::CreateRaw(this, &FdoodleEditorModule::PluginButtonClicked),
      FCanExecuteAction()
  );
  PluginCommands->MapAction(
      FDoodleCommands::Get().DoodleImportFbxWindow,
      FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(SDoodleImportFbxUI::Name); }),
      FCanExecuteAction()
  );
  /// @brief 注册回调(在这里出现在工具菜单中)
  UToolMenus::RegisterStartupCallback(
      FSimpleMulticastDelegate::FDelegate::CreateRaw(
          this, &FdoodleEditorModule::RegisterMenus
      )
  );

  /// @brief 注册tab
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(
          doodleTabName,
          FOnSpawnTab::CreateRaw(this, &FdoodleEditorModule::OnSpawnPluginTab)
      )
      .SetDisplayName(LOCTEXT("FdoodleTabTitle", "Doodle"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(
          SDoodleImportFbxUI::Name,
          FOnSpawnTab::CreateStatic(&SDoodleImportFbxUI::OnSpawnAction)
      )
      .SetDisplayName(LOCTEXT("FdoodleTabTitle", "Doodle Import Fbx"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);

  // AssetDataSource.Reset(NewObject<UContentBrowserAssetDataSource>(
  //    GetTransientPackage(), "doodle_AssetData"));
  // AssetDataSource->Initialize(R"(/doodle/test)");
  // auto AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
  //                          AssetRegistryConstants::ModuleName)
  //                          .Get();
  // AssetRegistry->AddPath(R"(/../../tmp2/Content/)");
}

void FdoodleEditorModule::ShutdownModule() {
  UToolMenus::UnRegisterStartupCallback(this);

  UToolMenus::UnregisterOwner(this);

  FdoodleStyle::Shutdown();

  FDoodleCommands::Unregister();

  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(doodleTabName);
  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SDoodleImportFbxUI::Name);
  // AssetDataSource.Reset();
}
TSharedRef<SDockTab> FdoodleEditorModule::OnSpawnPluginTab(
    const FSpawnTabArgs &SpawnTabArgs
) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[
      // Put your tab content here!
      SNew(SBox)
          .HAlign(HAlign_Left)
          .VAlign(VAlign_Top)[SNew(DoodleCopyMat)  // 这里创建我们自己的界面
  ]];
}

void FdoodleEditorModule::PluginButtonClicked() {
#if ENGINE_MINOR_VERSION == 27
  FGlobalTabmanager::Get()->TryInvokeTab(doodleTabName);
#else if ENGINE_MINOR_VERSION < 27
  FGlobalTabmanager::Get()->TryInvokeTab(doodleTabName);
#endif
}

void FdoodleEditorModule::RegisterMenus() {
  // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
  FToolMenuOwnerScoped OwnerScoped(this);

  {
    UToolMenu *Menu =
        UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    {
      FToolMenuSection &Section = Menu->FindOrAddSection("WindowLayout");
      Section.AddMenuEntryWithCommandList(
          FDoodleCommands::Get().OpenPluginWindow, PluginCommands
      );
      Section.AddMenuEntryWithCommandList(
          FDoodleCommands::Get().DoodleImportFbxWindow, PluginCommands
      );
    }
  }

  {
    UToolMenu *ToolbarMenu =
        UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
    {
      FToolMenuSection &Section = ToolbarMenu->FindOrAddSection("Settings");
      {
        FToolMenuEntry &Entry =
            Section.AddEntry(FToolMenuEntry::InitToolBarButton(
                FDoodleCommands::Get().OpenPluginWindow
            ));
        Entry.SetCommandList(PluginCommands);
      }
    }
  }
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FdoodleEditorModule, doodleEditor)
