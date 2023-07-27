#include "DoodleImportFbxUI.h"

#include "Widgets/SCanvas.h"
// 目录选择器
#include "Widgets/Input/SDirectoryPicker.h"
// 文件选择器
#include "AssetRegistry/IAssetRegistry.h"
#include "Widgets/Input/SFilePathPicker.h"
// 我们自己的多路径文件选择器
#include "Doodle/FilePathsPicker.h"
// 组合框
#include "Components/ComboBoxString.h"
// fbx读取需要
#include "FbxImporter.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "fbxsdk/scene/animation/fbxanimcurve.h"
#include "fbxsdk/scene/animation/fbxanimlayer.h"
#include "fbxsdk/scene/animation/fbxanimstack.h"
#include "fbxsdk/scene/geometry/fbxcamera.h"
#include "fbxsdk/scene/geometry/fbxcameraswitcher.h"
#include "fbxsdk/scene/geometry/fbxnode.h"

// 读写文件
#include "Misc/FileHelper.h"
// 元数据
#include "UObject/MetaData.h"
// 算法
#include "Algo/AllOf.h"
/// 自动导入类需要
#include "AssetImportTask.h"

/// 正则
#include "Internationalization/Regex.h"
/// 一般的导入任务设置
#include "AssetImportTask.h"
/// 导入模块
#include "AssetToolsModule.h"
/// 导入fbx需要
#include "Animation/AnimBoneCompressionSettings.h"  // 压缩骨骼设置
#include "Factories/Factory.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxTextureImportData.h"
#include "Factories/ImportSettings.h"
#include "Factories/MaterialImportHelpers.h"
/// 进度框
#include "Misc/ScopedSlowTask.h"
/// 属性按钮
#include "PropertyCustomizationHelpers.h"
/// 内容游览器模块
#include "ContentBrowserModule.h"
/// 内容游览器
#include "IContentBrowserSingleton.h"
/// 导入abc
#include "AbcImportSettings.h"
/// 编辑器笔刷效果
#include "EditorStyleSet.h"

/// 导入相机需要的头文件
#include "Camera/CameraComponent.h"  // 相机组件
#include "CineCameraActor.h"         // 相机
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequence.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneToolsUserSettings.h"          // 导入相机设置
#include "Sections/MovieSceneCameraCutSection.h"  // 相机剪切
#include "SequencerUtilities.h"                   // 创建相机

// 创建world
#include "AssetToolsModule.h"
#include "EditorLevelLibrary.h"
#include "Factories/WorldFactory.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "LevelEditorSubsystem.h"
#include "LevelSequence.h"
#include "Modules/ModuleManager.h"

// 导入abc
#include "AlembicImportFactory.h"
#include "Framework/Notifications/NotificationManager.h"  //通知管理类
#include "LevelEditorViewport.h"                          //编辑器视口
#include "Tracks/MovieSceneCameraCutTrack.h"              //处理对电影场景中CameraCut属性的操作。
#include "TransformData.h"                            //存储关于转换的信息，以便向转换部分添加键。
#include "Widgets/Notifications/SNotificationList.h"  // 编辑器通知

// 自定义导入abc
#include "Doodle/Abc/DoodleAbcImportSettings.h"
#include "Doodle/Abc/DoodleAlembicImportFactory.h"

#define LOCTEXT_NAMESPACE "SDoodleImportFbxUI"
const FName SDoodleImportFbxUI::Name{TEXT("DoodleImportFbxUI")};

namespace {

FString MakeName(const ANSICHAR* Name) {
  const TCHAR SpecialChars[]    = {TEXT('.'), TEXT(','), TEXT('/'), TEXT('`'), TEXT('%')};

  FString TmpName               = FString{ANSI_TO_TCHAR(Name)};

  // Remove namespaces
  int32 LastNamespaceTokenIndex = INDEX_NONE;
  if (TmpName.FindLastChar(TEXT(':'), LastNamespaceTokenIndex)) {
    const bool bAllowShrinking = true;
    //+1 to remove the ':' character we found
    TmpName.RightChopInline(LastNamespaceTokenIndex + 1, bAllowShrinking);
  }

  // Remove the special chars
  for (int32 i = 0; i < UE_ARRAY_COUNT(SpecialChars); i++) {
    TmpName.ReplaceCharInline(SpecialChars[i], TEXT('_'), ESearchCase::CaseSensitive);
  }

  return FSkeletalMeshImportData::FixupBoneName(TmpName);
}

FString GetNamepace(const ANSICHAR* Name) {
  FString TmpName               = FString{ANSI_TO_TCHAR(Name)};
  // Remove namespaces
  int32 LastNamespaceTokenIndex = INDEX_NONE;
  const bool bAllowShrinking    = true;
  if (TmpName.FindLastChar(TEXT(':'), LastNamespaceTokenIndex)) {
    //+1 to remove the ':' character we found
    TmpName.LeftChopInline(TmpName.Len() - LastNamespaceTokenIndex, bAllowShrinking);
  } else {
    return {};
  }
  LastNamespaceTokenIndex = INDEX_NONE;
  if (TmpName.FindLastChar(TEXT(':'), LastNamespaceTokenIndex)) {
    //+1 to remove the ':' character we found
    TmpName.RightChopInline(LastNamespaceTokenIndex + 1, bAllowShrinking);
  }
  return FSkeletalMeshImportData::FixupBoneName(TmpName);
}

void FindSkeletonNode(fbxsdk::FbxNode* Parent, TArray<fbxsdk::FbxNode*>& In_Skeketon) {
  if (Parent &&
      ((Parent->GetMesh() && Parent->GetMesh()->GetDeformerCount(fbxsdk::FbxDeformer::EDeformerType::eSkin) > 0) ||
       (Parent->GetNodeAttribute() &&
        (Parent->GetNodeAttribute()->GetAttributeType() == fbxsdk::FbxNodeAttribute::eSkeleton ||
         Parent->GetNodeAttribute()->GetAttributeType() == fbxsdk::FbxNodeAttribute::eNull)))) {
    In_Skeketon.Add(Parent);
  }

  int32 NodeCount = Parent->GetChildCount();
  for (int32 NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex) {
    fbxsdk::FbxNode* Child = Parent->GetChild(NodeIndex);
    FindSkeletonNode(Child, In_Skeketon);
  }
}

void Debug_To_File(const FStringView& In_String) {
  FString LFile_Path = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("Doodle"));
  // Always first check if the file that you want to manipulate exist.
  if (FFileHelper::SaveStringToFile(In_String, *LFile_Path)) {
    UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: to the text file"));
  } else {
    UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Failed to write FString to file."));
  }
}

}  // namespace

