#include "DoodleCopyMaterial.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "EditorAssetLibrary.h"
#include "GeometryCache.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

//批量导入
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
//重命名资产
#include "EditorAssetLibrary.h"
//保存包需要
#include "Editor.h"
//自定义abc导入
//#include "AbcWrap/DoodleAbcFactory.h"

void DoodleCopyMat::Construct(const FArguments &Arg) {
  //这个是ue界面的创建方法

  ChildSlot
      [SNew(SHorizontalBox) +
       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(1.f, 1.f))
               [SNew(SButton)                                    //创建按钮
                    .OnClicked(this, &DoodleCopyMat::getSelect)  //添加回调函数
                        [SNew(STextBlock)
                             .Text(FText::FromString(
                                 TEXT("获得选择物体")))  //按钮中的字符
  ]
                    .ToolTipText_Lambda([=]() -> FText {
                      return FText::FromString(TEXT("获得选中物体"));
                    })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(
               1.f,
               1.f))[SNew(SButton)  //创建按钮
                         .OnClicked(this,
                                    &DoodleCopyMat::CopyMateral)  //添加回调函数
                             [SNew(STextBlock)
                                  .Text(FText::FromString(
                                      TEXT("复制材质列表")))  //按钮中的字符
  ]
                         .ToolTipText_Lambda([=]() -> FText {
                           return FText::FromString(
                               TEXT("复制选中物体的材质列表"));
                         })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(1.f, 1.f))
               [SNew(SButton)
                    .OnClicked(this, &DoodleCopyMat::BathImport)  // 批量导入
                        [SNew(STextBlock)
                             .Text(FText::FromString(TEXT("批量导入")))]
                    .ToolTipText_Lambda([=]() -> FText {
                      return FText::FromString(TEXT("批量导入fbx和abc文件"));
                    })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(1.f, 1.f))
               [SNew(SButton)
                    .OnClicked(this, &DoodleCopyMat::BathReameAss)  //批量重命名
                        [SNew(STextBlock)
                             .Text(FText::FromString(TEXT("批量修改材质名称")))]
                    .ToolTipText_Lambda([=]() -> FText {
                      return FText::FromString(TEXT(
                          "选中骨骼物体,会将材料名称和骨骼物体的插槽名称统一"));
                    })] +

       SHorizontalBox::Slot()
           .AutoWidth()
           .HAlign(HAlign_Left)
           .Padding(FMargin(1.f, 1.f))
               [SNew(SButton)
                    .OnClicked(this, &DoodleCopyMat::importAbcFile)
                        [SNew(STextBlock)
                             .Text(FText::FromString(TEXT("alembic 导入")))]
                    .ToolTipText_Lambda([=]() -> FText {
                      return FText::FromString(
                          TEXT("自定义abc导入,带插槽, 有材质名称显示"));
                    })]];
}

void DoodleCopyMat::AddReferencedObjects(FReferenceCollector &collector) {
  // collector.AddReferencedObjects()
}

