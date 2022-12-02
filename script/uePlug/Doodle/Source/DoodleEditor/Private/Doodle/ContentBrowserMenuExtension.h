#pragma once

#include "CoreMinimal.h"

class FContentBrowserMenuExtension {
 public:
  FContentBrowserMenuExtension(const TArray<FAssetData>& InPaths);
  void AddMenuEntry(FMenuBuilder& MenuBuilder);
  void FillSubmenu(FMenuBuilder& MenuBuilder);

 private:
  void OnRegenerateLODSClicked();
  void ResizeTexTure();

  void BoildSkinLODS(USkeletalMesh* In_Skin_Mesh);
  TArray<FAssetData> Paths;
};