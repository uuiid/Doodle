#include "DoodleCopyMaterial.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "EditorAssetLibrary.h"
#include "GeometryCache.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
// 测试使用
#include "Doodle/ResizeTexture.h"

// 批量导入
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "HAL/FileManagerGeneric.h"
#include "IAssetTools.h"
#include "Internationalization/Regex.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Doodle/DoodleImportFbxUI.h"

/// 设置使用
#include "DoodleEditorSetting.h"
// 重命名资产
#include "EditorAssetLibrary.h"
// 保存包需要
#include "Editor.h"
// 自定义abc导入
// #include "AbcWrap/DoodleAbcFactory.h"
//  设置粒子系统材质需要
#include <Particles/ParticleEmitter.h>
#include <Particles/ParticleSystem.h>

#include "Particles/ParticleModuleRequired.h"
// 更改材质属性时的委托发布
#include "EditorSupportDelegates.h"

// 我们在测试时使用
#include "DoodleCreateLevel.h"
// 编辑器脚本
#include "EditorAssetLibrary.h"
/// 打开exe需要
#include "GenericPlatform/GenericPlatformProcess.h"

void DoodleCopyMat::Construct(const FArguments &Arg) {
  // 这个是ue界面的创建方法

  ChildSlot
      [SNew(SHorizontalBox) +
       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(
               1.f,
               1.f
           ))[SNew(SButton)  // 创建按钮
                  .OnClicked(this,
                             &DoodleCopyMat::getSelect)  // 添加回调函数
                      [SNew(STextBlock)
                           .Text(FText::FromString(
                               TEXT("获得选择物体")
                           ))  // 按钮中的字符
  ]
                  .ToolTipText_Lambda([=]() -> FText { return FText::FromString(TEXT("获得选中物体")); })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(
               1.f,
               1.f
           ))[SNew(SButton)  // 创建按钮
                  .OnClicked(this,
                             &DoodleCopyMat::CopyMateral)  // 添加回调函数
                      [SNew(STextBlock)
                           .Text(FText::FromString(
                               TEXT("复制材质列表")
                           ))  // 按钮中的字符
  ]
                  .ToolTipText_Lambda([=]() -> FText { return FText::FromString(
                                                           TEXT("复制选中物体的材质列表")
                                                       ); })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(
               1.f,
               1.f
           ))[SNew(SButton)
                  .OnClicked_Lambda([]() -> FReply {
                    FGlobalTabmanager::Get()->TryInvokeTab(SDoodleImportFbxUI::Name);
                    return FReply::Handled();
                  })  // 批量导入
                      [SNew(STextBlock)
                           .Text(FText::FromString(TEXT("批量导入")))]
                  .ToolTipText_Lambda([=]() -> FText { return FText::FromString(
                                                           TEXT("批量导入fbx和abc文件")
                                                       ); })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(1.f, 1.f))
               [SNew(SButton)
                    .OnClicked(this,
                               &DoodleCopyMat::BathReameAss)  // 批量重命名
                        [SNew(STextBlock)
                             .Text(FText::FromString(TEXT("批量修改材质名称")))]
                    .ToolTipText_Lambda([=]() -> FText { return FText::FromString(
                                                             TEXT("选中骨骼物体,"
                                                                  "会将材料名称和骨骼物体的插槽名称统一")
                                                         ); })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(
               FMargin{1.f, 1.f}
           )[SNew(SCheckBox).OnCheckStateChanged_Lambda(
               [this](const ECheckBoxState &in) /*-> FReply*/
               {
                 this->bEnableSeparateTranslucency =
                     in == ECheckBoxState::Checked;
               }
           )] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin{1.f, 1.f})
               [SNew(SButton)
                    .OnClicked_Lambda([this]() -> FReply {
                      FContentBrowserModule &contentBrowserModle =
                          FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
                              "ContentBrowser"
                          );
                      TArray<FAssetData> selectedAss;
                      contentBrowserModle.Get().GetSelectedAssets(selectedAss);
                      FResizeTexture L_r{};
                      for (auto &&item : selectedAss) {
                        UObject *loadObj = item.GetAsset();
                        if (loadObj == nullptr)
                          continue;
                        if (UTexture2D *l_tex = Cast<UTexture2D>(loadObj)) {
                          L_r.Resize(l_tex);
                        }
                      }

                      return FReply::Handled();
                    })[SNew(STextBlock).Text(FText::FromString(TEXT("test")))]
                    .ToolTipText_Lambda([]() -> FText { return FText::FromString(
                                                            TEXT("测试使用")
                                                        ); })]];
}

