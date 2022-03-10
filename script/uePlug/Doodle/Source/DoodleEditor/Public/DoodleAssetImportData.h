// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

///
///
#include "DoodleAssetImportData.generated.h"
class UFactory;
class FJsonObject;

UENUM()
enum class EDoodleImportType : uint8 { None = 0, Abc, Fbx };

/**
 *
 */
USTRUCT()
struct FDoodleAssetImportData {
  GENERATED_BODY()
 public:
  bool is_valid() const;

  void initialize(TSharedPtr<FJsonObject> InImportGroupJsonData);

 public:
  /* 导入文件的路径(文件名称) */
  UPROPERTY()
  FString import_file_path;
  /* 保存文件的路径(目录) */
  UPROPERTY()
  FString import_file_save_dir;

  /* 导入文件时fbx skeleton 所在文件夹*/
  UPROPERTY()
  FString fbx_skeleton_dir;
  /* 导入文件时fbx skeleton 文件名称 */
  UPROPERTY()
  FString fbx_skeleton_file_name;

  /* 导入文件时的json 数据 */
  TSharedPtr<FJsonObject> ImportGroupJsonData;
  UPROPERTY()
  EDoodleImportType import_type;
  UPROPERTY()
  uint64 start_frame;
  UPROPERTY()
  uint64 end_frame;
};

USTRUCT()
struct FDoodleAssetImportDataGroup {
  GENERATED_BODY()

  UPROPERTY()
  TArray<FDoodleAssetImportData> groups;
};
