#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"

class FToolBarBuilder;
class FMenuBuilder;
class UContentBrowserAssetDataSource;

class FdoodleEditorModule : public IModuleInterface {
 public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;

  /** This function will be bound to Command (by default it will bring up plugin
   * window) */
  void PluginButtonClicked();

 private:
  void RegisterMenus();

  TSharedRef<class SDockTab> OnSpawnPluginTab(
      const class FSpawnTabArgs &SpawnTabArgs
  );

 private:
  TSharedPtr<class FUICommandList> PluginCommands;
  // TStrongObjectPtr<UContentBrowserAssetDataSource> AssetDataSource;
};
