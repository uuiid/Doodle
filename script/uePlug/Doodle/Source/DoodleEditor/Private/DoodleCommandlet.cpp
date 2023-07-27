#include "DoodleCommandlet.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "DoodleCreateLevel.h"
#include "Editor.h"

#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "SourceControlHelpers.h"
#include "UObject/SavePackage.h"  // 保存包
// UDoodleAssCreateCommandlet::UDoodleAssCreateCommandlet(
//     const FObjectInitializer &ObjectInitializer
//)
//     : Super(ObjectInitializer), import_setting_path() {
//   LogToConsole = true;
// }
bool UDoodleAssCreateCommandlet::parse_params(const FString& in_params) {
  TMap<FString, FString> ParamsMap;
  ParseCommandLine(*in_params, CmdLineTokens, CmdLineSwitches, ParamsMap);
  import_setting_path = ParamsMap.FindRef(TEXT("path"));
  UE_LOG(LogTemp, Log, TEXT("导入配置路径是 %s"), import_setting_path.IsEmpty() ? (*import_setting_path) : TEXT("空"));

  return !import_setting_path.IsEmpty();
}

static bool SavePackage(UPackage* Package, const FString& PackageFilename) {
  FSavePackageArgs L_Arg{};
  L_Arg.Error         = GWarn;
  L_Arg.TopLevelFlags = RF_Standalone;
  return GEditor->SavePackage(Package, nullptr, *PackageFilename, L_Arg);
}

// void UDoodleAssCreateCommandlet::ClearDirtyPackages()
// {
//   TArray<UPackage *> DirtyPackages;
//   FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);
//   FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);

//   for (UPackage *Package : DirtyPackages)
//   {
//     Package->SetDirtyFlag(false);
//   }
// }

void UDoodleAssCreateCommandlet::save_temp_json(const FString& out_path) { return; }

int32 UDoodleAssCreateCommandlet::Main(const FString& Params) {
  UE_LOG(LogTemp, Log, TEXT("开始解析参数"));
  if (!parse_params(Params)) {
    save_temp_json(FPaths::ProjectDir());
    return 0;
  }

  return 0;
}
