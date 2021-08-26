// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DoodleAssetImportData.generated.h"


class UFactory;
class FJsonObject;

/**
 * 
 */
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

    /* 导入文件时fbx skeleton 所在文件夹*/
    FString fbx_skeleton_dir;
    /* 导入文件时fbx skeleton 文件名称 */
    FString fbx_skeleton_file_name;


    /* 导入文件时的json 数据 */
    TSharedPtr<FJsonObject> ImportGroupJsonData;

    import_file_type import_type;

    std::uint64_t start_frame;
    std::uint64_t end_frame;
};
