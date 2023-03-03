#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"

/**
 * 资产动作, 打开捏脸的配置
 *
 */
class FAssetTypeActions_CreateCharacter : public FAssetTypeActions_Base {
  EAssetTypeCategories::Type AssType;

 public:
  FAssetTypeActions_CreateCharacter(EAssetTypeCategories::Type InAssetCategory)
      : AssType(InAssetCategory){};

  // IAssetTypeActions interface
  virtual FText GetName() const override;
  virtual FColor GetTypeColor() const override;
  virtual UClass* GetSupportedClass() const override;
  virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
  virtual uint32 GetCategories() override;
  // End of IAssetTypeActions interface
};
