#include "doodleCopyMaterial.h"

#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "EditorAssetLibrary.h"
#include "GeometryCache.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

//批量导入
#include "AbcImportSettings.h"
#include "AbcImporter.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "IAssetTools.h"
#include "ObjectTools.h"
#include "Misc/Paths.h"
#include "HAL/FileManagerGeneric.h"
#include "Internationalization/Regex.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"

//重命名资产
#include "EditorAssetLibrary.h"
#include "doodleAbcFactory.h"
void DoodleCopyMat::Construct(const FArguments& Arg) {
  //这个是ue界面的创建方法

  ChildSlot
    [SNew(SHorizontalBox) +
    SHorizontalBox::Slot( )
    .AutoWidth( )
    .HAlign(HAlign_Left)
    .Padding(FMargin(1.f, 1.f))
    [SNew(SButton)                                    //创建按钮
    .OnClicked(this, &DoodleCopyMat::getSelect)  //添加回调函数
    [SNew(STextBlock)
    .Text(FText::FromString(
      "Get Select Obj"))  //按钮中的字符
    ]] +
    SHorizontalBox::Slot( )
    .AutoWidth( )
    .HAlign(HAlign_Left)
    .Padding(FMargin(
      1.f,
      1.f))[SNew(SButton)  //创建按钮
    .OnClicked(this,
               &DoodleCopyMat::CopyMateral)  //添加回调函数
    [SNew(STextBlock)
    .Text(FText::FromString(
      "copy To obj"))  //按钮中的字符
    ]] +
    SHorizontalBox::Slot( )
    .AutoWidth( )
    .HAlign(HAlign_Left)
    .Padding(FMargin(1.f, 1.f))
    [SNew(SButton).OnClicked(this, &DoodleCopyMat::BathImport)
    [SNew(STextBlock).Text(FText::FromString("bath import"))]] +
    SHorizontalBox::Slot( )
    .AutoWidth( )
    .HAlign(HAlign_Left)
    .Padding(FMargin(1.f, 1.f))
    [SNew(SButton).OnClicked(this, &DoodleCopyMat::BathReameAss)
    [SNew(STextBlock).Text(FText::FromString("bath rename"))]]];
}

void DoodleCopyMat::AddReferencedObjects(FReferenceCollector& collector) {
  // collector.AddReferencedObjects()
}

FReply DoodleCopyMat::getSelect( ) {
  /*
  获得文件管理器中的骨架网格物体的选择
  这是一个按钮的回调参数
  */

  //获得文件管理器的模块(或者类?)
  FContentBrowserModule& contentBrowserModle =
    FModuleManager::Get( ).LoadModuleChecked<FContentBrowserModule>(
      "ContentBrowser");
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get( ).GetSelectedAssets(selectedAss);
  for (int i = 0; i < selectedAss.Num( ); i++) {
    // 测试选中物体是否是骨骼物体
    if (selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( )) {
      //如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("确认骨骼物体 %s"),
             *(selectedAss[i].GetFullName( )));

      UObject* skinObj = selectedAss[i].ToSoftObjectPath( ).TryLoad( );
      // assLoad.LoadAsset(selectedAss[i].GetFullName( ));
      //将加载的类转换为skeletalMesh类并进行储存
      if (skinObj) {
        copySoureSkinObj = Cast<USkeletalMesh>(skinObj);
        UE_LOG(LogTemp, Log, TEXT("%s"), *(copySoureSkinObj->GetPathName( )));
      }
      // TArray<FSkeletalMaterial> SoureMat = copySoureSkinObj->Materials;
      // for (int m = 0; m < SoureMat.Num( ); m++)
      //{
      //    SoureMat[m].MaterialInterface->GetPathName( );
      //    UE_LOG(LogTemp, Log, TEXT("%s"),
      //    *(SoureMat[m].MaterialInterface->GetPathName( )));
      //}
      //
      // if (UClass *cl = loadObj->GetClass())
      //{
      //    if (UProperty *mproperty = cl->FindPropertyByName("materials"))
      //    {
      //        mproperty.
      //        UE_LOG(LogTemp, Log, TEXT("%s"), *(mproperty->GetName()));
      //    }
      //}
      // selectedAss[i].ToSoftObjectPath( ).TryLoad()
      // TFieldIterator<UProperty> iter(loadObj);
      // USkeletalMeshComponent test;
      // test.getmaterial
      // test.SetMaterial( );
      // UStaticMeshComponent test2;
      // test2.SetMaterial( );
    }  //测试是否是几何缓存物体
    else if (selectedAss[i].GetClass( ) == UGeometryCache::StaticClass( )) {
      //如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("确认几何缓存  %s"),
             *(selectedAss[i].GetFullName( )));
      UObject* cacheObj = selectedAss[i].ToSoftObjectPath( ).TryLoad( );
      if (cacheObj) {
        copySoureGeoCache = cacheObj;
        //*(cacheObj->GetFullName( )
        UE_LOG(LogTemp, Log, TEXT("%s"), *(cacheObj->GetFullName( )));
      }
    }
    // bool is =selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( );
    // UE_LOG(LogTemp, Log, TEXT("%s"), *(FString::FromInt(is)));
    // selectedAss[i].GetFullName( )
  }
  return FReply::Handled( );
}