FString UDoodleBaseImportData::GetImportPath(const FString& In_Path_Prefix) {
  FRegexPattern L_Reg_Ep_Pattern{LR"([ep|EP|Ep]_?(\d+))"};
  FRegexMatcher L_Reg_Ep{L_Reg_Ep_Pattern, ImportPath};

  if (L_Reg_Ep.FindNext()) {
    Eps = FCString::Atoi64(*L_Reg_Ep.GetCaptureGroup(1));
  }

  FRegexPattern L_Reg_ScPattern{LR"([sc|SC|Sc]_?(\d+)([a-z])?)"};
  FRegexMatcher L_Reg_Sc{L_Reg_ScPattern, ImportPath};

  if (L_Reg_Sc.FindNext()) {
    Shot = FCString::Atoi64(*L_Reg_Sc.GetCaptureGroup(1));
    if (L_Reg_Sc.GetEndLimit() > 2) {
      ShotAb = L_Reg_Sc.GetCaptureGroup(2);
    }
  }

  FString L_Path = FString::Printf(TEXT("/Game/Shot/ep%.4d/%s%.4d_sc%.4d%s"), Eps, *In_Path_Prefix, Eps, Shot, *ShotAb);
  return L_Path;
}

void UDoodleBaseImportData::GenStartAndEndTime() {
  FRegexPattern L_Reg_Time_Pattern{LR"(_(\d+)-(\d+))"};
  FRegexMatcher L_Reg_Time{L_Reg_Time_Pattern, FPaths::GetBaseFilename(ImportPath)};
  StartTime = {1000};
  EndTime   = {1001};

  if (L_Reg_Time.FindNext() && L_Reg_Time.GetEndLimit() > 2) {
    StartTime = FCString::Atoi64(*L_Reg_Time.GetCaptureGroup(1));
    EndTime   = FCString::Atoi64(*L_Reg_Time.GetCaptureGroup(2));
  }
}

FString UDoodleBaseImportData::GetPathPrefix(const FString& In_Path) {
  FString L_Prefix{};
  int32 L_Index      = INDEX_NONE;
  FString L_FileName = FPaths::GetBaseFilename(In_Path);
  if (L_FileName.FindChar('_', L_Index)) {
    L_FileName.LeftChopInline(L_FileName.Len() - L_Index, true);
    L_Prefix = L_FileName;
  }
  return L_Prefix;
}

void UDoodleFbxImport_1::GenPathPrefix(const FString& In_Path_Prefix, const FString& In_Path_Suffix) {
  GenStartAndEndTime();
  FString L_String = FString::Format(
      TEXT("FbxI_{0}_{1}"),
      TArray<FStringFormatArg>{
          FStringFormatArg{In_Path_Suffix}, FStringFormatArg{FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))}}
  );
  ImportPathDir = GetImportPath(In_Path_Prefix) / L_String;
}

void UDoodleFbxImport_1::ImportFile() {
  UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
  L_Data->GroupName                 = TEXT("doodle import");
  L_Data->Filenames.Add(ImportPath);
  L_Data->DestinationPath                                        = ImportPathDir;
  L_Data->bReplaceExisting                                       = true;
  L_Data->bSkipReadOnly                                          = true;
  L_Data->bReplaceExisting                                       = true;

  UFbxFactory* k_fbx_f                                           = NewObject<UFbxFactory>(L_Data);
  L_Data->Factory                                                = k_fbx_f;

  k_fbx_f->ImportUI->MeshTypeToImport                            = FBXIT_SkeletalMesh;
  k_fbx_f->ImportUI->OriginalImportType                          = FBXIT_SkeletalMesh;
  k_fbx_f->ImportUI->bImportAsSkeletal                           = true;
  k_fbx_f->ImportUI->bCreatePhysicsAsset                         = false;
  k_fbx_f->ImportUI->bImportMesh                                 = true;
  k_fbx_f->ImportUI->bImportAnimations                           = true;
  k_fbx_f->ImportUI->bImportRigidMesh                            = true;
  k_fbx_f->ImportUI->bImportMaterials                            = false;
  k_fbx_f->ImportUI->bImportTextures                             = false;
  k_fbx_f->ImportUI->bResetToFbxOnMaterialConflict               = false;

  k_fbx_f->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
  k_fbx_f->ImportUI->bAutomatedImportShouldDetectType            = false;
  k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength     = FBXALIT_ExportedTime;
  k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks   = true;
  k_fbx_f->ImportUI->bAllowContentTypeImport                     = true;
  k_fbx_f->ImportUI->TextureImportData->MaterialSearchLocation   = EMaterialSearchLocation::UnderRoot;
  if (SkinObj) {
    if (OnlyAnim) {
      k_fbx_f->ImportUI->Skeleton         = SkinObj;
      k_fbx_f->ImportUI->MeshTypeToImport = FBXIT_Animation;
      k_fbx_f->ImportUI->bImportMesh      = false;
    } else {
      k_fbx_f->ImportUI->Skeleton          = SkinObj;
      k_fbx_f->ImportUI->MeshTypeToImport  = FBXIT_SkeletalMesh;
      k_fbx_f->ImportUI->bImportAsSkeletal = true;
      k_fbx_f->ImportUI->bImportMesh       = true;
    }
  }
  FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
  TArray<UObject*> L_Objs             = AssetToolsModule.Get().ImportAssetsAutomated(L_Data);

  FARFilter LFilter{};
  LFilter.bIncludeOnlyOnDiskAssets = false;
  LFilter.bRecursivePaths          = true;
  LFilter.bRecursiveClasses        = true;
  LFilter.PackagePaths.Add(FName{ImportPathDir});
  LFilter.ClassPaths.Add(UAnimSequence::StaticClass()->GetClassPathName());

  IAssetRegistry::Get()->EnumerateAssets(LFilter, [this](const FAssetData& InAss) -> bool {
    UAnimSequence* L_Anim = Cast<UAnimSequence>(InAss.GetAsset());
    if (L_Anim) {
      L_Anim->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(
          L_Anim, TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression")
      );
      /// 这里无效代码, 防止崩溃
      TArray<UObject*> LL{};
      L_Anim->GetPreloadDependencies(LL);
    }
    return true;
  });

  // for (UObject* L_Obj : L_Objs) {
  //   if (UAnimSequence* L_Seq = Cast<UAnimSequence>(L_Obj)) {
  //     L_Seq->BoneCompressionSettings = LoadObject<UAnimBoneCompressionSettings>(L_Seq,
  //     TEXT("/Engine/Animation/DefaultRecorderBoneCompression.DefaultRecorderBoneCompression"));
  //   }
  // }
}