FReply DoodleCopyMat::getSelect() {
  /*
            获得文件管理器中的骨架网格物体的选择
            这是一个按钮的回调参数
            */

  //获得文件管理器的模块(或者类?)
  FContentBrowserModule &contentBrowserModle =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);
  for (int i = 0; i < selectedAss.Num(); i++) {
    // 测试选中物体是否是骨骼物体
    if (selectedAss[i].GetClass()->IsChildOf<USkeletalMesh>()) {
      //如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("确认骨骼物体 %s"),
             *(selectedAss[i].GetFullName()));

      UObject *skinObj = selectedAss[i].ToSoftObjectPath().TryLoad();
      // assLoad.LoadAsset(selectedAss[i].GetFullName( ));
      //将加载的类转换为skeletalMesh类并进行储存
      if (skinObj) {
        copySoureSkinObj = Cast<USkeletalMesh>(skinObj);
        UE_LOG(LogTemp, Log, TEXT("%s"), *(copySoureSkinObj->GetPathName()));
      }

    }  //测试是否是几何缓存物体
    else if (selectedAss[i].GetClass() == UGeometryCache::StaticClass()) {
      //如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("确认几何缓存  %s"),
             *(selectedAss[i].GetFullName()));
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
          "ContentBrowser");
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);
  for (int i = 0; i < selectedAss.Num(); i++) {
    UObject *loadObj =
        selectedAss[i]
            .ToSoftObjectPath()
            .TryLoad();  // assLoad.LoadAsset(selectedAss[i].GetFullName( ));

    // 测试选中物体是否是骨骼物体
    if (selectedAss[i].GetClass()->IsChildOf<USkeletalMesh>()) {
      //如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"),
             *(selectedAss[i].GetFullName()));

      USkeletalMesh *copyTrange = Cast<USkeletalMesh>(loadObj);

      UE_LOG(LogTemp, Log, TEXT("确认并加载为几何物体 %s"),
             *(copyTrange->GetPathName()));
      ENGINE_MINOR_VERSION;
#if ENGINE_MINOR_VERSION == 27
      TArray<FSkeletalMaterial> trangeMat = copyTrange->GetMaterials();
      if (copySoureSkinObj)
        for (int m = 0; m < trangeMat.Num(); m++) {
          trangeMat[m] = copySoureSkinObj->GetMaterials()[m];
          UE_LOG(LogTemp, Log, TEXT("%s"),
                 *(trangeMat[m].MaterialInterface->GetPathName()));
          //材质插槽命名
        }
      copyTrange->SetMaterials(trangeMat);
#else if ENGINE_MINOR_VERSION <= 26
      TArray<FSkeletalMaterial> trangeMat = copyTrange->Materials;
      if (copySoureSkinObj)
        for (int m = 0; m < trangeMat.Num(); m++) {
          trangeMat[m] = copySoureSkinObj->Materials[m];
          UE_LOG(LogTemp, Log, TEXT("%s"),
                 *(trangeMat[m].MaterialInterface->GetPathName()));
          //材质插槽命名
        }
      copyTrange->Materials = trangeMat;
#endif

    }  //如果是几何缓存就复制几何缓存
    else if (selectedAss[i].GetClass() == UGeometryCache::StaticClass()) {
      UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"),
             *(selectedAss[i].GetFullName()));

      UGeometryCache *copyTrange = Cast<UGeometryCache>(loadObj);
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
  FlushRenderingCommands();
  auto fileName =
      OpenFileDialog("Bath import abc file", "", " import files|*.abc;*.fbx");

  IAssetTools &assetTools =
      FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools")
          .Get();  //获得资产工具

  // FRegexPattern
  // myPattern_abc(TEXT(R"re(ep(\d+)_sc(\d+)_.*_.(\d+)-(\d+))re"));
  FRegexPattern myPattern_fbx(TEXT(R"re(ep(\d+)_sc(\d+)_.*_)re"));

  TArray<UAssetImportTask *> importTaskList;
  for (auto &name : fileName) {
    auto fnames = FPaths::GetBaseFilename(name);

    FString uePath;
    UE_LOG(LogTemp, Log, TEXT("文件名 %s"), *fnames);

    auto AssetImportTask = NewObject<UAssetImportTask>();
    AssetImportTask->bAutomated = true;
    AssetImportTask->bReplaceExisting = true;
    AssetImportTask->bReplaceExistingSettings = true;
    AssetImportTask->bSave = true;
    AssetImportTask->DestinationPath = "/Game/Shot";
    AssetImportTask->Filename = name;

    // if (FPaths::GetExtension(name) == "abc") {
    // 	FRegexMatcher myMatch(myPattern_abc, fnames);
    // 	auto abcImportTask = UAbcImportSettings::Get();
    // 	abcImportTask->ImportType = EAlembicImportType::GeometryCache;
    // 	abcImportTask->GeometryCacheSettings.bApplyConstantTopologyOptimizations
    // = 		true;
    // abcImportTask->GeometryCacheSettings.bFlattenTracks = true;
    // abcImportTask->GeometryCacheSettings.CompressedPositionPrecision = 0.01f;
    // abcImportTask->SamplingSettings.FrameStart = 1001;
    // 	abcImportTask->SamplingSettings.FrameEnd = 1200;
    // 	abcImportTask->SamplingSettings.bSkipEmpty = true;
    // 	abcImportTask->SamplingSettings.FrameSteps = 25.f;
    // 	abcImportTask->ConversionSettings.bFlipV = true;
    // 	abcImportTask->ConversionSettings.Preset = EAbcConversionPreset::Maya;
    // 	abcImportTask->ConversionSettings.Scale = FVector{ 1.f, -1.f, 1.f };
    // 	abcImportTask->ConversionSettings.Rotation = FVector{ 90.f, 0.f, 0.f };
    // 	abcImportTask->MaterialSettings.bFindMaterials = true;

    // 	if (myMatch.FindNext()) {
    // 		auto eps = FCString::Atoi(*myMatch.GetCaptureGroup(1));
    // 		auto shot = FCString::Atoi(*myMatch.GetCaptureGroup(2));
    // 		auto start_f = FCString::Atoi(*myMatch.GetCaptureGroup(3));
    // 		auto end_f = FCString::Atoi(*myMatch.GetCaptureGroup(4));

    // 		UE_LOG(LogTemp, Log, TEXT("集数 %s"),
    // *(myMatch.GetCaptureGroup(1))); 		UE_LOG(LogTemp, Log,
    // TEXT("镜头号 %s"),
    // *(myMatch.GetCaptureGroup(2))); 		UE_LOG(LogTemp, Log,
    // TEXT("开始帧 %s"),
    // *(myMatch.GetCaptureGroup(3))); 		UE_LOG(LogTemp, Log,
    // TEXT("结束帧 %s"),
    // *(myMatch.GetCaptureGroup(4))); 		uePath =
    // 			FString::Printf(TEXT("/Game/Shot/Ep%03i/Sc%04i/Ren"),
    // eps, shot); 		UE_LOG(LogTemp, Log, TEXT("导入路径 %s"),
    // *(uePath)); 		AssetImportTask->DestinationPath = uePath;
    // 		abcImportTask->SamplingSettings.FrameEnd = end_f;
    // 	}
    // 	AssetImportTask->Options = abcImportTask;
    // 	//
    // assetTools.ImportAssetTasks(TArray<UAssetImportTask*>{AssetImportTask});
    // }
    //	else
    if (FPaths::GetExtension(name) == "fbx") {
      FRegexMatcher myMatch(myPattern_fbx, fnames);
      // auto fbxImport = NewObject<UFbxImportUI>( );
      // fbxImport->bImportAnimations = true;
      // fbxImport->bImportAsSkeletal = true;
      // fbxImport->bImportMesh = true;
      auto fbxFactory = NewObject<UFbxFactory>();
      fbxFactory->ImportUI->bImportAnimations = true;
      fbxFactory->ImportUI->bImportAsSkeletal = false;
      fbxFactory->ImportUI->bImportMesh = false;
      fbxFactory->ImportUI->MeshTypeToImport = FBXIT_Animation;
      fbxFactory->ImportUI->OriginalImportType = FBXIT_Animation;
      // fbxFactory->ImportUI->SkeletalMeshImportData->bImportMorphTargets =
      // false;
      fbxFactory->ImportUI->bImportMaterials = false;
      fbxFactory->ImportUI->bImportTextures = false;
      fbxFactory->ImportUI->bAutomatedImportShouldDetectType = false;
      // fbxFactory->SetDetectImportTypeOnImport(false);
      auto skinObj = LoadObject<USkeleton>(
          nullptr,
          TEXT("/Game/"
               "shot_ep001_sc0009_Anm_fbx_v0001__miaoyu__Ch003A0_Skeleton.shot_"
               "ep001_sc0009_Anm_fbx_v0001__miaoyu__Ch003A0_Skeleton"),
          nullptr, LOAD_ResolvingDeferredExports);
      // auto skinMeshObj = LoadObject<USkeletalMesh>(
      // 	nullptr,
      // 	TEXT("/Game/shot_ep001_sc0009_Anm_fbx_v0001__miaoyu__Ch003A0.shot_ep001_sc0009_Anm_fbx_v0001__miaoyu__Ch003A0"));
      if (!skinObj) {
        return FReply::Handled();
        UE_LOG(LogTemp, Log, TEXT("无法加载骨骼"));
      } else {
        UE_LOG(LogTemp, Log, TEXT("加载骨骼"));
      }
      // fbxFactory->ImportUI->SkeletalMeshImportData =
      // 	UFbxSkeletalMeshImportData::GetImportDataForSkeletalMesh(skinMeshObj,
      // fbxFactory->ImportUI->SkeletalMeshImportData);

      fbxFactory->ImportUI->Skeleton = skinObj;
      AssetImportTask->Factory = fbxFactory;

      if (myMatch.FindNext()) {
        auto eps = FCString::Atoi(*myMatch.GetCaptureGroup(1));
        auto shot = FCString::Atoi(*myMatch.GetCaptureGroup(2));
        // auto start_f = FCString::Atoi(*myMatch.GetCaptureGroup(3));
        // auto end_f = FCString::Atoi(*myMatch.GetCaptureGroup(4));
        UE_LOG(LogTemp, Log, TEXT("集数 %s"), *(myMatch.GetCaptureGroup(1)));
        UE_LOG(LogTemp, Log, TEXT("镜头号 %s"), *(myMatch.GetCaptureGroup(2)));
        // UE_LOG(LogTemp, Log, TEXT("开始帧 %s"),
        // *(myMatch.GetCaptureGroup(3))); UE_LOG(LogTemp, Log, TEXT("结束帧
        // %s"), *(myMatch.GetCaptureGroup(4)));
        uePath =
            FString::Printf(TEXT("/Game/Shot/ep%03i/sc%04i/Ren"), eps, shot);
        UE_LOG(LogTemp, Log, TEXT("导入路径 %s"), *(uePath));
        AssetImportTask->DestinationPath = uePath;
      }

      AssetImportTask->Options = fbxFactory->ImportUI;
    }
    importTaskList.Add(AssetImportTask);
    UEditorAssetLibrary::SaveDirectory("/Game/shot");
  }
  assetTools.ImportAssetTasks(importTaskList);
  return FReply::Handled();
}