void DoodleCopyMat::AddReferencedObjects(FReferenceCollector &collector) {
  // collector.AddReferencedObjects();
}

FReply DoodleCopyMat::getSelect() {
  /*
            获得文件管理器中的骨架网格物体的选择
            这是一个按钮的回调参数
            */

  // 获得文件管理器的模块(或者类?)
  FContentBrowserModule &contentBrowserModle =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser"
      );
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);
  for (int i = 0; i < selectedAss.Num(); i++) {
    // 测试选中物体是否是骨骼物体
    if (selectedAss[i].GetClass()->IsChildOf<USkeletalMesh>()) {
      // 如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("确认骨骼物体 %s"), *(selectedAss[i].GetFullName()));

      UObject *skinObj = selectedAss[i].ToSoftObjectPath().TryLoad();
      // assLoad.LoadAsset(selectedAss[i].GetFullName( ));
      // 将加载的类转换为skeletalMesh类并进行储存
      if (skinObj) {
        copySoureSkinObj = Cast<USkeletalMesh>(skinObj);
        UE_LOG(LogTemp, Log, TEXT("%s"), *(copySoureSkinObj->GetPathName()));
      }

    }  // 测试是否是几何缓存物体
    else if (selectedAss[i].GetClass() == UGeometryCache::StaticClass()) {
      // 如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("确认几何缓存  %s"), *(selectedAss[i].GetFullName()));
      UObject *cacheObj = selectedAss[i].ToSoftObjectPath().TryLoad();
      if (cacheObj) {
        copySoureGeoCache = cacheObj;
        //*(cacheObj->GetFullName( )
        UE_LOG(LogTemp, Log, TEXT("%s"), *(cacheObj->GetFullName()));
      }
    }
    // bool is =selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( );
    // UE_LOG(LogTemp, Log, TEXT("%s"), *(FString::FromInt(is)));
    // selectedAss[i].GetFullName( )
  }
  return FReply::Handled();
}

FReply DoodleCopyMat::CopyMateral() {
  FContentBrowserModule &contentBrowserModle =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser"
      );
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);
  for (int i = 0; i < selectedAss.Num(); i++) {
    UObject *loadObj =
        selectedAss[i]
            .ToSoftObjectPath()
            .TryLoad();  // assLoad.LoadAsset(selectedAss[i].GetFullName( ));

    // 测试选中物体是否是骨骼物体
    if (selectedAss[i].GetClass()->IsChildOf<USkeletalMesh>()) {
      // 如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"), *(selectedAss[i].GetFullName()));

      USkeletalMesh *copyTrange = Cast<USkeletalMesh>(loadObj);

      UE_LOG(LogTemp, Log, TEXT("确认并加载为几何物体 %s"), *(copyTrange->GetPathName()));
      ENGINE_MINOR_VERSION;
#if ENGINE_MINOR_VERSION == 27
      TArray<FSkeletalMaterial> trangeMat = copyTrange->GetMaterials();
      if (copySoureSkinObj)
        for (int m = 0; m < trangeMat.Num(); m++) {
          trangeMat[m] = copySoureSkinObj->GetMaterials()[m];
          UE_LOG(LogTemp, Log, TEXT("%s"), *(trangeMat[m].MaterialInterface->GetPathName()));
          // 材质插槽命名
        }
      copyTrange->SetMaterials(trangeMat);
#else if ENGINE_MINOR_VERSION <= 26
      TArray<FSkeletalMaterial> trangeMat = copyTrange->Materials;
      if (copySoureSkinObj)
        for (int m = 0; m < trangeMat.Num(); m++) {
          trangeMat[m] = copySoureSkinObj->Materials[m];
          UE_LOG(LogTemp, Log, TEXT("%s"), *(trangeMat[m].MaterialInterface->GetPathName()));
          // 材质插槽命名
        }
      copyTrange->Materials = trangeMat;
#endif

    }  // 如果是几何缓存就复制几何缓存
    else if (selectedAss[i].GetClass() == UGeometryCache::StaticClass()) {
      UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"), *(selectedAss[i].GetFullName()));

      UGeometryCache *copyTrange          = Cast<UGeometryCache>(loadObj);
      TArray<UMaterialInterface *> trange = copyTrange->Materials;

      if (copySoureGeoCache) {
        auto soure = Cast<UGeometryCache>(copySoureGeoCache);
        for (int m = 0; m < trange.Num(); m++) {
          trange[m] = soure->Materials[m];
          UE_LOG(LogTemp, Log, TEXT("%s"), *(trange[m]->GetPathName()));
        }
      }
      copyTrange->Materials = trange;
    }
  }
  return FReply::Handled();
}

