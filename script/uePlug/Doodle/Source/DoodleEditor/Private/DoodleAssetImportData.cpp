// Fill out your copyright notice in the Description page of Project Settings.

#include "DoodleAssetImportData.h"

/// 导入文件转换 FDoodleAssetImportData
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
/// 导入abc
#include "AbcImportSettings.h"

/// 一般的导入任务设置
#include "AssetImportTask.h"
/// 导入fbx需要
#include "Factories/Factory.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/ImportSettings.h"
#include "Factories/MaterialImportHelpers.h"

void FDoodleAssetImportData::set_fbx(UAssetImportTask *in_import_data) {
  UE_LOG(LogTemp, Log, TEXT("读取到fbx文件 %s"), *(import_file_path));
  // in_import_data->FactoryName = "FbxFactory";
  in_import_data->Factory                                        = DuplicateObject<UFbxFactory>(GetDefault<UFbxFactory>(), in_import_data);

  UFbxFactory *k_fbx_f                                           = Cast<UFbxFactory>(in_import_data->Factory);

  k_fbx_f->ImportUI->MeshTypeToImport                            = FBXIT_SkeletalMesh;
  k_fbx_f->ImportUI->OriginalImportType                          = FBXIT_SkeletalMesh;
  k_fbx_f->ImportUI->bImportAsSkeletal                           = true;
  k_fbx_f->ImportUI->bImportMesh                                 = true;
  k_fbx_f->ImportUI->bImportAnimations                           = true;
  k_fbx_f->ImportUI->bImportRigidMesh                            = true;
  k_fbx_f->ImportUI->bImportMaterials                            = false;
  k_fbx_f->ImportUI->bImportTextures                             = false;
  k_fbx_f->ImportUI->bResetToFbxOnMaterialConflict               = false;

  k_fbx_f->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
  k_fbx_f->ImportUI->bAutomatedImportShouldDetectType            = false;
  k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength =
      FBXALIT_ExportedTime;
  k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks = true;
  k_fbx_f->ImportUI->bAllowContentTypeImport                   = true;
  k_fbx_f->ImportUI->TextureImportData->MaterialSearchLocation =
      EMaterialSearchLocation::UnderRoot;

  if (!fbx_skeleton_file_name.IsEmpty())  // 名称不为空的情况下直接导入几何体和网格
  {
    UE_LOG(LogTemp, Log, TEXT("找到骨骼名称，开始尝试加载 %s"), *fbx_skeleton_file_name);
    USkeleton *skinObj =
        LoadObject<USkeleton>(USkeleton::StaticClass(), *fbx_skeleton_file_name, nullptr, LOAD_ResolvingDeferredExports);
    if (skinObj) {
      UE_LOG(LogTemp, Log, TEXT("加载 %s 成功， 只导入动画"), *fbx_skeleton_file_name);
      k_fbx_f->ImportUI->Skeleton                      = skinObj;
      k_fbx_f->ImportUI->MeshTypeToImport              = FBXIT_Animation;
      k_fbx_f->ImportUI->OriginalImportType            = FBXIT_SkeletalMesh;
      k_fbx_f->ImportUI->bImportAsSkeletal             = true;
      k_fbx_f->ImportUI->bImportMesh                   = false;
      k_fbx_f->ImportUI->bImportAnimations             = true;
      k_fbx_f->ImportUI->bImportRigidMesh              = false;
      k_fbx_f->ImportUI->bImportMaterials              = false;
      k_fbx_f->ImportUI->bImportTextures               = false;
      k_fbx_f->ImportUI->bResetToFbxOnMaterialConflict = false;

      k_fbx_f->ImportUI->SkeletalMeshImportData->bImportMorphTargets =
          true;
      k_fbx_f->ImportUI->bAutomatedImportShouldDetectType = false;
      k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength =
          FBXALIT_ExportedTime;
      k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks = true;
      k_fbx_f->ImportUI->bAllowContentTypeImport                   = true;
    } else
      UE_LOG(LogTemp, Log, TEXT("加载 %s 失败， 导入骨骼网格体和动画"), *fbx_skeleton_file_name);
  } else
    UE_LOG(LogTemp, Log, TEXT("没有指定骨骼名称, 直接导入骨骼和动画"));
}

