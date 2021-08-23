#include "DoodleCommandlet.h"
#include "AbcImportSettings.h"
#include "AssetImportTask.h"
#include "Factories/FbxFactory.h"

#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"

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
    UE_LOG(LogTemp, Log, TEXT("导入设置路径 %s"),
        import_setting_path.IsEmpty() ? (*import_setting_path) : TEXT("空"));

    return !import_setting_path.IsEmpty();
}
bool UDoodleAssCreateCommandlet::parse_import_setting(const FString& in_import_setting_file)
{
    FString k_json_str;
    if (FFileHelper::LoadFileToString(k_json_str, *in_import_setting_file)) {
        TSharedRef<TJsonReader<>> k_json_r = TJsonReaderFactory<>::Create(k_json_str);
        TSharedPtr<FJsonObject> k_root;
        if (FJsonSerializer::Deserialize(k_json_r, k_root) && k_root.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("开始读取json配置文件"));
            auto k_group = k_root->TryGetField("groups");
            if (k_group)
            {
                for (auto& i : k_group->AsArray()) {
                    auto  import_setting = NewObject<UDoodleAssetImportData>(this);
                    import_setting->initialize(i->AsObject());
                    if (import_setting->is_valid())
                        import_setting_list.Add(import_setting);
                }
            }
            else
            {
                auto  import_setting = NewObject<UDoodleAssetImportData>(this);
                import_setting->initialize(k_root);
                if (import_setting->is_valid())
                    import_setting_list.Add(import_setting);
            }
        }
    }

    for (auto& i : import_setting_list) {
        UE_LOG(LogTemp, Log, TEXT("开始开始创建导入配置"));
        auto k_setting = NewObject<UAutomatedAssetImportData>(this);
        k_setting->bSkipReadOnly = true;
        k_setting->Filenames.Add(i->import_file_path);
        k_setting->DestinationPath = i->import_file_save_dir;
        k_setting->bReplaceExisting = true;
        UE_LOG(LogTemp, Log, TEXT("导入目标路径为 %s"), *(i->import_file_save_dir));
        switch (i->import_type)
        {
        case UDoodleAssetImportData::import_file_type::Abc: {
            UE_LOG(LogTemp, Log, TEXT("读取到abc文件 %s"), *(i->import_file_path));
            k_setting->FactoryName = "AlembicImportFactory";
            k_setting->Initialize(nullptr);
            auto k_abc_stting = UAbcImportSettings::Get();
            UAssetImportTask* k_assetImportTask = NewObject<UAssetImportTask>(this);
            k_setting->Factory->AssetImportTask = k_assetImportTask;
            k_abc_stting->ImportType = EAlembicImportType::GeometryCache;       //导入为几何缓存
            k_abc_stting->MaterialSettings.bCreateMaterials = false;            //不创建材质
            k_abc_stting->MaterialSettings.bFindMaterials = true;               //寻找材质
            k_abc_stting->ConversionSettings.Preset = EAbcConversionPreset::Max;//导入预设为3dmax
            k_abc_stting->GeometryCacheSettings.bFlattenTracks = true;          //合并轨道
            k_abc_stting->SamplingSettings.bSkipEmpty = true;                   // 跳过空白帧
            k_abc_stting->SamplingSettings.FrameStart = i->start_frame;         //开始帧
            k_abc_stting->SamplingSettings.FrameEnd = i->end_frame;             //结束帧
            k_abc_stting->SamplingSettings.FrameSteps = 1;                      //帧步数
            k_assetImportTask->bAutomated = true;
            k_assetImportTask->bReplaceExisting = true;
            k_assetImportTask->bSave = true;
            k_assetImportTask->Options = k_abc_stting;

            ImportDataList.Add(k_setting);
            break;
        }
        case UDoodleAssetImportData::import_file_type::Fbx:
        {
            UE_LOG(LogTemp, Log, TEXT("读取到fbx文件 %s"), *(i->import_file_path));
            k_setting->FactoryName = "FbxFactory";
            k_setting->Initialize(nullptr);

            UFbxFactory* k_fbx_f = Cast<UFbxFactory>(k_setting->Factory);
            if (!k_fbx_f)
                break;
            FString k_fbx_dir = i->fbx_skeleton_dir.IsEmpty() ? FString{ TEXT("/Game/Character/") } : i->fbx_skeleton_dir;
            FString k_fbx_name = i->fbx_skeleton_file_name;
            if (k_fbx_name.IsEmpty()) // 名称为空的情况下直接导入几何体和网格
            {
                UE_LOG(LogTemp, Log, TEXT("没有指定骨骼名称, 直接导入骨骼和动画"));
                setting_import_fbx_is_skobj(k_fbx_f);
            }
            else //尝试加载骨骼物体， 并且只导入动画
            {
                UE_LOG(LogTemp, Log, TEXT("找到骨骼名称，开始尝试加载 %s"), *k_fbx_name);
                USkeleton* skinObj = LoadObject<USkeleton>(
                    USkeleton::StaticClass(),
                    *k_fbx_name,
                    nullptr,
                    LOAD_ResolvingDeferredExports);
                if (skinObj) {
                    UE_LOG(LogTemp, Log, TEXT("加载 %s 成功， 只导入动画"), *k_fbx_name);
                    k_fbx_f->ImportUI->Skeleton = skinObj;
                    k_fbx_f->ImportUI->MeshTypeToImport = FBXIT_Animation;
                    k_fbx_f->ImportUI->OriginalImportType = FBXIT_SkeletalMesh;
                    k_fbx_f->ImportUI->bImportAsSkeletal = true;
                    k_fbx_f->ImportUI->bImportMesh = false;
                    k_fbx_f->ImportUI->bImportAnimations = true;
                    k_fbx_f->ImportUI->bImportRigidMesh = false;
                    k_fbx_f->ImportUI->bImportMaterials = false;
                    k_fbx_f->ImportUI->bImportTextures = false;
                    k_fbx_f->ImportUI->bResetToFbxOnMaterialConflict = false;


                    k_fbx_f->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
                    k_fbx_f->ImportUI->bAutomatedImportShouldDetectType = false;
                    k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
                    k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks = true;
                    k_fbx_f->ImportUI->bAllowContentTypeImport = true;
                }
                else {
                    UE_LOG(LogTemp, Log, TEXT("加载 %s 失败， 导入骨骼网格体和动画"), *k_fbx_name);
                    setting_import_fbx_is_skobj(k_fbx_f);
                }
            }
            ImportDataList.Add(k_setting);
            break;
        }
        case UDoodleAssetImportData::import_file_type::None:
        {
            UE_LOG(LogTemp, Log, TEXT("未知导入类型， 跳过"));
            k_setting->Initialize(nullptr);
            break;
        }
        default:
            break;
        }
    }


    return import_setting_list.Num() != 0 && ImportDataList.Num() != 0;
}