FReply DoodleCopyMat::BathImport() {
  const UDoodleEditorSetting *l_setting = GetDefault<UDoodleEditorSetting>();
  if (!FPaths::FileExists(l_setting->DoodleExePath)) {
    FText l_t = FText::FromString(TEXT("错误"));
    FMessageDialog::Open(EAppMsgType::OkCancel, FText::FromString(TEXT("没有找到DoodleExe.exe运行文件,请在设置中输入正确的路径")), &l_t);
    return FReply::Handled();
  }

  FString l_temp_dir  = FPaths::Combine(FPaths::ProjectSavedDir(), FPaths::CreateTempFilename(TEXT("doodle"), TEXT("import"), TEXT("")));
  FString l_prj       = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
  FString l_abs_path  = FPaths::ConvertRelativePathToFull(l_temp_dir);
  FString l_com       = FString::Format(TEXT(R"(--ue4outpath="{0}" --ue4Project="{1}")"), TArray<FStringFormatArg>{FStringFormatArg{l_abs_path}, FStringFormatArg{l_prj}});
  FString l_start_dir = FPaths::GetPath(l_setting->DoodleExePath);
  FProcHandle l_h     = FPlatformProcess::CreateProc(
      *l_setting->DoodleExePath,
      *l_com,
      true,
      false,
      false,
      nullptr,
      0,
      *l_start_dir,
      nullptr,
      nullptr
  );
  FPlatformProcess::WaitForProc(l_h);

  IFileManager::Get().IterateDirectory(
      *l_abs_path,
      [](const TCHAR *in_path, bool in_) -> bool {
        doodle::init_ue4_project l_import_tool{};
        FString l_path{in_path};
        if (FPaths::FileExists(l_path) && FPaths::GetExtension(l_path, true) == TEXT(".json_doodle")) {
          l_import_tool.import_ass_data(l_path);
        }
        return true;
      }
  );

  return FReply::Handled();
}

FReply DoodleCopyMat::BathReameAss() {
  FContentBrowserModule &contentBrowserModle =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser"
      );
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);

  for (auto &&item : selectedAss) {
    UObject *loadObj = item.GetAsset();
    if (loadObj == nullptr)
      continue;
    if (item.GetClass()->IsChildOf<USkeletalMesh>()) {
      // 确认时骨骼物体
      USkeletalMesh *skinObj = Cast<USkeletalMesh>(loadObj);
      UE_LOG(LogTemp, Log, TEXT("确认物体, 并进行转换 %s"), *(skinObj->GetPathName()));
      if (skinObj == nullptr)
        UE_LOG(LogTemp, Log, TEXT("不是骨骼物体 %s"), *(skinObj->GetPathName()));
#if ENGINE_MINOR_VERSION == 27
      for (auto &mat : skinObj->GetMaterials()) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface, mat.ImportedMaterialSlotName.ToString());
        }
      }
#else if ENGINE_MINOR_VERSION <= 26
      for (auto &mat : skinObj->Materials) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface, mat.ImportedMaterialSlotName.ToString());
        }
      }
#endif
    } else if (item.GetClass()->IsChildOf<UStaticMesh>()) {
      UStaticMesh *k_st = Cast<UStaticMesh>(loadObj);
      UE_LOG(LogTemp, Log, TEXT("确认物体, 并进行转换 %s"), *(k_st->GetPathName()));
      if (k_st == nullptr) {
        UE_LOG(LogTemp, Log, TEXT("不是静态网格体 %s"), *(k_st->GetPathName()));
        continue;
      }
#if ENGINE_MINOR_VERSION == 27
      for (auto &mat : k_st->GetStaticMaterials()) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface, mat.ImportedMaterialSlotName.ToString());
        }
      }

#else if ENGINE_MINOR_VERSION <= 26
      for (auto &mat : k_st->StaticMaterials) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface, mat.ImportedMaterialSlotName.ToString());
        }
      }
#endif
    } else {
      UE_LOG(LogTemp, Log, TEXT("不支持的类型"));
      continue;
    }
  }
  return FReply::Handled();
}

