// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PluginMenu.h"
#include "PluginMenuStyle.h"
#include "PluginMenuCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "MapGenerator/LoadSkeletonAnimationUI.h"
#include "MapGenerator/MutiSceneMapUI.h"
#include "MapGenerator/SingleSceneMapUI.h"
#include "MapGenerator/TestWindowUI.h"
#include "MapGenerator/LoadBPAnimationUI.h"
#include "MapGenerator/CreateCustomBPMapUI.h"
#include "MapGenerator/CreateBPMapUI.h"
#include "MapGenerator/CleanUpMapUI.h"
#include "MapGenerator/CleanUpFolderUI.h"
#include "MapGenerator/DataType.h"

static const FName Menu1_1UITabName("Menu1_1UI");
static const FName Menu1_2UITabName("Menu1_2UI");
static const FName Menu1_3UITabName("Menu1_3UI");
static const FName Menu1_4UITabName("Menu1_4UI");
static const FName Menu1_5UITabName("Menu1_5UI");
static const FName Menu1_6UITabName("Menu1_6UI");
static const FName Menu1_7UITabName("Menu1_7UI");

#ifdef LOCTEXT_NAMESPACE
#undef LOCTEXT_NAMESPACE
#endif

#define LOCTEXT_NAMESPACE "FPluginMenuModule"

void FPluginMenuModule::StartupModule() {
  // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

  FPluginMenuStyle::Initialize();
  FPluginMenuStyle::ReloadTextures();

  FPluginMenuCommands::Register();

  PluginCommands = MakeShareable(new FUICommandList);

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_1UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_1UITabName),
      FCanExecuteAction()
  );

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_2UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_2UITabName),
      FCanExecuteAction()
  );

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_3UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_3UITabName),
      FCanExecuteAction()
  );

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_4UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_4UITabName),
      FCanExecuteAction()
  );

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_5UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_5UITabName),
      FCanExecuteAction()
  );

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_6UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_6UITabName),
      FCanExecuteAction()
  );

  PluginCommands->MapAction(
      FPluginMenuCommands::Get().OpenMenu1_7UI,
      FExecuteAction::CreateRaw(this, &FPluginMenuModule::MenuEntryClickedHandle, Menu1_7UITabName),
      FCanExecuteAction()
  );

  FLevelEditorModule &LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

  MenuExtender                          = MakeShareable(new FExtender);
  // MenuExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FPluginMenuModule::AddToolbarExtension));
  MenuExtender->AddMenuBarExtension(
      "Help",
      EExtensionHook::After,
      PluginCommands,
      FMenuBarExtensionDelegate::CreateRaw(this, &FPluginMenuModule::AddPullDownMenu)
  );

  // LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(MyExtender);
  LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_1UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_1UITab)).SetDisplayName(FText::FromString("Load Skeleton Anim")).SetMenuType(ETabSpawnerMenuType::Hidden);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_2UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_2UITab)).SetDisplayName(FText::FromString("Load BP Anim")).SetMenuType(ETabSpawnerMenuType::Hidden);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_3UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_3UITab)).SetDisplayName(FText::FromString("Create Map")).SetMenuType(ETabSpawnerMenuType::Hidden);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_4UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_4UITab)).SetDisplayName(FText::FromString("Create Map With Custom BP")).SetMenuType(ETabSpawnerMenuType::Hidden);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_5UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_5UITab)).SetDisplayName(FText::FromString("Create Map With Refrenced BP")).SetMenuType(ETabSpawnerMenuType::Hidden);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_6UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_6UITab)).SetDisplayName(FText::FromString(" Clean up Map")).SetMenuType(ETabSpawnerMenuType::Hidden);

  FGlobalTabmanager::Get()->RegisterNomadTabSpawner(Menu1_7UITabName, FOnSpawnTab::CreateRaw(this, &FPluginMenuModule::OnSpawnMenu1_7UITab)).SetDisplayName(FText::FromString(" Clean up Folder")).SetMenuType(ETabSpawnerMenuType::Hidden);

  TabNames.Add(Menu1_1UITabName);
  TabNames.Add(Menu1_2UITabName);
  TabNames.Add(Menu1_3UITabName);
  TabNames.Add(Menu1_4UITabName);
  TabNames.Add(Menu1_5UITabName);
  TabNames.Add(Menu1_6UITabName);
  TabNames.Add(Menu1_7UITabName);
}

void FPluginMenuModule::ShutdownModule() {
  // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
  // we call this function before unloading the module.
  FLevelEditorModule &LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
  LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(MenuExtender);
  LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);

  FPluginMenuStyle::Shutdown();

  FPluginMenuCommands::Unregister();

  for (auto TabName : TabNames) {
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
  }

  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_1UITabName);
  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_2UITabName);
  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_3UITabName);
  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_4UITabName);
  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_5UITabName);
  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_6UITabName);
  // FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Menu1_7UITabName);
}

TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_1UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [SNew(SLoadSkeletonAnimationUI)];
}

TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_2UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [SNew(SLoadBPAnimationUI)];
}
TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_3UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [

          SNew(SMutiSceneMapUI)];
}

TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_4UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [SNew(SCreateCustomBPMapUI)];
}

TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_5UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [SNew(SCreateBPMapUI)];
}

TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_6UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [SNew(SCleanUpMapUI)];
}

TSharedRef<SDockTab> FPluginMenuModule::OnSpawnMenu1_7UITab(const FSpawnTabArgs &SpawnTabArgs) {
  return SNew(SDockTab)
      [SNew(SCleanUpFolderUI)
       // SNew(STestWindowUI)
  ];
}

void FPluginMenuModule::MenuEntryClickedHandle(FName TabName) {
  OpenMenuTab(TabName);
}

void FPluginMenuModule::AddMenuExtension(FMenuBuilder &Builder) {
  Builder.AddMenuEntry(FPluginMenuCommands::Get().OpenPluginWindow);
}

void FPluginMenuModule::AddToolbarExtension(FToolBarBuilder &Builder) {
  // Builder.AddToolBarButton(FPluginMenuCommands::Get().OpenPluginWindow);
}

void FPluginMenuModule::AddPullDownMenu(FMenuBarBuilder &MenuBarBuilder) {
  MenuBarBuilder.AddPullDownMenu(
      FText::FromString("Plugin Menu"),
      FText::FromString("Plugin Menu"),
      FNewMenuDelegate::CreateRaw(this, &FPluginMenuModule::FillMenu),
      "Custom"
  );
}

void FPluginMenuModule::FillMenu(FMenuBuilder &MenuBuilder) {
  MenuBuilder.BeginSection("MapGenerator");
  {
    MenuBuilder.AddSubMenu(
        FText::FromString("Map Generator"),
        FText::FromString("Auto Generate Map With Anim Data"),
        FNewMenuDelegate::CreateRaw(this, &FPluginMenuModule::FillSubMenu1)
    );
  }
  MenuBuilder.EndSection();
}

void FPluginMenuModule::FillSubMenu1(FMenuBuilder &MenuBuilder) {
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_1UI, NAME_None,
      FText::FromString("Load Skeleton Animation"),
      FText::FromString("Load BP Character In Level Sequence"),
      FSlateIcon()
  );
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_2UI, NAME_None,
      FText::FromString("Load BP Animation"),
      FText::FromString("Load BP Character In Level Sequence"),
      FSlateIcon()
  );
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_3UI, NAME_None,
      FText::FromString("Create Map"),
      FText::FromString("Generate Map With Anim Data"),
      FSlateIcon()
  );
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_4UI, NAME_None,
      FText::FromString("Create Map With Custom BP"),
      FText::FromString("Generate Map With Custom BP Assets"),
      FSlateIcon()
  );
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_5UI, NAME_None,
      FText::FromString("Create Map With Referenced BP"),
      FText::FromString("Generate Map With Referenced BP Assets"),
      FSlateIcon()
  );
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_6UI, NAME_None,
      FText::FromString("Clean up Map"),
      FText::FromString("Clean up Map"),
      FSlateIcon()
  );
  MenuBuilder.AddMenuEntry(
      FPluginMenuCommands::Get().OpenMenu1_7UI, NAME_None,
      FText::FromString("Clean up Folder"),
      FText::FromString("Clean up Folder"),
      FSlateIcon()
  );
}

FName FPluginMenuModule::FindPlaceholderTab() {
  FName FoundTabName;

  if (TabNames.Num()) {
    for (auto TabName : TabNames) {
      if (MenuTabManager.IsValid())
        if (MenuTabManager->FindExistingLiveTab(TabName)) {
          FoundTabName = TabName;
          break;
        }
    }
  }
  return FoundTabName;
}

void FPluginMenuModule::OpenMenuTab(FName TabName) {
  if (MenuTabManager.IsValid()) {
    FName PlaceholderTabName = FindPlaceholderTab();
    if (PlaceholderTabName.IsNone()) {
      MenuTabManager->TryInvokeTab(TabName);
    } else {
      if (FGlobalTabmanager::Get()->FindExistingLiveTab(TabName).IsValid()) {
        MenuTabManager->TryInvokeTab(TabName);
      } else {
        TSharedRef<SDockTab> PluginTab = FGlobalTabmanager::Get()->TryInvokeTab(TabName).ToSharedRef();
        PluginTab.Get().RequestCloseTab();
        MenuTabManager->InsertNewDocumentTab(PlaceholderTabName, FTabManager::ESearchPreference::PreferLiveTab, PluginTab);
      }
    }
  } else {
    MenuTabManager = FGlobalTabmanager::Get();
    MenuTabManager->TryInvokeTab(TabName);
  }
}

void FPluginMenuModule::AddSubMenuEntry(TArray<struct FMenuEntryInfo> SubMenuEntries) {
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPluginMenuModule, PluginMenu)
