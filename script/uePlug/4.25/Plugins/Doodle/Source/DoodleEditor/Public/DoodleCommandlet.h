#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "DoodleCommandlet.generated.h"

class UFactory;
class FJsonObject;
namespace {
    class Episodes {
    public:
        Episodes() = default;
        ~Episodes() = default;
        uint64 eps;
    };

    class Shot {
    public:
        Shot() = default;
        ~Shot() = default;
        uint64 shot;
        TOptional<FString> shot_ab;
    };
}



UCLASS()
class DOODLEEDITOR_API UDoodleAssetImportData : public UObject
{
    GENERATED_BODY()
public:
    enum class import_file_type : uint8 {
        None,
        Abc,
        Fbx
    };

    bool is_valid() const;

    void initialize(TSharedPtr<FJsonObject> InImportGroupJsonData);
public:
    /* 导入文件的路径(文件名称) */
    FString import_file_path;
    /* 保存文件的路径(目录) */
    FString import_file_save_dir;
    /* 导入文件时的json 数据 */
    TSharedPtr<FJsonObject> ImportGroupJsonData;

    import_file_type import_type;

};




UCLASS()
class DOODLEEDITOR_API UDoodleAssCreateCommandlet : public UCommandlet
{
    GENERATED_UCLASS_BODY()

        /** Parsed commandline tokens */
        TArray<FString> CmdLineTokens;

    /** Parsed commandline switches */
    TArray<FString> CmdLineSwitches;
private:
    /**
     *
     */
    UPROPERTY()
        UAutomatedAssetImportData* GlobalImportData;

    /** */
    UPROPERTY()
        TArray<UAutomatedAssetImportData*> ImportDataList;

    UPROPERTY()
        TArray<UDoodleAssetImportData*> import_setting_list;

    FString import_setting_path;
private:
    bool parse_params(const FString& in_params);
    bool parse_import_setting(const FString& in_import_setting_file);
    bool import_and_save(const TArray<UAutomatedAssetImportData*>& assets_import_list);
    void ClearDirtyPackages();
    //static bool SavePackage(UPackage* Package, const FString& PackageFilename)
public:
    virtual int32 Main(const FString& Params) override;
};