bool UDoodleFbxImport_1::FindSkeleton(const TArray<FDoodleUSkeletonData_1> In_Skeleton) {
  TArray<fbxsdk::FbxNode*> L_Fbx_Node_list{};
  FString L_NameSpace{};
  auto* L_ImportFbx = UnFbx::FFbxImporter::GetInstance();
  L_ImportFbx->ClearAllCaches();
  L_ImportFbx->ImportFromFile(*ImportPath, FPaths::GetExtension(ImportPath));
  FScopedSlowTask L_Task_Scoped2{
      (float_t)L_ImportFbx->Scene->GetNodeCount() * 2, LOCTEXT("DoingSlowWork2", "扫描 fbx 文件骨骼中...")};

  TSet<FString> L_NodeNameSet{};

  for (size_t i = 0; i < L_ImportFbx->Scene->GetNodeCount(); ++i) {
    auto L_FbxNode = L_ImportFbx->Scene->GetNode(i);
    auto L_Attr    = L_FbxNode->GetNodeAttribute();
    // 只添加骨骼
    if (L_Attr && L_Attr->GetAttributeType() == fbxsdk::FbxNodeAttribute::eSkeleton) {
      FString L_Name = MakeName(L_ImportFbx->Scene->GetNode(i)->GetName());
      L_NodeNameSet.Add(L_Name);
    }
    // 获取名称空间
    if (L_NameSpace.IsEmpty()) L_NameSpace = GetNamepace(L_ImportFbx->Scene->GetNode(i)->GetName());

    L_Task_Scoped2.EnterProgressFrame(1.0f);
  }
  if (L_NameSpace.IsEmpty()) {
    return false;
  }

  for (auto&& L_SK_Data : In_Skeleton) {
    L_Task_Scoped2.EnterProgressFrame(1.0f);
    FString L_BaseName = FPaths::GetBaseFilename(ImportPath);
    if (!L_SK_Data.SkinTag.IsEmpty() && L_BaseName.Find(L_SK_Data.SkinTag) != INDEX_NONE) {
      SkinObj = L_SK_Data.SkinObj;
      return true;
    }
  }
  for (auto&& L_SK_Data : In_Skeleton) {
    L_Task_Scoped2.EnterProgressFrame(1.0f);
    if (Algo::AllOf(
            L_SK_Data.BoneNames, [&](const FString& IN_Str) { return L_NodeNameSet.Contains(IN_Str); }
        )  /// 进一步确认骨骼内容
    ) {
      SkinObj = L_SK_Data.SkinObj;
      return true;
    }
  }
  return false;
}

void UDoodleFbxCameraImport_1::GenPathPrefix(const FString& In_Path_Prefix, const FString& In_Path_Suffix) {
  FString L_Folder = GetImportPath(In_Path_Prefix);
  ImportPathDir    = L_Folder / FPaths::GetBaseFilename(L_Folder) + "_" + In_Path_Suffix;
}