FReply DoodleCopyMat::CopyMateral( ) {
  FContentBrowserModule& contentBrowserModle =
    FModuleManager::Get( ).LoadModuleChecked<FContentBrowserModule>(
      "ContentBrowser");
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get( ).GetSelectedAssets(selectedAss);
  for (int i = 0; i < selectedAss.Num( ); i++) {
    UObject* loadObj =
      selectedAss[i]
      .ToSoftObjectPath( )
      .TryLoad( );  // assLoad.LoadAsset(selectedAss[i].GetFullName( ));

// 测试选中物体是否是骨骼物体
    if (selectedAss[i].GetClass( )->IsChildOf<USkeletalMesh>( )) {
      //如果是骨骼物体就可以复制材质了
      UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"),
             *(selectedAss[i].GetFullName( )));

      USkeletalMesh* copyTrange = Cast<USkeletalMesh>(loadObj);

      UE_LOG(LogTemp, Log, TEXT("确认并加载为几何物体 %s"),
             *(copyTrange->GetPathName( )));
      TArray<FSkeletalMaterial> trangeMat = copyTrange->Materials;
      if (copySoureSkinObj)
        for (int m = 0; m < trangeMat.Num( ); m++) {
          trangeMat[m] = copySoureSkinObj->Materials[m];
          UE_LOG(LogTemp, Log, TEXT("%s"),
                 *(trangeMat[m].MaterialInterface->GetPathName( )));
          //材质插槽命名
        }
      copyTrange->Materials = trangeMat;

    }  //如果是几何缓存就复制几何缓存
    else if (selectedAss[i].GetClass( ) == UGeometryCache::StaticClass( )) {
      UE_LOG(LogTemp, Log, TEXT("开始复制材质 %s"),
             *(selectedAss[i].GetFullName( )));

      UGeometryCache* copyTrange = Cast<UGeometryCache>(loadObj);
      TArray<UMaterialInterface*> trange = copyTrange->Materials;

      if (copySoureGeoCache) {
        auto soure = Cast<UGeometryCache>(copySoureGeoCache);
        for (int m = 0; m < trange.Num( ); m++) {
          trange[m] = soure->Materials[m];
          UE_LOG(LogTemp, Log, TEXT("%s"), *(trange[m]->GetPathName( )));
        }
      }
      copyTrange->Materials = trange;
    }
  }
  return FReply::Handled( );
}