FReply DoodleCopyMat::set_marteral_deep() {
  FContentBrowserModule &contentBrowserModle =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser"
      );
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);

  for (auto &&item : selectedAss) {
    UObject *loadObj = item.GetAsset();
    if (loadObj == nullptr)
      continue;
    if (loadObj->GetClass()->IsChildOf<UParticleSystem>()) {
      auto *k_par = Cast<UParticleSystem>(loadObj);
      for (auto *k_em : k_par->Emitters) {
        for (auto *k_mat_i : k_em->MeshMaterials) {
          auto *k_mat = k_mat_i->GetMaterial();
          if (k_mat) {
            UE_LOG(LogTemp, Log, TEXT("开始设置材质景深后渲染属性 %s"), *k_mat->GetName());
            k_mat->bEnableSeparateTranslucency =
                this->bEnableSeparateTranslucency;
            k_mat->ForceRecompileForRendering();
          }
        }
        k_em->UpdateModuleLists();
        for (auto *k_mod : k_em->ModulesNeedingInstanceData) {
          if (k_mod->GetClass()->IsChildOf<UParticleModuleRequired>()) {
            auto k_mod_req = Cast<UParticleModuleRequired>(k_mod);
            auto *k_mat    = k_mod_req->Material->GetMaterial();
            if (k_mat) {
              UE_LOG(LogTemp, Log, TEXT("开始设置材质景深后渲染属性 %s"), *k_mat->GetName());
              k_mat->bEnableSeparateTranslucency =
                  this->bEnableSeparateTranslucency;
              k_mat->ForceRecompileForRendering();
            }
          }
        }
      }
      for (auto &k_mat_s : k_par->NamedMaterialSlots) {
        auto *k_mat = k_mat_s.Material->GetMaterial();
        if (k_mat) {
          UE_LOG(LogTemp, Log, TEXT("开始设置材质景深后渲染属性 %s"), *k_mat->GetName());
          k_mat->bEnableSeparateTranslucency =
              this->bEnableSeparateTranslucency;
          k_mat->ForceRecompileForRendering();
        }
      }
    }
  }

  return FReply::Handled();
}

TArray<FString> DoodleCopyMat::OpenFileDialog(const FString &DialogTitle, const FString &DefaultPath, const FString &FileTypes) {
  TArray<FString> OutFileNames;
  // FString OutDir;
  void *ParentWindowPtr = FSlateApplication::Get()
                              .GetActiveTopLevelWindow()
                              ->GetNativeWindow()
                              ->GetOSWindowHandle();
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
  if (DesktopPlatform) {
    uint32 SelectionFlag =
        1;  // A value of 0 represents single file selection while a value of 1
            // represents multiple file selection
    DesktopPlatform->OpenFileDialog(ParentWindowPtr, DialogTitle, DefaultPath, FString(""), FileTypes, SelectionFlag, OutFileNames);
    // DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle,
    //                                     DefaultPath, OutDir);
  }
  return OutFileNames;
}

FString DoodleCopyMat::OpenDirDialog(const FString &DialogTitle, const FString &DefaultPath) {
  FString OutDir;
  void *ParentWindowPtr = FSlateApplication::Get()
                              .GetActiveTopLevelWindow()
                              ->GetNativeWindow()
                              ->GetOSWindowHandle();
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
  if (DesktopPlatform) {
    DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle, DefaultPath, OutDir);
  }
  return OutDir;
}

void DoodleCopyMat::set_material_attr(UMaterialInterface *in_mat, const FString &in_SlotName) {
  IPlatformFile &FileManager = FPlatformFileManager::Get().GetPlatformFile();

  UE_LOG(LogTemp, Log, TEXT("确认材质插槽名称 %s"), *in_SlotName);
  auto old_path = in_mat->GetPathName();
  auto new_path =
      in_mat->GetPathName().Replace(*(in_mat->GetName()), *in_SlotName);
  if (old_path == new_path) {
    UE_LOG(LogTemp, Log, TEXT("材质新旧路径相同，不需要重命名"));
  }

  if (FileManager.FileExists(*new_path)) {
    if (old_path != new_path) {
      UE_LOG(LogTemp, Log, TEXT("新路径存在资产,无法重命名 %s"), *(new_path));
    } else {
      UE_LOG(LogTemp, Log, TEXT("材质新旧路径相同，不需要重命名"));
    }
  }

  if (old_path != new_path && !FileManager.FileExists(*new_path)) {
    UEditorAssetLibrary::RenameAsset(old_path, new_path);
    UE_LOG(LogTemp, Log, TEXT("重命名材质 路径 %s --> %s"), *(old_path), *(new_path));
  }

  if (in_mat->GetMaterial() != nullptr) {
    auto mat_m = in_mat->GetMaterial();
    bool l_tmp{true};
    mat_m->SetMaterialUsage(l_tmp, EMaterialUsage::MATUSAGE_GeometryCache);

    UEditorAssetLibrary::SaveAsset(mat_m->GetPathName());
    UE_LOG(LogTemp, Log, TEXT("使材料支持集合缓存 %s"), *(mat_m->GetPathName()));
  }
}