void UDoodleFbxCameraImport_1::ImportFile() {
  const FFrameRate L_Rate{25, 1};
  const FFrameNumber L_Start{1001};
  FFrameNumber L_End{1200};

  FScopedSlowTask L_Task_Scoped{6.0f, LOCTEXT("Import_CAm", "导入camera")};
  L_Task_Scoped.MakeDialog();

  auto& L_AssTool = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

  L_Task_Scoped.EnterProgressFrame(
      1, FText::Format(
             LOCTEXT("Import_ImportingCameraFile1", "导入 \"{0}\"..."),
             FText::FromString(FPaths::GetBaseFilename(ImportPath))
         )
  );

  ULevelSequence* L_ShotSequence                        = LoadObject<ULevelSequence>(nullptr, *ImportPathDir);

  // 打开fbx
  UMovieSceneUserImportFBXSettings* L_ImportFBXSettings = GetMutableDefault<UMovieSceneUserImportFBXSettings>();
  FFBXInOutParameters InOutParams;
  // 修改一下设置
  L_ImportFBXSettings->bMatchByNameOnly       = false;
  L_ImportFBXSettings->bCreateCameras         = false;
  L_ImportFBXSettings->bReplaceTransformTrack = true;
  L_ImportFBXSettings->bReduceKeys            = false;
  // 这里使用包装导入
  if (!MovieSceneToolHelpers::ReadyFBXForImport(ImportPath, L_ImportFBXSettings, InOutParams)) {
    return;
  }

  // 已经打开的fbx, 直接获取, 是一个单例
  UnFbx::FFbxImporter* L_FbxImporter = UnFbx::FFbxImporter::GetInstance();
  L_FbxImporter->ImportFromFile(*ImportPath, FPaths::GetExtension(ImportPath));
  fbxsdk::FbxTimeSpan L_Fbx_Time     = L_FbxImporter->GetAnimationTimeSpan(
      L_FbxImporter->Scene->GetRootNode(), L_FbxImporter->Scene->GetCurrentAnimationStack()
  );
  // 获取结束帧
  L_End = (int32)L_Fbx_Time.GetStop().GetFrameCount(fbxsdk::FbxTime::ePAL);
  UE_LOG(LogTemp, Log, TEXT("fbx time %d -> %d"), L_Start.Value, L_End.Value);

  L_Task_Scoped.EnterProgressFrame(
      1,
      FText::Format(LOCTEXT("Import_ImportingCameraFile2", "检查定序器 \"{0}\"..."), FText::FromString(ImportPathDir))
  );

  // 创建定序器
  if (!L_ShotSequence) {
    for (TObjectIterator<UClass> it{}; it; ++it) {
      if (it->IsChildOf(UFactory::StaticClass())) {
        if (it->GetName() == "LevelSequenceFactoryNew") {
          L_ShotSequence = CastChecked<ULevelSequence>(L_AssTool.CreateAsset(
              FPaths::GetBaseFilename(ImportPathDir), FPaths::GetPath(ImportPathDir), ULevelSequence::StaticClass(),
              it->GetDefaultObject<UFactory>()
          ));
        }
      }
    }
  }
  L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile3", "设置定序器以及相机 ..."));

  // 设置定序器属性
  L_ShotSequence->GetMovieScene()->SetDisplayRate(L_Rate);
  L_ShotSequence->GetMovieScene()->SetTickResolutionDirectly(L_Rate);
  L_ShotSequence->GetMovieScene()->Modify();

  /// 设置范围
  L_ShotSequence->GetMovieScene()->SetWorkingRange((L_Start - 30) / L_Rate, (L_End + 30) / L_Rate);
  L_ShotSequence->GetMovieScene()->SetViewRange((L_Start - 30) / L_Rate, (L_End + 30) / L_Rate);
  L_ShotSequence->GetMovieScene()->SetPlaybackRange(TRange<FFrameNumber>{L_Start, L_End}, true);
  L_ShotSequence->Modify();

  // FSoftObjectPath L_LevelSequenceSoftPath{ImportPathDir};
  // UObject* L_LoadedObject                       = L_LevelSequenceSoftPath.TryLoad();
  UAssetEditorSubsystem* L_AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
  L_AssetEditorSubsystem->OpenEditorForAsset(L_ShotSequence);

  L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile4", "尝试打开定序器 ..."));

  IAssetEditorInstance* L_AssetEditor                = L_AssetEditorSubsystem->FindEditorForAsset(L_ShotSequence, true);

  ILevelSequenceEditorToolkit* L_LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(L_AssetEditor);
  ISequencer* L_ShotSequencer                        = L_LevelSequenceEditor->GetSequencer().Get();
  UMovieScene* L_Move                                = L_ShotSequence->GetMovieScene();
  ACineCameraActor* L_CameraActor{};
  // 相机task
  UMovieSceneTrack* L_Task = L_ShotSequence->GetMovieScene()->GetCameraCutTrack();
  if (!L_Task)
    // 添加相机时以及强制评估了, 不需要再强制评估
    FSequencerUtilities::CreateCamera(L_ShotSequencer->AsShared(), true, L_CameraActor);
  else
    // 强制评估序列, 要不然相机指针会空
    L_ShotSequencer->ForceEvaluate();
  L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile5", "刷新 ..."));

  L_Task = L_ShotSequence->GetMovieScene()->GetCameraCutTrack();

  // Cast<FStructProperty>(L_Task->GetClass()->FindPropertyByName("CameraBindingID"))->;
  // 寻找相机组件
  UCameraComponent* L_Cam{};
  for (auto&& L_Section : L_Task->GetAllSections()) {
    L_Cam = Cast<UMovieSceneCameraCutSection>(L_Section)->GetFirstCamera(*L_ShotSequencer, MovieSceneSequenceID::Root);
    if (L_Cam) break;
  }

  if (!L_CameraActor) L_CameraActor = Cast<ACineCameraActor>(L_Cam->GetOwner());

  FString L_CamLable = L_CameraActor->GetActorNameOrLabel();

  UE_LOG(LogTemp, Log, TEXT("camera name %s"), *L_CamLable);
  // 寻找相机id
  FGuid L_CamGuid = L_ShotSequencer->FindObjectId(*L_CameraActor, MovieSceneSequenceID::Root);

  UE_LOG(LogTemp, Log, TEXT("guid %s"), *L_CamGuid.ToString());

  TMap<FGuid, FString> L_Map{};
  L_Map.Add(L_CamGuid, L_CamLable);

  L_Task_Scoped.EnterProgressFrame(1, LOCTEXT("Import_ImportingCameraFile6", "开始导入帧 ..."));

  // 正式开始导入
  MovieSceneToolHelpers::ImportFBXCameraToExisting(
      L_FbxImporter, L_ShotSequence, L_ShotSequencer, L_ShotSequencer->GetFocusedTemplateID(), L_Map, false, true
  );

  UWorld* World = Cast<UWorld>(L_ShotSequencer->GetPlaybackContext());
  bool bValid   = MovieSceneToolHelpers::ImportFBXIfReady(
      World, L_ShotSequence, L_ShotSequencer, L_ShotSequencer->GetFocusedTemplateID(), L_Map, L_ImportFBXSettings,
      InOutParams
  );

  L_ShotSequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}

