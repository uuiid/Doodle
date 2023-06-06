#include "DoodleEditor.h"

#include "Doodle/ContentBrowserMenuExtension.h"
#include "Doodle/DoodleImportFbxUI.h"
#include "DoodleCommands.h"
#include "DoodleStyle.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "doodleCopyMaterial.h"

#include "ContentBrowserModule.h"  ///内容游览器
#include "Doodle/CreateCharacter/CreateCharacterMianUI.h"
#include "Doodle/CreateCharacter/CoreData/CreateCharacterActor.h"
#include "Doodle/CreateCharacter/Editor/CreateCharacterActor_Customization.h"
#include "AssetToolsModule.h"  // 注册资产动作
#include "Doodle/CreateCharacter/Editor/CreateCharacter_AssetTypeActions.h"

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
      FExecuteAction::CreateRaw(this, &FdoodleEditorModule::PluginButtonClicked), FCanExecuteAction()
  );
  PluginCommands->MapAction(
      FDoodleCommands::Get().DoodleImportFbxWindow,
      FExecuteAction::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(SDoodleImportFbxUI::Name); }),
      FCanExecuteAction()
  );

  /// @brief 注册回调(在这里出现在工具菜单中)
  UToolMenus::RegisterStartupCallback(
      FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FdoodleEditorModule::RegisterMenus)
  );

  /// @brief 注册tab
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(doodleTabName, FOnSpawnTab::CreateRaw(this, &FdoodleEditorModule::OnSpawnPluginTab))
      .SetDisplayName(LOCTEXT("FdoodleTabTitle1", "Doodle"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);
  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(SDoodleImportFbxUI::Name, FOnSpawnTab::CreateStatic(&SDoodleImportFbxUI::OnSpawnAction))
      .SetDisplayName(LOCTEXT("FdoodleTabTitle2", "Doodle Import Fbx"))
      .SetMenuType(ETabSpawnerMenuType::Hidden);

  FContentBrowserModule &ContentBrowserModule =
      FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
  TArray<FContentBrowserMenuExtender_SelectedAssets> &MenuExtenderDelegates =
      ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
  MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
      [this](const TArray<FAssetData> &Path) -> TSharedRef<FExtender> {
        // 创建一个扩展并保存数据
        Extension                          = MakeShareable(new FContentBrowserMenuExtension(Path));
        // 创建一个扩展 创建包含委托的扩展程序，该委托将被调用以获取有关新上下文菜单项的信息
        TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
        // 创建一个共享指针委托，该委托保留对对象“NewFolder”的弱引用。这是一个钩子名称，由扩展器用来标识将扩展路径上下文菜单的外部对象
        MenuExtender->AddMenuExtension(
            "GetAssetActions", EExtensionHook::After, TSharedPtr<FUICommandList>(),
            FMenuExtensionDelegate::CreateSP(Extension.ToSharedRef(), &FContentBrowserMenuExtension::AddMenuEntry)
        );
        return MenuExtender.ToSharedRef();
      }
  ));

  // 注册资产类别
  IAssetTools &L_AssetTools         = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
  EAssetTypeCategories::Type L_Type = L_AssetTools.RegisterAdvancedAssetCategory(
      FName{TEXT("Doodle Character")}, LOCTEXT("Doodle Character", "Doodle Character")
  );

  // 注册资产动作
  L_AssetTools.RegisterAssetTypeActions(
      CreateAssetActions.Add_GetRef(MakeShared<FAssetTypeActions_CreateCharacter>(L_Type)).ToSharedRef()
  );

  // 注册详细面板
  FPropertyEditorModule &L_Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
  L_Module.RegisterCustomClassLayout(ADoodleCreateCharacterActor::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&CreateCharacterActor_Customization::MakeInstance));
  L_Module.NotifyCustomizationModuleChanged();

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

  // 取消注册资产动作
  if (FModuleManager::Get().IsModuleLoaded("AssetTools")) {
    IAssetTools &L_AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    for (auto &&i : CreateAssetActions)
      L_AssetTools.UnregisterAssetTypeActions(
          i.ToSharedRef()
      );
  }
  CreateAssetActions.Empty();
  // 详细面板
  if (FModuleManager::Get().IsModuleLoaded("PropertyEditor")) {
    FPropertyEditorModule &L_Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    L_Module.UnregisterCustomClassLayout(ADoodleCreateCharacterActor::StaticClass()->GetFName());
    L_Module.NotifyCustomizationModuleChanged();
  }
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
      Section.AddMenuEntryWithCommandList(FDoodleCommands::Get().OpenPluginWindow, PluginCommands);
      Section.AddMenuEntryWithCommandList(FDoodleCommands::Get().DoodleImportFbxWindow, PluginCommands);
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