FReply DoodleCopyMat::BathReameAss() {
  FContentBrowserModule &contentBrowserModle =
      FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
          "ContentBrowser");
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get().GetSelectedAssets(selectedAss);

  for (auto &&item : selectedAss) {
    UObject *loadObj = item.GetAsset();
    if (loadObj == nullptr) continue;
    if (item.GetClass()->IsChildOf<USkeletalMesh>()) {
      //确认时骨骼物体
      USkeletalMesh *skinObj = Cast<USkeletalMesh>(loadObj);
      UE_LOG(LogTemp, Log, TEXT("确认物体, 并进行转换 %s"),
             *(skinObj->GetPathName()));
      if (skinObj == nullptr)
        UE_LOG(LogTemp, Log, TEXT("不是骨骼物体 %s"),
               *(skinObj->GetPathName()));
#if ENGINE_MINOR_VERSION == 27
      for (auto &mat : skinObj->GetMaterials()) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface,
                            mat.ImportedMaterialSlotName.ToString());
        }
      }
#else if ENGINE_MINOR_VERSION <= 26
      for (auto &mat : skinObj->Materials) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface,
                            mat.ImportedMaterialSlotName.ToString());
        }
      }
#endif
    } else if (item.GetClass()->IsChildOf<UStaticMesh>()) {
      UStaticMesh *k_st = Cast<UStaticMesh>(loadObj);
      UE_LOG(LogTemp, Log, TEXT("确认物体, 并进行转换 %s"),
             *(k_st->GetPathName()));
      if (k_st == nullptr) {
        UE_LOG(LogTemp, Log, TEXT("不是静态网格体 %s"), *(k_st->GetPathName()));
        continue;
      }
#if ENGINE_MINOR_VERSION == 27
      for (auto &mat : k_st->GetStaticMaterials()) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface,
                            mat.ImportedMaterialSlotName.ToString());
        }
      }
      
