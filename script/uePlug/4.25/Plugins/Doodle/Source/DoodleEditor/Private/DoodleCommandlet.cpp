#include "DoodleCommandlet.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "AutomatedAssetImportData.h"
#include "Modules/ModuleManager.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AbcImportSettings.h"
#include "AssetImportTask.h"
#include "Factories/FbxFactory.h"
#include "Factories/ImportSettings.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"


#include "AutomatedAssetImportData.h"
#include "Modules/ModuleManager.h"
#include "Factories/Factory.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Factories/ImportSettings.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Misc/FeedbackContext.h"
#include "HAL/PlatformFilemanager.h"
#include "GameFramework/WorldSettings.h"

UDoodleAssCreateCommandlet::UDoodleAssCreateCommandlet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    import_setting_path()
{
    LogToConsole = true;
}
bool UDoodleAssCreateCommandlet::parse_params(const FString& in_params)
{
    const TCHAR* ParamStr = *in_params;
    TMap<FString, FString> ParamsMap;
    ParseCommandLine(ParamStr, CmdLineTokens, CmdLineSwitches, ParamsMap);
    import_setting_path = ParamsMap.FindRef("path");

    return !import_setting_path.IsEmpty();
}
bool UDoodleAssCreateCommandlet::parse_import_setting(const FString& in_import_setting_file)
{
    FString k_json_str;
    if (FFileHelper::LoadFileToString(k_json_str, *in_import_setting_file)) {
        TSharedRef<TJsonReader<>> k_json_r = TJsonReaderFactory<>::Create(k_json_str);
        TSharedPtr<FJsonObject> k_root;
        if (FJsonSerializer::Deserialize(k_json_r, k_root) && k_root.IsValid()) {
            auto  import_setting = NewObject<UDoodleAssetImportData>(this);
            import_setting->initialize(k_root);
            if (import_setting->is_valid())
                import_setting_list.Add(import_setting);
        }
    }

    for (auto& i : import_setting_list) {
        auto k_setting = NewObject<UAutomatedAssetImportData>(this);
        k_setting->bSkipReadOnly = true;
        k_setting->Filenames.Add(i->import_file_path);
        k_setting->DestinationPath = i->import_file_save_dir;
        k_setting->bReplaceExisting = true;
        switch (i->import_type)
        {
        case UDoodleAssetImportData::import_file_type::Abc: {
            k_setting->FactoryName = "AlembicImportFactory";
            k_setting->Initialize(nullptr);
            auto k_abc_stting = UAbcImportSettings::Get();

            auto k_assetImportTask = NewObject<UAssetImportTask>(this);
            k_setting->Factory->AssetImportTask = k_assetImportTask;
            k_abc_stting->ImportType = EAlembicImportType::GeometryCache;//导入为几何缓存
            k_abc_stting->MaterialSettings.bCreateMaterials = false; //不创建材质
            k_abc_stting->MaterialSettings.bFindMaterials = true;//寻找材质
            k_abc_stting->ConversionSettings.Preset = EAbcConversionPreset::Max;//导入预设为3dmax
            k_abc_stting->GeometryCacheSettings.bFlattenTracks = true;//合并轨道
            k_abc_stting->SamplingSettings.bSkipEmpty = true;// 跳过空白帧
            k_abc_stting->SamplingSettings.FrameStart = 1000;//开始帧
            k_abc_stting->SamplingSettings.FrameEnd = 1200;//开始帧
            k_abc_stting->SamplingSettings.FrameSteps = 1;//帧步数
            k_assetImportTask->bAutomated = true;
            k_assetImportTask->bReplaceExisting = true;
            k_assetImportTask->bSave = true;
            k_assetImportTask->Options = k_abc_stting;

            ImportDataList.Add(k_setting);
            break;
        }
        case UDoodleAssetImportData::import_file_type::Fbx:
        {

            k_setting->FactoryName = "FbxFactory";
            k_setting->Initialize(nullptr);

            auto k_fbx_f = Cast<UFbxFactory>(k_setting->Factory);
            if (k_fbx_f) {
            }
            break;
        }
        case UDoodleAssetImportData::import_file_type::None:
        {
            k_setting->Initialize(nullptr);
            break;
        }
        default:
            break;
        }
    }




    return import_setting_list.Num() != 0 && ImportDataList.Num() != 0;
}

static bool SavePackage(UPackage* Package, const FString& PackageFilename)
{
    return GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFilename, GWarn);
}
bool UDoodleAssCreateCommandlet::import_and_save(const TArray<UAutomatedAssetImportData*>& assets_import_list)
{
    FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
    for (auto ImportData : assets_import_list) {
        UE_LOG(LogTemp, Log, TEXT("Importing group %s"), *ImportData->GetDisplayName());

        TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
        if (ImportedAssets.Num() > 0) {
            TArray<UPackage*> DirtyPackages;

            TArray<FSourceControlStateRef> PackageStates;

            FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);
            FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);

            for (int32 PackageIndex = 0; PackageIndex < DirtyPackages.Num(); ++PackageIndex)
            {
                UPackage* PackageToSave = DirtyPackages[PackageIndex];

                FString PackageFilename = SourceControlHelpers::PackageFilename(PackageToSave);
                SavePackage(PackageToSave, PackageFilename);
            }

        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to import all assets in group %s"), *ImportData->GetDisplayName());
        }
    }
    return true;
}
void UDoodleAssCreateCommandlet::ClearDirtyPackages()
{
    TArray<UPackage*> DirtyPackages;
    FEditorFileUtils::GetDirtyContentPackages(DirtyPackages);
    FEditorFileUtils::GetDirtyWorldPackages(DirtyPackages);

    for (UPackage* Package : DirtyPackages)
    {
        Package->SetDirtyFlag(false);
    }
}
int32 UDoodleAssCreateCommandlet::Main(const FString& Params)
{
    bool bSuccess = false;

    if (!parse_params(Params))
        return 0;

    if (!parse_import_setting(import_setting_path))
        return 0;
    if (!import_and_save(ImportDataList))
        return 1;


    return 0;
}

bool UDoodleAssetImportData::is_valid() const
{
    return !import_file_path.IsEmpty() && !import_file_save_dir.IsEmpty();
}

#define DOODLE_TO_ATTR(Attr_name, to_fun, conv) \
   { \
        auto k_val = ImportGroupJsonData->TryGetField(#Attr_name); \
        if (k_val.IsValid())\
            Attr_name = conv(k_val->to_fun());\
   }

void UDoodleAssetImportData::initialize(TSharedPtr<FJsonObject> InImportGroupJsonData)
{
    ImportGroupJsonData = InImportGroupJsonData;


    if (ImportGroupJsonData.IsValid()) {
        DOODLE_TO_ATTR(import_file_path, AsString, );
        DOODLE_TO_ATTR(import_file_save_dir, AsString, );
        DOODLE_TO_ATTR(import_type, AsNumber, static_cast<import_file_type>);
    }

}