void FDoodleAssetImportData::set_abc(UAssetImportTask *in_import_data) {
  UE_LOG(LogTemp, Log, TEXT("读取到abc文件 %s"), *(import_file_path));

  for (TObjectIterator<UClass> it{}; it; ++it) {
    if (it->IsChildOf(UFactory::StaticClass())) {
      if (it->GetName() == "AlembicImportFactory") {
        in_import_data->Factory = it->GetDefaultObject<UFactory>();
      }
    }
  }

  /// 获取abc默认设置并修改
  UAbcImportSettings *k_abc_stting = UAbcImportSettings::Get();
  // UAbcImportSettings *k_abc_stting = DuplicateObject<
  //     UAbcImportSettings>(UAbcImportSettings::Get(), in_import_data);
  k_abc_stting->ImportType =
      EAlembicImportType::GeometryCache;                    // 导入为几何缓存
  k_abc_stting->MaterialSettings.bCreateMaterials = false;  // 不创建材质
  k_abc_stting->MaterialSettings.bFindMaterials   = true;   // 寻找材质
  k_abc_stting->ConversionSettings.Preset =
      EAbcConversionPreset::Max;  // 导入预设为3dmax
  k_abc_stting->ConversionSettings.bFlipV            = true;
  k_abc_stting->ConversionSettings.Scale.X           = 1.0;
  k_abc_stting->ConversionSettings.Scale.Y           = -1.0;
  k_abc_stting->ConversionSettings.Scale.Z           = 1.0;
  k_abc_stting->ConversionSettings.Rotation.X        = 90.0;
  k_abc_stting->ConversionSettings.Rotation.Y        = 0.0;
  k_abc_stting->ConversionSettings.Rotation.Z        = 0.0;

  k_abc_stting->GeometryCacheSettings.bFlattenTracks = true;         // 合并轨道
  k_abc_stting->SamplingSettings.bSkipEmpty          = true;         // 跳过空白帧
  k_abc_stting->SamplingSettings.FrameStart          = start_frame;  // 开始帧
  k_abc_stting->SamplingSettings.FrameEnd            = end_frame;    // 结束帧
  k_abc_stting->SamplingSettings.FrameSteps          = 1;            // 帧步数

  in_import_data->Options                            = k_abc_stting;
}

UAssetImportTask *FDoodleAssetImportData::get_input(UObject *Outer) {
  UAssetImportTask *l_task         = NewObject<UAssetImportTask>(Outer);
  l_task->bAutomated               = true;
  l_task->bReplaceExisting         = true;
  l_task->bReplaceExistingSettings = true;
  /// @fix 此处不需要自动保存, 否则会出现一个ue4.27中发现的bug 会将
  /// UAssetImportTask::ImportedObjectPaths 值转换为乱码

  // l_task->bSave = true;
  l_task->DestinationPath          = import_file_save_dir;
  l_task->Filename                 = import_file_path;

  // UAutomatedAssetImportData *k_setting = NewObject<UAutomatedAssetImportData>(Outer);
  // k_setting->bSkipReadOnly = true;
  // k_setting->Filenames.Add(import_file_path);
  // k_setting->DestinationPath = import_file_save_dir;
  // k_setting->bReplaceExisting = true;
  // k_setting->GroupName = import_file_save_dir;
  UE_LOG(LogTemp, Log, TEXT("导入目标路径为 %s"), *(import_file_save_dir));
  switch (import_type) {
    case EDoodleImportType::abc:
      set_abc(l_task);
      break;
    case EDoodleImportType::fbx:
      set_fbx(l_task);
      break;
    default:
      break;
  }
  return l_task;
}
