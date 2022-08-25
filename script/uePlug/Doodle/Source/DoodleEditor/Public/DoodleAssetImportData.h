// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

///
///
#include "DoodleAssetImportData.generated.h"
class UFactory;
class FJsonObject;
class UAssetImportTask;
class UAnimSequence;

UENUM()
enum class EDoodleImportType : uint8 {
  None = 0,
  abc,
  fbx,
  camera
};

/**
 *
 */
USTRUCT()
struct FDoodleAssetImportData {
  GENERATED_BODY()
 public:
  void set_fbx(UAssetImportTask *in_import_data);
  void set_abc(UAssetImportTask *in_import_data);

  UAssetImportTask *get_input(UObject *Outer);

 public:
  /* 导入文件的路径(文件名称) */
  UPROPERTY()
  FString import_file_path;
  /* 保存文件的路径(目录 + 文件名) */
  UPROPERTY()
  FString import_file_save_dir;

  /* 导入文件时fbx skeleton 路径 + 文件名称 */
  UPROPERTY()
  FString fbx_skeleton_file_name;

  UPROPERTY()
  /**
   * @brief 导入的类型
   *
   */
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

  UPROPERTY()
  uint64 start_frame;
  UPROPERTY()
  uint64 end_frame;
  UPROPERTY()
  FString world_path;
  UPROPERTY()
  FString level_path;
};