void UDoodleAbcImport_1::GenPathPrefix(const FString& In_Path_Prefix, const FString& In_Path_Suffix) {
  FString L_String = FString::Format(
      TEXT("AbcI_{0}_{1}"),
      TArray<FStringFormatArg>{
          FStringFormatArg{In_Path_Suffix}, FStringFormatArg{FDateTime::Now().ToString(TEXT("%m_%d_%H_%M"))}}
  );
  ImportPathDir = GetImportPath(In_Path_Prefix) / L_String;
}

void UDoodleAbcImport_1::ImportFile() {
  UAutomatedAssetImportData* L_Data = NewObject<UAutomatedAssetImportData>();
  L_Data->GroupName                 = TEXT("doodle import");
  L_Data->Filenames.Add(ImportPath);
  L_Data->DestinationPath                     = ImportPathDir;
  L_Data->bReplaceExisting                    = true;
  L_Data->bSkipReadOnly                       = true;
  L_Data->bReplaceExisting                    = true;

  UDoodleAbcImportFactory* k_abc_f            = NewObject<UDoodleAbcImportFactory>(L_Data);
  L_Data->Factory                             = k_abc_f;
  UDoodleAbcImportSettings* k_abc_stting      = NewObject<UDoodleAbcImportSettings>(L_Data);
  k_abc_f->ImportSettings                     = k_abc_stting;

  k_abc_stting->ImportType                    = EDoodleAlembicImportType::GeometryCache;  // 导入为几何缓存
  k_abc_stting->ConversionSettings.bFlipV     = true;
  k_abc_stting->ConversionSettings.Scale.X    = 1.0;
  k_abc_stting->ConversionSettings.Scale.Y    = -1.0;
  k_abc_stting->ConversionSettings.Scale.Z    = 1.0;
  k_abc_stting->ConversionSettings.Rotation.X = 90.0;
  k_abc_stting->ConversionSettings.Rotation.Y = 0.0;
  k_abc_stting->ConversionSettings.Rotation.Z = 0.0;

  k_abc_stting->GeometryCacheSettings.bFlattenTracks = true;       // 合并轨道
  k_abc_stting->SamplingSettings.bSkipEmpty          = true;       // 跳过空白帧
  k_abc_stting->SamplingSettings.FrameStart          = StartTime;  // 开始帧
  k_abc_stting->SamplingSettings.FrameEnd            = EndTime;    // 结束帧
  k_abc_stting->SamplingSettings.FrameSteps          = 1;          // 帧步数

  FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

  AssetToolsModule.Get().ImportAssetsAutomated(L_Data);
}

class SDoodleImportUiItem : public SMultiColumnTableRow<SDoodleImportFbxUI::UDoodleBaseImportDataPtrType> {
 public:
  SLATE_BEGIN_ARGS(SDoodleImportUiItem) : _ItemShow() {}

  SLATE_ARGUMENT(SDoodleImportFbxUI::UDoodleBaseImportDataPtrType, ItemShow)

  SLATE_END_ARGS()

 public:
  void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) {
    ItemShow = InArgs._ItemShow;
    ItemType = TEXT("onoe");
    if (auto L_FBX = Cast<UDoodleFbxImport_1>(InArgs._ItemShow)) {
      ItemShowFBX = L_FBX;
    } else if (Cast<UDoodleFbxCameraImport_1>(InArgs._ItemShow)) {
      ItemType = TEXT("导入的相机");
    } else if (Cast<UDoodleAbcImport_1>(InArgs._ItemShow)) {
      ItemType = TEXT("abc文件");
    }

    FSuperRowType::Construct(FSuperRowType::FArguments().Padding(0), InOwnerTableView);
  }

 public:  // override SMultiColumnTableRow
  virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override {
    // SHeaderRow::Column(TEXT("Import_File"))
    // SHeaderRow::Column(TEXT("Ep_And_Shot"))
    // SHeaderRow::Column(TEXT("Time_Ranges"))
    // SHeaderRow::Column(TEXT("Skeleton_Path"))
    // SHeaderRow::Column(TEXT("Import_Path_Dir"))

    if (ColumnName == TEXT("Import_File"))  // 判断列名为Fbx File，次名称在创建View时，通过SHeaderRow::Column指定
    {
      return SNew(STextBlock).Text(FText::FromString(ItemShow->ImportPath));
    } else if (ColumnName == TEXT("Time_Ranges")) {
      // clang-format off
			return SNew(STextBlock)
				.Text_Lambda([this]() -> FText {
				return FText::FromString(FString::Printf(TEXT("%d - %d"), ItemShow->StartTime, ItemShow->EndTime));
					})
          // clang-format on
          ;
    } else if (ColumnName == TEXT("Import_Path_Dir")) {
      return SNew(STextBlock).Text(FText::FromString(ItemShow->ImportPathDir));
    } else if (ColumnName == TEXT("Ep_And_Shot")) {
      return SNew(STextBlock)
          .Text(FText::FromString(
              FString::Printf(TEXT("EP: %d SC: %d%s"), ItemShow->Eps, ItemShow->Shot, *ItemShow->ShotAb)
          ));

    }

    else if (ColumnName == TEXT("Skeleton_Path") && ItemShowFBX) {
      // clang-format off
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(1.f)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() -> FText {
				return FText::FromString(FString::Printf(TEXT("%s"), *(ItemShowFBX->SkinObj != nullptr ?
					ItemShowFBX->SkinObj->GetPackage()->GetPathName() : FString{ TEXT("") })));
						})
				]
			+ SHorizontalBox::Slot()///  
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()/// ⬅️, 将选中的给到属性上
				.HAlign(HAlign_Right)
				[
					PropertyCustomizationHelpers::MakeUseSelectedButton(FSimpleDelegate::CreateRaw(this, &SDoodleImportUiItem::DoodleUseSelected))/// 委托转发
				]
			+ SHorizontalBox::Slot()/// 🔍 将属性显示在资产编辑器中
				.HAlign(HAlign_Right)
				[
					PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateRaw(this, &SDoodleImportUiItem::DoodleBrowse))/// 委托转发
				]
			+ SHorizontalBox::Slot()/// 重置, 将属性给空
				.HAlign(HAlign_Right)
				[
					PropertyCustomizationHelpers::MakeResetButton(FSimpleDelegate::CreateRaw(this, &SDoodleImportUiItem::DoodleReset))/// 委托转发
				]
				]
          // clang-format on
          ;
    } else {
      return SNew(STextBlock).Text(FText::FromString(ItemType));
    }
  }

  void DoodleUseSelected() {
    FContentBrowserModule& L_ContentBrowserModle =
        FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FAssetData> L_SelectedAss;
    L_ContentBrowserModle.Get().GetSelectedAssets(L_SelectedAss);

    FAssetData* L_It = Algo::FindByPredicate(L_SelectedAss, [](const FAssetData& InAss) -> bool {
      return Cast<USkeleton>(InAss.GetAsset()) != nullptr;
    });
    if (L_It != nullptr) {
      ItemShowFBX->SkinObj = Cast<USkeleton>(L_It->GetAsset());
    }
  }
  void DoodleBrowse() {
    if (ItemShowFBX->SkinObj != nullptr) {
      FContentBrowserModule& L_ContentBrowserModle =
          FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
      L_ContentBrowserModle.Get().SyncBrowserToAssets(TArray<FAssetData>{FAssetData{ItemShowFBX->SkinObj}});
    }
  }
  void DoodleReset() { ItemShowFBX->SkinObj = nullptr; }

 private:
  SDoodleImportFbxUI::UDoodleBaseImportDataPtrType ItemShow;
  UDoodleFbxImport_1* ItemShowFBX{};
  FString ItemType{};
};

