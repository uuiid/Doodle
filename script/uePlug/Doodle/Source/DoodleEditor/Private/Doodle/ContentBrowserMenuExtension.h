#pragma once

#include "CoreMinimal.h"

class FContentBrowserMenuExtension {
 public:
  FContentBrowserMenuExtension(const TArray<FAssetData>& InPaths);
  void AddMenuEntry(FMenuBuilder& MenuBuilder);
  void FillSubmenu(FMenuBuilder& MenuBuilder);
  void OnRegenerateLODSClicked();

 private:
  TArray<FAssetData> Paths;
};