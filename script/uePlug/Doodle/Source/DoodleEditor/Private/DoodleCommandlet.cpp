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
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SourceControlHelpers.h"
#include "UObject/SavePackage.h"  // 保存包
UDoodleAssCreateCommandlet::UDoodleAssCreateCommandlet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), import_setting_path() {
  LogToConsole = true;
  IsEditor     = true;
}
bool UDoodleAssCreateCommandlet::parse_params(const FString& in_params) {
  TMap<FString, FString> ParamsMap;
  ParseCommandLine(*in_params, CmdLineTokens, CmdLineSwitches, ParamsMap);
  FString L_ImportSetiingPath = ParamsMap.FindRef(TEXT("path"));
  UE_LOG(LogTemp, Log, TEXT("导入配置路径是 %s"), !L_ImportSetiingPath.IsEmpty() ? (*L_ImportSetiingPath) : TEXT("空"));

  if (!L_ImportSetiingPath.IsEmpty()) {  /// 解码导入的json数据

    FString L_JsonStr;

    if (FFileHelper::LoadFileToString(L_JsonStr, *L_ImportSetiingPath)) {
      TSharedRef<TJsonReader<TCHAR>> L_JsonReader = TJsonReaderFactory<TCHAR>::Create(L_JsonStr);
      TSharedPtr<FJsonObject> L_JsonObject        = MakeShareable(new FJsonObject);
      UE_LOG(LogTemp, Log, TEXT("开始读取json配置文件"));
      if (FJsonSerializer::Deserialize(L_JsonReader, L_JsonObject) && L_JsonObject.IsValid()) {
        TArray<TSharedPtr<FJsonValue>> L_Array = L_JsonObject->GetArrayField(TEXT("files"));
        for (auto&& L_Value : L_Array) {
          ImportFiles.Add(L_Value->AsString());
        }
      }
    }
  }

  return ImportFiles.IsEmpty();
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

int32 UDoodleAssCreateCommandlet::Main(const FString& Params) {
  UE_LOG(LogTemp, Log, TEXT("开始解析参数"));
  if (!parse_params(Params)) {
    FDoodleCreateLevel L_CreateLevel{};
    L_CreateLevel.ImportFiles(ImportFiles);
    return 0;
  }

  return 0;
}