void SDoodleImportFbxUI::Construct(const FArguments& Arg) {
  const FSlateFontInfo Font = FAppStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));

#if PLATFORM_WINDOWS
  const FString FileFilterText = TEXT("fbx and abc |*.fbx;*.abc|fbx (*.fbx)|*.fbx|abc (*.abc)|*.abc");
#else
  const FString FileFilterText = FString::Printf(TEXT("%s"), *FileFilterType.ToString());
#endif

  const static TArray<TSharedPtr<FString>> L_DepType{
      MakeShared<FString>(TEXT("Lig")), MakeShared<FString>(TEXT("Lig_Sim")), MakeShared<FString>(TEXT("Vfx"))};

  Path_Suffix = *L_DepType[0];

  // clang-format off
	ChildSlot
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 0.0f))
		.BorderImage(new FSlateBrush())
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			// 扫描文件目录槽
		+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BinaryPathLabel", "将文件和文件夹拖入到这个窗口中, 会自动扫描文件夹下后缀为abc和fbx的子文件,并将所有的文件添加到导入列表中.\n同时也会根据拖入的相机以及各种文件生成关卡"))
		.ColorAndOpacity(FSlateColor{ FLinearColor{1,0,0,1} })
		.Font(Font)
		]
	// 前缀槽
	+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BinaryPathLabel1", "项目缩写"))
		.Font(Font)
		]
	+ SHorizontalBox::Slot()
		.FillWidth(8.0f)
		[
			/// 生成的前缀
			SNew(SEditableTextBox)
			.Text_Lambda([this]()-> FText {
		return FText::FromString(this->Path_Prefix);
				})
		.OnTextChanged_Lambda([this](const FText& In_Text) {
					GenPathPrefix(In_Text.ToString(), Path_Suffix);
			})
					.OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type) {
				GenPathPrefix(In_Text.ToString(), Path_Suffix);
						})
		]
		]
	// 后缀槽
	+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BinaryPathLabel11", "部门缩写"))
		.ColorAndOpacity(FSlateColor{ FLinearColor{1,0,0,1} })
		.Font(Font)
		]
	+ SHorizontalBox::Slot()
		.FillWidth(8.0f)
		[
			///  
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&L_DepType)
		.OnSelectionChanged_Lambda(
			[this](const TSharedPtr<FString>& In, ESelectInfo::Type) {
				GenPathPrefix(Path_Prefix, *In);
			})
		.OnGenerateWidget_Lambda(
			[this](const TSharedPtr<FString>& In) {
				return SNew(STextBlock).Text(FText::FromString(*In));
			})
				.InitiallySelectedItem(L_DepType[0])
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(Path_Suffix); })
				]
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("importTitle", "导入的fbx 动画文件, 相机, abc文件"))
		.Font(Font)
		]

	/// 主要的列表小部件(Fbx)
	+ SVerticalBox::Slot()
		.FillHeight(3.0f)
		.VAlign(VAlign_Top)
		.Padding(2.0f)
		[
			SAssignNew(ListImportGui, SListView<SDoodleImportFbxUI::UDoodleBaseImportDataPtrType>)
			.ItemHeight(80) // 小部件高度
		.ListItemsSource(&ListImportData)
		.ScrollbarVisibility(EVisibility::All)
		.OnGenerateRow_Lambda(// 生成小部件
			[](SDoodleImportFbxUI::UDoodleBaseImportDataPtrType InItem,
				const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow> {
					return SNew(SDoodleImportUiItem, OwnerTable)
						.ItemShow(InItem);
			}
		)
		.SelectionMode(ESelectionMode::Type::Single) //单选
				.HeaderRow ///题头元素
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(TEXT("Import_File"))
					.FillWidth(4.0f)
					[
						SNew(SBorder)
						.Padding(5)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Import File")))
					]
					]
			+ SHeaderRow::Column(TEXT("Ep_And_Shot"))
				.FillWidth(1.0f)
				.DefaultLabel(LOCTEXT("Ep_And_Shot", "集数和镜头"))

				+ SHeaderRow::Column(TEXT("Time_Ranges"))
				.FillWidth(1.0f)
				.DefaultLabel(LOCTEXT("Time Ranges", "时间范围"))

				+ SHeaderRow::Column(TEXT("Skeleton_Path"))
				.FillWidth(4.0f)
				.DefaultLabel(LOCTEXT("Skeleton Path", "骨骼路径"))

				+ SHeaderRow::Column(TEXT("Import_Path_Dir"))
				.FillWidth(2.0f)
				.DefaultLabel(LOCTEXT("Import Path Dir", "导入的目标"))
				)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Search USkeleton", "Search USkeleton"))
		.ToolTipText(LOCTEXT("Search USkeleton Tip", "寻找骨骼"))
		.OnClicked_Lambda([this]() {
		FindSK();
		return FReply::Handled();
			})
		]
	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Search USkeleton And Import", "Search USkeleton And Import"))
		.ToolTipText(LOCTEXT("Search USkeleton Tip2", "寻找骨骼并导入Fbx"))
		.OnClicked_Lambda([this]() {
		FindSK();
		ImportFile();
		return FReply::Handled();
			})
		]

	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Clear USkeleton", "Clear USkeleton"))
		.ToolTipText(LOCTEXT("Clear USkeleton Tip", "清除查找的骨骼"))
		.OnClicked_Lambda([this]() {
		for (auto&& i : ListImportData) {
			if (i->IsA<UDoodleFbxImport_1>()) {
				Cast<UDoodleFbxImport_1>(i)->SkinObj = nullptr;
			}
		}
		ListImportGui->RebuildList();
		return FReply::Handled();
			})
		]

	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Clear All", "Clear All"))
		.ToolTipText(LOCTEXT("Clear USkeleton Tip", "清除所有"))
		.OnClicked_Lambda([this]() {
		ListImportData.Empty();
		ListImportGui->RebuildList();
		return FReply::Handled();
			})
		]
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Search USkeleton Import", "Search USkeleton Direct Import"))
		.ToolTipText(LOCTEXT("Search USkeleton Tip3", "不寻找骨骼, 直接导入 Fbx, 如果已经寻找过则使用寻找的数据"))
		.OnClicked_Lambda([this]() {
		ImportFile();
		return FReply::Handled();
			})
		]
		]

		];
  // clang-format on
}

void SDoodleImportFbxUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
        return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SDoodleImportFbxUI)];  // 这里创建我们自己的界面
}


bool SDoodleImportFbxUI::IsCamera(UnFbx::FFbxImporter* InFbx) {
        TArray<fbxsdk::FbxCamera*> L_Cameras{};
        MovieSceneToolHelpers::GetCameras(InFbx->Scene->GetRootNode(), L_Cameras);

        return !L_Cameras.IsEmpty();
}

void SDoodleImportFbxUI::FindSK() {
        for (auto&& i : ListImportData) {
    if (auto&& L_Fbx = Cast<UDoodleFbxImport_1>(i)) {
      if (FPaths::FileExists(L_Fbx->ImportPath) && FPaths::GetExtension(L_Fbx->ImportPath, true) == TEXT(".fbx")) {
        UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
        FbxImporter->ClearAllCaches();

        TArray<TSharedPtr<UDoodleFbxImport_1>> L_RemoveList;
        FScopedSlowTask L_Task_Scoped1{2.0f, LOCTEXT("FindSK1", "加载 fbx 文件中...")};
        L_Task_Scoped1.MakeDialog();
        // FString L_Debug_str{};

        FbxImporter->ImportFromFile(L_Fbx->ImportPath, FPaths::GetExtension(L_Fbx->ImportPath));
        L_Fbx->FindSkeleton(AllSkinObjs);
      }
    }
        }
}

void SDoodleImportFbxUI::ImportFile() {
        FScopedSlowTask L_Task_Scoped1{(float)ListImportData.Num(), LOCTEXT("ImportFile1", "加载 fbx 文件中...")};
        L_Task_Scoped1.MakeDialog();
        for (auto&& i : ListImportData) {
    L_Task_Scoped1.EnterProgressFrame(1.0f, LOCTEXT("ImportFile2", "导入文件中"));
    i->ImportFile();
        }
}

void SDoodleImportFbxUI::GenPathPrefix(const FString& In_Path_Prefix, const FString& In_Path_Suffix) {
        Path_Prefix = In_Path_Prefix;
        Path_Suffix = In_Path_Suffix;
        for (auto&& L_Fbx : ListImportData) {
    L_Fbx->GenPathPrefix(Path_Prefix, Path_Suffix);
        }
        ListImportGui->RebuildList();
}

void SDoodleImportFbxUI::SetFbxOnlyAnim() {
        TSet<FString> L_Abc_path{};
        for (auto&& L_Fbx : ListImportData) {
    if (FPaths::GetExtension(L_Fbx->ImportPath, true) == TEXT(".abc")) {
      FString L_Path = FPaths::GetPath(L_Fbx->ImportPath) / FPaths::GetBaseFilename(L_Fbx->ImportPath);
      L_Path += ".fbx";
      FPaths::NormalizeFilename(L_Path);
      L_Abc_path.Emplace(L_Path);
    }
        }

        for (auto&& L_Fbx : ListImportData) {
    FString L_Path = L_Fbx->ImportPath;
    FPaths::NormalizeFilename(L_Path);
    if (L_Abc_path.Contains(L_Path)) {
      if (auto L_F = Cast<UDoodleFbxImport_1>(L_Fbx)) {
        L_F->OnlyAnim = false;
      }
    }
        }
}