#else if ENGINE_MINOR_VERSION <= 26
      for (auto &mat : k_st->StaticMaterials) {
        if (mat.ImportedMaterialSlotName.IsValid()) {
          set_material_attr(mat.MaterialInterface,
                            mat.ImportedMaterialSlotName.ToString());
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

FReply DoodleCopyMat::importAbcFile() {
  // FlushRenderingCommands();
  // auto fileName = OpenFileDialog("Bath import abc file", "", " import files |
  // *.abc");
  ////解析文件名称
  // FRegexPattern
  // myPattern_abc(TEXT(R"re(ep(\d+)_sc(\d+)_.*_.(\d+)-(\d+))re"));
  ////FRegexPattern myPattern_fbx(TEXT(R"re(ep(\d+)_sc(\d+)_.*_)re"));
  // IAssetTools& assetTools =
  //	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();//获得资产工具
  // auto abcImportFactory = NewObject<UDoodleAbcFactory>();
  // auto AssetImportTask = NewObject<UAssetImportTask>();
  // TArray<UAssetImportTask*> importTaskList;
  // for (auto& name : fileName) {
  //	auto fnames = FPaths::GetBaseFilename(name);
  //	FRegexMatcher myMatch(myPattern_abc, fnames);
  //	FString uePath;
  //	UE_LOG(LogTemp, Log, TEXT("文件名 %s"), *fnames);
  //	AssetImportTask->bAutomated = true;
  //	AssetImportTask->bReplaceExisting = true;
  //	AssetImportTask->bReplaceExistingSettings = true;
  //	AssetImportTask->bSave = true;
  //	AssetImportTask->DestinationPath = "/Game/Shot";
  //	AssetImportTask->Filename = name;
  //	AssetImportTask->Factory = abcImportFactory;
  //	auto abcImportTask = UAbcImportSettings::Get();
  //	abcImportTask->ImportType = EAlembicImportType::GeometryCache;
  //	abcImportTask->GeometryCacheSettings.bApplyConstantTopologyOptimizations
  //		= true;
  // abcImportTask->GeometryCacheSettings.bFlattenTracks = true;
  // abcImportTask->GeometryCacheSettings.CompressedPositionPrecision
  // = 		0.01f; 	abcImportTask->SamplingSettings.FrameStart = 1001;
  //	abcImportTask->SamplingSettings.FrameEnd = 1200;
  //	abcImportTask->SamplingSettings.bSkipEmpty = true;
  //	abcImportTask->SamplingSettings.FrameSteps = 25.f;
  //	abcImportTask->ConversionSettings.bFlipV = true;
  //	abcImportTask->ConversionSettings.Preset = EAbcConversionPreset::Maya;
  //	abcImportTask->ConversionSettings.Scale = FVector{ 1.f, -1.f, 1.f };
  //	abcImportTask->ConversionSettings.Rotation = FVector{ 90.f, 0.f, 0.f };
  //	abcImportTask->MaterialSettings.bFindMaterials = true;
  //	if (myMatch.FindNext()) {
  //		auto eps = FCString::Atoi(*myMatch.GetCaptureGroup(1));
  //		auto shot = FCString::Atoi(*myMatch.GetCaptureGroup(2));
  //		auto start_f = FCString::Atoi(*myMatch.GetCaptureGroup(3));
  //		auto end_f = FCString::Atoi(*myMatch.GetCaptureGroup(4));
  //		UE_LOG(LogTemp, Log, TEXT("集数 %s"),
  //*(myMatch.GetCaptureGroup(1))); 		UE_LOG(LogTemp, Log,
  // TEXT("镜头号 %s"),
  //*(myMatch.GetCaptureGroup(2))); 		UE_LOG(LogTemp, Log,
  // TEXT("开始帧 %s"),
  //*(myMatch.GetCaptureGroup(3))); 		UE_LOG(LogTemp, Log,
  // TEXT("结束帧 %s"),
  //*(myMatch.GetCaptureGroup(4))); 		uePath =
  // FString::Printf(TEXT("/Game/Shot/ep%03i/sc%04i/Ren"), eps, shot);
  //		UE_LOG(LogTemp, Log, TEXT("导入路径 %s"), *(uePath));
  //		AssetImportTask->DestinationPath = uePath;
  //		abcImportTask->SamplingSettings.FrameEnd = end_f;
  //	}
  //	AssetImportTask->Options = abcImportTask;
  //	importTaskList.Add(AssetImportTask);
  //}
  // assetTools.ImportAssetTasks(importTaskList);

  return FReply::Handled();
}

TArray<FString> DoodleCopyMat::OpenFileDialog(const FString &DialogTitle,
                                              const FString &DefaultPath,
                                              const FString &FileTypes) {
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
    DesktopPlatform->OpenFileDialog(ParentWindowPtr, DialogTitle, DefaultPath,
                                    FString(""), FileTypes, SelectionFlag,
                                    OutFileNames);
    // DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle,
    //                                     DefaultPath, OutDir);
  }
  return OutFileNames;
}

FString DoodleCopyMat::OpenDirDialog(const FString &DialogTitle,
                                     const FString &DefaultPath) {
  FString OutDir;
  void *ParentWindowPtr = FSlateApplication::Get()
                              .GetActiveTopLevelWindow()
                              ->GetNativeWindow()
                              ->GetOSWindowHandle();
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
  if (DesktopPlatform) {
    DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle,
                                         DefaultPath, OutDir);
  }
  return OutDir;
}

void DoodleCopyMat::set_material_attr(UMaterialInterface *in_mat,
                                      const FString &in_SlotName) {
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
    UE_LOG(LogTemp, Log, TEXT("重命名材质 路径 %s --> %s"), *(old_path),
           *(new_path));
  }

  if (in_mat->GetMaterial() != nullptr) {
    auto mat_m = in_mat->GetMaterial();
    bool l_tmp{true};
    mat_m->SetMaterialUsage(l_tmp, EMaterialUsage::MATUSAGE_GeometryCache);

    UEditorAssetLibrary::SaveAsset(mat_m->GetPathName());
    UE_LOG(LogTemp, Log, TEXT("使材料支持集合缓存 %s"),
           *(mat_m->GetPathName()));
  }
}
