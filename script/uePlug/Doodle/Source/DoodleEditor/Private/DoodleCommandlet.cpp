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
UDoodleAssCreateCommandlet::UDoodleAssCreateCommandlet(
    const FObjectInitializer &ObjectInitializer
)
    : Super(ObjectInitializer), import_setting_path() {
  LogToConsole = true;
}
bool UDoodleAssCreateCommandlet::parse_params(const FString &in_params) {
  const TCHAR *ParamStr = *in_params;
  TMap<FString, FString> ParamsMap;
  ParseCommandLine(ParamStr, CmdLineTokens, CmdLineSwitches, ParamsMap);
  import_setting_path = ParamsMap.FindRef(TEXT("path"));
  UE_LOG(LogTemp, Log, TEXT("导入设置路径 %s"), import_setting_path.IsEmpty() ? (*import_setting_path) : TEXT("空"));

  return !import_setting_path.IsEmpty();
}

static bool SavePackage(UPackage *Package, const FString &PackageFilename) {
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27
  return GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFilename, GWarn);
#else if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 2)
  FSavePackageArgs L_Arg{};
  L_Arg.Error         = GWarn;
  L_Arg.TopLevelFlags = RF_Standalone;
  return GEditor->SavePackage(Package, nullptr, *PackageFilename, L_Arg);
#endif  // ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 27
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

void UDoodleAssCreateCommandlet::save_temp_json(const FString &out_path) {
  FDoodleAssetImportData l_data{};
  l_data.import_file_path       = "import_file_path.abc(or fbx)";
  l_data.import_file_save_dir   = "ue4 file save path";
  l_data.fbx_skeleton_file_name = "ue4_skeleton_path_dir";
  l_data.import_type            = EDoodleImportType::abc;
  l_data.start_frame            = 1001;
  l_data.end_frame              = 1200;
  FString l_josn{};
  FJsonObjectConverter::UStructToJsonObjectString<FDoodleAssetImportData>(
      l_data, l_josn, CPF_None, CPF_None
  );
  FFileHelper::SaveStringToFile(
      l_josn, *(out_path / "doodle_import_data_optional.json")
  );

  FDoodleAssetImportDataGroup l_list{};
  l_list.groups.Add(l_data);
  l_list.groups.Add(l_data);
  FJsonObjectConverter::UStructToJsonObjectString<FDoodleAssetImportDataGroup>(
      l_list, l_josn, CPF_None, CPF_None
  );
  FFileHelper::SaveStringToFile(l_josn, *(out_path / "doodle_import_data_main.json"));

  return;
}

int32 UDoodleAssCreateCommandlet::Main(const FString &Params) {
  UE_LOG(LogTemp, Log, TEXT("开始解析参数"));
  if (!parse_params(Params)) {
    save_temp_json(FPaths::ProjectDir());
    return 0;
  }

  return 0;
}
