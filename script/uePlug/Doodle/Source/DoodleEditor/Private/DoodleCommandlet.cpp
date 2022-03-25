#include "DoodleCommandlet.h"

#include "AbcImportSettings.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "DoodleCreateLevel.h"
#include "Editor.h"
#include "Factories/Factory.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/ImportSettings.h"
#include "Factories/MaterialImportHelpers.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"
#include "JsonObjectConverter.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SourceControlHelpers.h"

UDoodleAssCreateCommandlet::UDoodleAssCreateCommandlet(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer), import_setting_path()
{
  LogToConsole = true;
}
bool UDoodleAssCreateCommandlet::parse_params(const FString &in_params)
{
  const TCHAR *ParamStr = *in_params;
  TMap<FString, FString> ParamsMap;
  ParseCommandLine(ParamStr, CmdLineTokens, CmdLineSwitches, ParamsMap);
  import_setting_path = ParamsMap.FindRef("path");
  UE_LOG(LogTemp, Log, TEXT("导入设置路径 %s"),
         import_setting_path.IsEmpty() ? (*import_setting_path) : TEXT("空"));

  return !import_setting_path.IsEmpty();
}
bool UDoodleAssCreateCommandlet::parse_import_setting(
    const FString &in_import_setting_file)
{
  FString k_json_str;
  TArray<FDoodleAssetImportData> import_setting_list;
  if (FFileHelper::LoadFileToString(k_json_str, *in_import_setting_file))
  {
    UE_LOG(LogTemp, Log, TEXT("开始读取json配置文件"));
    FDoodleAssetImportDataGroup l_data_list{};
    UE_LOG(LogTemp, Log, TEXT("开始测试是 FDoodleAssetImportDataGroup"));
    if (FJsonObjectConverter::JsonObjectStringToUStruct<
            FDoodleAssetImportDataGroup>(k_json_str, &l_data_list, CPF_None,
                                         CPF_None))
    {
      import_setting_list = l_data_list.groups;
    }
    UE_LOG(LogTemp, Log, TEXT("开始测试 FDoodleAssetImportData"));
    FDoodleAssetImportData l_data{};
    if (FJsonObjectConverter::JsonObjectStringToUStruct<FDoodleAssetImportData>(
            k_json_str, &l_data, CPF_None, CPF_None))
    {
      import_setting_list.Add(l_data);
    }
  }

  for (auto &i : import_setting_list)
  {
    UE_LOG(LogTemp, Log, TEXT("开始开始创建导入配置"));
    ImportDataList.Add(i.get_input(this));
  }

  return import_setting_list.Num() != 0 && ImportDataList.Num() != 0;
}

static bool SavePackage(UPackage *Package, const FString &PackageFilename)
{
  return GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFilename,
                              GWarn);
}

bool UDoodleAssCreateCommandlet::import_and_save(
    const TArray<UAutomatedAssetImportData *> &assets_import_list)
{
  UE_LOG(LogTemp, Log, TEXT("开始导入文件"));
  FAssetToolsModule &AssetToolsModule =
      FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
  for (auto ImportData : assets_import_list)
  {
    UE_LOG(LogTemp, Log, TEXT("Importing group %s"),
           *ImportData->GetDisplayName());

    TArray<UObject *> ImportedAssets =
        AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
    if (ImportedAssets.Num() > 0)
    {
      UE_LOG(LogTemp, Log, TEXT("导入完成， 开始保存"));
      UEditorLoadingAndSavingUtils::SaveDirtyPackages(true, true);
      // TArray<UPackage *> DirtyPackages;
      // TArray<FSourceControlStateRef> PackageStates;
      // FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);
      // FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);
      // for (int32 PackageIndex = 0; PackageIndex < DirtyPackages.Num();
      //      ++PackageIndex)
      // {
      //   UPackage *PackageToSave = DirtyPackages[PackageIndex];
      //   FString PackageFilename =
      //       SourceControlHelpers::PackageFilename(PackageToSave);
      // }
    }
    else
    {
      UE_LOG(LogTemp, Error, TEXT("Failed to import all assets in group %s"),
             *ImportData->GetDisplayName());
    }
  }
  return true;
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

void UDoodleAssCreateCommandlet::save_temp_json(const FString &out_path)
{
  FDoodleAssetImportData l_data{};
  l_data.import_file_path = "import_file_path.abc(or fbx)";
  l_data.import_file_save_dir = "ue4 file save path";
  l_data.fbx_skeleton_dir = "ue4_skeleton_path_dir";
  l_data.fbx_skeleton_file_name = "ue4_skeleton_path_dir";
  l_data.import_type = EDoodleImportType::Abc;
  l_data.start_frame = 1001;
  l_data.end_frame = 1200;
  FString l_josn{};
  FJsonObjectConverter::UStructToJsonObjectString<FDoodleAssetImportData>(
      l_data, l_josn, CPF_None, CPF_None);
  FFileHelper::SaveStringToFile(
      l_josn, *(out_path / "doodle_import_data_optional.json"));

  FDoodleAssetImportDataGroup l_list{};
  l_list.groups.Add(l_data);
  l_list.groups.Add(l_data);
  FJsonObjectConverter::UStructToJsonObjectString<FDoodleAssetImportDataGroup>(
      l_list, l_josn, CPF_None, CPF_None);
  FFileHelper::SaveStringToFile(l_josn,
                                *(out_path / "doodle_import_data_main.json"));

  return;
}

int32 UDoodleAssCreateCommandlet::Main(const FString &Params)
{
  UE_LOG(LogTemp, Log, TEXT("开始解析参数"));
  if (!parse_params(Params))
  {
    save_temp_json(FPaths::ProjectDir());
    return 0;
  }

  UE_LOG(LogTemp, Log, TEXT("解析参数完成"));
  if (!parse_import_setting(import_setting_path))
    return 0;

  UE_LOG(LogTemp, Log, TEXT("开始导入和保存文件"));
  if (!import_and_save(ImportDataList))
    return 1;

  return 0;
}