void UDoodleAssCreateCommandlet::setting_import_fbx_is_skobj(UFbxFactory* k_fbx_f)
{
    k_fbx_f->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
    k_fbx_f->ImportUI->OriginalImportType = FBXIT_SkeletalMesh;
    k_fbx_f->ImportUI->bImportAsSkeletal = true;
    k_fbx_f->ImportUI->bImportMesh = true;
    k_fbx_f->ImportUI->bImportAnimations = true;
    k_fbx_f->ImportUI->bImportRigidMesh = true;
    k_fbx_f->ImportUI->bImportMaterials = false;
    k_fbx_f->ImportUI->bImportTextures = false;
    k_fbx_f->ImportUI->bResetToFbxOnMaterialConflict = false;


    k_fbx_f->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
    k_fbx_f->ImportUI->bAutomatedImportShouldDetectType = false;
    k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
    k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks = true;
    k_fbx_f->ImportUI->bAllowContentTypeImport = true;
}

static bool SavePackage(UPackage* Package, const FString& PackageFilename)
{
    return GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFilename, GWarn);
}
bool UDoodleAssCreateCommandlet::import_and_save(const TArray<UAutomatedAssetImportData*>& assets_import_list)
{
    UE_LOG(LogTemp, Log, TEXT("开始导入文件"));
    FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
    for (auto ImportData : assets_import_list) {
        UE_LOG(LogTemp, Log, TEXT("Importing group %s"), *ImportData->GetDisplayName());

        TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
        if (ImportedAssets.Num() > 0) {
            UE_LOG(LogTemp, Log, TEXT("导入完成， 开始保存"));
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
    UE_LOG(LogTemp, Log, TEXT("开始解析参数"));
    if (!parse_params(Params))
        return 0;

    UE_LOG(LogTemp, Log, TEXT("解析参数完成"));
    if (!parse_import_setting(import_setting_path))
        return 0;

    UE_LOG(LogTemp, Log, TEXT("开始导入和保存文件"));
    if (!import_and_save(ImportDataList))
        return 1;


    return 0;
}