FReply DoodleCopyMat::BathImport( ) {
  FlushRenderingCommands( );
  auto fileName =
    OpenFileDialog("Bath import abc file", "", " import files|*.abc;*.fbx");
  // TScriptInterface<IAssetTools> test = UAssetToolsHelpers::GetAssetTools( );

  //auto dir = OpenDirDialog("Bath import abc file", "");//获得目录

  IAssetTools& assetTools =
    FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get( );//获得资产工具

  //auto path = FPaths::GetPath(fileName[0]);
  //UE_LOG(LogTemp, Log, TEXT("找到文件夹 %s"), *path)
  //auto &fileManges = FFileManagerGeneric::Get();
  //TArray<FString> filenames;
  //FString ex = "*.abc";
  //fileManges.FindFilesRecursive(filenames, *path, *ex, true, false);
  //for (auto& i : filenames)
  //{
  //  UE_LOG(LogTemp, Log, TEXT("找到文件 %s"), *i);
  //}
  FRegexPattern myPattern_abc(TEXT(R"re(ep(\d+)_sc(\d+)_.*_.(\d+)-(\d+))re"));
  FRegexPattern myPattern_fbx(TEXT(R"re(ep(\d+)_sc(\d+)_.*_)re"));

  for (auto& name : fileName) {
    auto fnames = FPaths::GetBaseFilename(name);

    FString uePath;
    UE_LOG(LogTemp, Log, TEXT("文件名 %s"), *fnames);

    auto AssetImportTask = NewObject<UAssetImportTask>( );
    AssetImportTask->bAutomated = true;
    AssetImportTask->bReplaceExisting = true;
    AssetImportTask->bReplaceExistingSettings = true;
    AssetImportTask->bSave = true;
    AssetImportTask->DestinationPath = "/Game/Shot";
    AssetImportTask->Filename = name;

    if (FPaths::GetExtension(name) == "abc") {
      FRegexMatcher myMatch(myPattern_abc, fnames);
      auto abcImportTask = UAbcImportSettings::Get( );
      abcImportTask->ImportType = EAlembicImportType::GeometryCache;
      abcImportTask->GeometryCacheSettings.bApplyConstantTopologyOptimizations =
        true;
      abcImportTask->GeometryCacheSettings.bFlattenTracks = true;
      abcImportTask->GeometryCacheSettings.CompressedPositionPrecision = 0.01f;
      abcImportTask->SamplingSettings.FrameStart = 1001;
      abcImportTask->SamplingSettings.FrameEnd = 1200;
      abcImportTask->SamplingSettings.bSkipEmpty = true;
      abcImportTask->SamplingSettings.FrameSteps = 25.f;
      abcImportTask->ConversionSettings.bFlipV = true;
      abcImportTask->ConversionSettings.Preset = EAbcConversionPreset::Maya;
      abcImportTask->ConversionSettings.Scale = FVector{ 1.f, -1.f, 1.f };
      abcImportTask->ConversionSettings.Rotation = FVector{ 90.f, 0.f, 0.f };
      abcImportTask->MaterialSettings.bFindMaterials = true;

      if (myMatch.FindNext( )) {
        auto eps = FCString::Atoi(*myMatch.GetCaptureGroup(1));
        auto shot = FCString::Atoi(*myMatch.GetCaptureGroup(2));
        auto start_f = FCString::Atoi(*myMatch.GetCaptureGroup(3));
        auto end_f = FCString::Atoi(*myMatch.GetCaptureGroup(4));


        UE_LOG(LogTemp, Log, TEXT("集数 %s"), *(myMatch.GetCaptureGroup(1)));
        UE_LOG(LogTemp, Log, TEXT("镜头号 %s"), *(myMatch.GetCaptureGroup(2)));
        UE_LOG(LogTemp, Log, TEXT("开始帧 %s"), *(myMatch.GetCaptureGroup(3)));
        UE_LOG(LogTemp, Log, TEXT("结束帧 %s"), *(myMatch.GetCaptureGroup(4)));
        uePath = FString::Printf(TEXT("/Game/Shot/Ep%03i/Sc%04i/Ren"), eps, shot);
        UE_LOG(LogTemp, Log, TEXT("导入路径 %s"), *(uePath));
        AssetImportTask->DestinationPath = uePath;
        abcImportTask->SamplingSettings.FrameEnd = end_f;
      }
      AssetImportTask->Options = abcImportTask;
      //assetTools.ImportAssetTasks(TArray<UAssetImportTask*>{AssetImportTask});
    }
    else if (FPaths::GetExtension(name) == "fbx")
    {
      FRegexMatcher myMatch(myPattern_fbx, fnames);
      //auto fbxImport = NewObject<UFbxImportUI>( );
      //fbxImport->bImportAnimations = true;
      //fbxImport->bImportAsSkeletal = true;
      //fbxImport->bImportMesh = true;
      auto fbxFactory = NewObject<UFbxFactory>( );
      fbxFactory->ImportUI->bImportAnimations = true;
      fbxFactory->ImportUI->bImportAsSkeletal = true;
      fbxFactory->ImportUI->bImportMesh = true;
      fbxFactory->ImportUI->MeshTypeToImport = FBXIT_SkeletalMesh;
      fbxFactory->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
      fbxFactory->ImportUI->bImportMaterials = false;
      fbxFactory->ImportUI->bImportTextures = false;
      AssetImportTask->Factory = fbxFactory;


      if (myMatch.FindNext( )) {
        auto eps = FCString::Atoi(*myMatch.GetCaptureGroup(1));
        auto shot = FCString::Atoi(*myMatch.GetCaptureGroup(2));
        //auto start_f = FCString::Atoi(*myMatch.GetCaptureGroup(3));
        //auto end_f = FCString::Atoi(*myMatch.GetCaptureGroup(4));
        UE_LOG(LogTemp, Log, TEXT("集数 %s"), *(myMatch.GetCaptureGroup(1)));
        UE_LOG(LogTemp, Log, TEXT("镜头号 %s"), *(myMatch.GetCaptureGroup(2)));
        //UE_LOG(LogTemp, Log, TEXT("开始帧 %s"), *(myMatch.GetCaptureGroup(3)));
        //UE_LOG(LogTemp, Log, TEXT("结束帧 %s"), *(myMatch.GetCaptureGroup(4)));
        uePath = FString::Printf(TEXT("/Game/Shot/Ep%03i/Sc%04i/Ren"), eps, shot);
        UE_LOG(LogTemp, Log, TEXT("导入路径 %s"), *(uePath));
        AssetImportTask->DestinationPath = uePath;
      }
    }
    assetTools.ImportAssetTasks(TArray<UAssetImportTask*>{AssetImportTask});
    UEditorAssetLibrary::SaveDirectory("/Game/Shot");
  }
  return FReply::Handled( );
}

