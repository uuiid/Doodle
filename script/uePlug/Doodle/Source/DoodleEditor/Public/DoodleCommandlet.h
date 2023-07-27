#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"

///
#include "DoodleCommandlet.generated.h"

class UFactory;
class FJsonObject;
class UFbxFactory;

UCLASS()
class DOODLEEDITOR_API UDoodleAssCreateCommandlet : public UCommandlet {
  GENERATED_BODY()

  /** Parsed commandline tokens */
  TArray<FString> CmdLineTokens;

  /** Parsed commandline switches */
  TArray<FString> CmdLineSwitches;

 private:
  /**
   *
   */
  UPROPERTY()
  UAutomatedAssetImportData *GlobalImportData;

  /** */
  UPROPERTY()
  TArray<UAutomatedAssetImportData *> ImportDataList;

  FString import_setting_path;

 private:
  bool parse_params(const FString &in_params);
  void setting_import_fbx_is_skobj(UFbxFactory *k_fbx_f);

  // void ClearDirtyPackages();
  // static bool SavePackage(UPackage* Package, const FString& PackageFilename)

  void save_temp_json(const FString &out_path);

 public:
  virtual int32 Main(const FString &Params) override;
};