void SDoodleImportFbxUI::AddFile(const FString& In_File) {
        /// @brief 先扫描前缀
        if (this->Path_Prefix.IsEmpty()) {
    this->Path_Prefix = UDoodleBaseImportData::GetPathPrefix(In_File);
        }

        /// @brief 寻找到相同的就跳过
        if (ListImportData.FindByPredicate([&](const SDoodleImportFbxUI::UDoodleBaseImportDataPtrType& In_FBx) {
              return In_FBx->ImportPath == In_File;
            })) {
    return;
        };
        SDoodleImportFbxUI::UDoodleBaseImportDataPtrType L_File{};
        /// 扫描fbx 和abc 文件
        if (FPaths::FileExists(In_File) && FPaths::GetExtension(In_File, true) == TEXT(".fbx")) {
    UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
    FbxImporter->ClearAllCaches();

    FScopedSlowTask L_Task_Scoped1{2.0f, LOCTEXT("DoingSlowWork1", "加载 fbx 文件中...")};
    L_Task_Scoped1.MakeDialog();
    // FString L_Debug_str{};

    FbxImporter->ImportFromFile(In_File, FPaths::GetExtension(In_File));

    if (IsCamera(FbxImporter)) {
      L_Task_Scoped1.EnterProgressFrame(1.0f, LOCTEXT("DoingSlowWork21", "确认为相机"));
      SDoodleImportFbxUI::UDoodleBaseImportDataPtrType L_ptr = NewObject<UDoodleFbxCameraImport_1>();
      L_ptr->ImportPath                                      = In_File;
      L_File                                                 = ListImportData.Emplace_GetRef(L_ptr);
    } else {
      TObjectPtr<UDoodleFbxImport_1> L_ptr = NewObject<UDoodleFbxImport_1>();
      L_ptr->ImportPath                    = In_File;
      L_Task_Scoped1.EnterProgressFrame(1.0f, LOCTEXT("DoingSlowWork3", "寻找匹配骨骼"));
      if (L_ptr->FindSkeleton(AllSkinObjs)) L_File = ListImportData.Emplace_GetRef(L_ptr);
    }
        }
        if (FPaths::FileExists(In_File) && FPaths::GetExtension(In_File, true) == TEXT(".abc")) {
    SDoodleImportFbxUI::UDoodleBaseImportDataPtrType L_ptr = NewObject<UDoodleAbcImport_1>();
    L_ptr->ImportPath                                      = In_File;
    L_File                                                 = ListImportData.Emplace_GetRef(L_ptr);
        }
        if (L_File) L_File->GenStartAndEndTime();
}



// DragBegin
FReply SDoodleImportFbxUI::OnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) {
        auto L_Opt = InDragDropEvent.GetOperationAs<FExternalDragOperation>();
        return L_Opt && L_Opt->HasFiles() ? FReply::Handled() : FReply::Unhandled();
}

FReply SDoodleImportFbxUI::OnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent) {
        auto L_Opt = InDragDropEvent.GetOperationAs<FExternalDragOperation>();

        if (!(L_Opt && L_Opt->HasFiles())) return FReply::Unhandled();

        ListImportData.Empty();
        AllSkinObjs.Empty();
        // 优先扫描内部的sk
        AllSkinObjs = FDoodleUSkeletonData_1::ListAllSkeletons();

        for (auto&& Path : L_Opt->GetFiles()) {
    if (FPaths::DirectoryExists(Path)) {
      // 目录进行迭代
      IFileManager::Get().IterateDirectoryRecursively(*Path, [this](const TCHAR* InPath, bool in_) -> bool {
        AddFile(InPath);
        return true;
      });
    } else if (FPaths::FileExists(Path)) {
      // 文件直接添加
      AddFile(Path);
    }
        }
        GenPathPrefix(Path_Prefix, Path_Suffix);
        SetFbxOnlyAnim();
        ListImportGui->RebuildList();

        return FReply::Handled();
}

// DragEnd

TArray<FDoodleUSkeletonData_1> FDoodleUSkeletonData_1::ListAllSkeletons() {
        FScopedSlowTask L_Task_Scoped{2.0f, LOCTEXT("Import_Fbx2", "扫描所有的Skin")};
        L_Task_Scoped.MakeDialog();
        TArray<FDoodleUSkeletonData_1> L_AllSkinObjs{};

        FARFilter LFilter{};
        LFilter.bIncludeOnlyOnDiskAssets = false;
        LFilter.bRecursivePaths          = true;
        LFilter.bRecursiveClasses        = true;
        LFilter.ClassPaths.Add(USkeleton::StaticClass()->GetClassPathName());

        IAssetRegistry::Get()->EnumerateAssets(LFilter, [&](const FAssetData& InAss) -> bool {
          USkeleton* L_SK = Cast<USkeleton>(InAss.GetAsset());
          if (L_SK) {
            FDoodleUSkeletonData_1& L_Ref_Data = L_AllSkinObjs.Emplace_GetRef();
            L_Ref_Data.SkinObj                 = L_SK;
            for (auto&& L_Item : L_SK->GetReferenceSkeleton().GetRawRefBoneInfo())
              L_Ref_Data.BoneNames.Add(L_Item.ExportName);
          }
          return true;
        });
        FRegexPattern L_Reg_Ep_Pattern{LR"((SK_)?(\w+)_Skeleton)"};
        for (auto&& L_Sk : L_AllSkinObjs) {
    FRegexMatcher L_Reg{L_Reg_Ep_Pattern, L_Sk.SkinObj->GetName()};
    if (L_Reg.FindNext()) {
      FString L_Str = L_Reg.GetCaptureGroup(2);
      L_Sk.SkinTag  = L_Str.IsEmpty() ? L_Reg.GetCaptureGroup(1) : L_Str;
    }
        }
        return L_AllSkinObjs;
}

#undef LOCTEXT_NAMESPACE
