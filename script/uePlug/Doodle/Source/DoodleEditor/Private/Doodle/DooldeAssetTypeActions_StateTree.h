#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"

class FDoodleAssetTypeActions_StateTree : public FAssetTypeActions_Base {
 public:
  explicit FDoodleAssetTypeActions_StateTree(const EAssetTypeCategories::Type InAssetCategory)
      : AssetCategory(InAssetCategory){};

 protected:
  // IAssetTypeActions Implementation
  virtual FText GetName() const override {
    return NSLOCTEXT("DoodleAssetTypeActions", "FDoodleAssetTypeActions_StateTree", "Doodle");
  }

  virtual FColor GetTypeColor() const override {
    return FColor(201, 185, 29);
  }

  virtual UClass* GetSupportedClass() const override;
  virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
  virtual uint32 GetCategories() override;

 private:
  uint32 AssetCategory = EAssetTypeCategories::Gameplay;
};