FReply DoodleCopyMat::BathReameAss( ) {
  FContentBrowserModule& contentBrowserModle =
    FModuleManager::Get( ).LoadModuleChecked<FContentBrowserModule>(
      "ContentBrowser");
  TArray<FAssetData> selectedAss;
  contentBrowserModle.Get( ).GetSelectedAssets(selectedAss);

  for (auto&& item : selectedAss) {
    UObject* loadObj = item.ToSoftObjectPath( ).TryLoad( );
    if (loadObj == nullptr) continue;
    if (!item.GetClass( )->IsChildOf<USkeletalMesh>( )) continue;
    //确认时骨骼物体
    USkeletalMesh* skinObj = Cast<USkeletalMesh>(loadObj);
    UE_LOG(LogTemp, Log, TEXT("确认并加载骨骼物体 %s"),
           *(skinObj->GetPathName( )));
    if (skinObj == nullptr) continue;
    for (auto& mat : skinObj->Materials) {
      if (mat.ImportedMaterialSlotName.IsValid( )) {
        if (mat.MaterialInterface->GetMaterial( ) != nullptr) {
          mat.MaterialInterface->GetMaterial( )->bUsedWithGeometryCache = true;
          UE_LOG(LogTemp, Log, TEXT("使材料支持集合缓存 %s"),
                 *(mat.MaterialInterface->GetPathName( )));
        }
        UE_LOG(LogTemp, Log, TEXT("确认材质插槽名称 %s"),
               *(mat.ImportedMaterialSlotName.ToString( )));
        UEditorAssetLibrary::RenameAsset(
          mat.MaterialInterface->GetPathName( ),
          mat.MaterialInterface->GetPathName( ).Replace(
          *(mat.MaterialInterface->GetName( )),
          *(mat.ImportedMaterialSlotName.ToString( ))));
        UE_LOG(LogTemp, Log, TEXT("重命名材质 路径 %s  %s --> %s"),
               *(mat.MaterialInterface->GetPathName( )),
               *(mat.MaterialInterface->GetName( )),
               *(mat.ImportedMaterialSlotName.ToString( )));
      }
    }
  }

  return FReply::Handled( );
}

TArray<FString> DoodleCopyMat::OpenFileDialog(const FString& DialogTitle,
                                              const FString& DefaultPath,
                                              const FString& FileTypes) {
  TArray<FString> OutFileNames;
  //FString OutDir;
  void* ParentWindowPtr = FSlateApplication::Get( )
    .GetActiveTopLevelWindow( )
    ->GetNativeWindow( )
    ->GetOSWindowHandle( );
  IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get( );
  if (DesktopPlatform) {
    uint32 SelectionFlag =
      1;  // A value of 0 represents single file selection while a value of 1
          // represents multiple file selection
    DesktopPlatform->OpenFileDialog(ParentWindowPtr, DialogTitle, DefaultPath,
                                    FString(""), FileTypes, SelectionFlag,
                                    OutFileNames);
    //DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle,
    //                                     DefaultPath, OutDir);
  }
  return OutFileNames;
}

FString DoodleCopyMat::OpenDirDialog(const FString& DialogTitle,
                                     const FString& DefaultPath) {
  FString OutDir;
  void* ParentWindowPtr = FSlateApplication::Get( )
    .GetActiveTopLevelWindow( )
    ->GetNativeWindow( )
    ->GetOSWindowHandle( );
  IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get( );
  if (DesktopPlatform) {
    DesktopPlatform->OpenDirectoryDialog(ParentWindowPtr, DialogTitle,
                                         DefaultPath, OutDir);
  }
  return OutDir;
}
