#include "DoodleImportFbxUI.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "AssetRegistry/IAssetRegistry.h"

// fbx读取需要
#include "FbxImporter.h"
#include "Rendering/SkeletalMeshLODImporterData.h"

#include "fbxsdk/scene/geometry/fbxcameraswitcher.h"
#include "fbxsdk/scene/geometry/fbxcamera.h"
#include "fbxsdk/scene/animation/fbxanimstack.h"
#include "fbxsdk/scene/animation/fbxanimlayer.h"
#include "fbxsdk/scene/geometry/fbxnode.h"
#include "fbxsdk/scene/animation/fbxanimcurve.h"

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
#define LOCTEXT_NAMESPACE "SDoodleImportFbxUI"
const FName SDoodleImportFbxUI::Name{TEXT("DoodleImportFbxUI")};

namespace {
struct grop_SDoodleImportFbxUI {
  UnFbx::FFbxImporter* FbxImporterData;
  grop_SDoodleImportFbxUI(UnFbx::FFbxImporter* In) : FbxImporterData(In) {}
  ~grop_SDoodleImportFbxUI() {
    FbxImporterData->ClearAllCaches();
    FbxImporterData->ReleaseScene();
  }
};

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
       (Parent->GetNodeAttribute() && (Parent->GetNodeAttribute()->GetAttributeType() == fbxsdk::FbxNodeAttribute::eSkeleton ||
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

class SDoodleImportFbxUiItem : public SMultiColumnTableRow<TSharedPtr<doodle_ue4::FFbxImport>> {
 public:
  SLATE_BEGIN_ARGS(SDoodleImportFbxUiItem) : _ItemShow() {}

  SLATE_ARGUMENT(TSharedPtr<doodle_ue4::FFbxImport>, ItemShow)

  SLATE_END_ARGS()

 public:
  void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) {
    ItemShow = InArgs._ItemShow;

    FSuperRowType::Construct(FSuperRowType::FArguments().Padding(0), InOwnerTableView);
  }

 public:  // override SMultiColumnTableRow
  virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override {
    if (ColumnName == TEXT("Fbx File"))  // 判断列名为Fbx File，次名称在创建View时，通过SHeaderRow::Column指定
    {
      return SNew(STextBlock)
          .Text(FText::FromString(ItemShow->ImportFbxPath));
    } else if (ColumnName == TEXT("Import Path Dir")) {
      return SNew(STextBlock)
          .Text(FText::FromString(ItemShow->ImportPathDir));
    } else {
      // clang-format off
       return SNew(SHorizontalBox) 
        + SHorizontalBox::Slot()
        .Padding(1.f)
        .HAlign(HAlign_Left)
        [
          SNew(STextBlock)
          .Text_Lambda([this]() -> FText {
            return FText::FromString(FString::Printf(TEXT("%s"), *( ItemShow->SkinObj != nullptr ?
                                                                    ItemShow->SkinObj->GetPackage()->GetPathName() : FString{TEXT("")})));
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
            PropertyCustomizationHelpers::MakeUseSelectedButton(FSimpleDelegate::CreateRaw(this,&SDoodleImportFbxUiItem::DoodleUseSelected))/// 委托转发
          ]
          + SHorizontalBox::Slot()/// 🔍 将属性显示在资产编辑器中
          .HAlign(HAlign_Right)
          [
            PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateRaw(this,&SDoodleImportFbxUiItem::DoodleBrowse))/// 委托转发
          ]
          + SHorizontalBox::Slot()/// 重置, 将属性给空
          .HAlign(HAlign_Right)
          [
            PropertyCustomizationHelpers::MakeResetButton(FSimpleDelegate::CreateRaw(this,&SDoodleImportFbxUiItem::DoodleReset))/// 委托转发
          ]
        ]
          // clang-format on
          ;
      // return SNew(STextBlock)
      //     .Text(FText::FromString(FString::Printf(TEXT("%s"), *ItemShow->SkinObj->GetPackage()->GetPathName())));
    }
  }

  void DoodleUseSelected() {
    FContentBrowserModule& L_ContentBrowserModle =
        FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
            "ContentBrowser"
        );
    TArray<FAssetData> L_SelectedAss;
    L_ContentBrowserModle.Get().GetSelectedAssets(L_SelectedAss);

    FAssetData* L_It = Algo::FindByPredicate(L_SelectedAss, [](const FAssetData& InAss) -> bool {
      return Cast<USkeleton>(InAss.GetAsset()) != nullptr;
    });
    if (L_It != nullptr) {
      ItemShow->SkinObj = Cast<USkeleton>(L_It->GetAsset());
    }
  }
  void DoodleBrowse() {
    if (ItemShow->SkinObj != nullptr) {
      FContentBrowserModule& L_ContentBrowserModle =
          FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(
              "ContentBrowser"
          );
      L_ContentBrowserModle.Get().SyncBrowserToAssets(TArray<FAssetData>{FAssetData{ItemShow->SkinObj}});
    }
  }
  void DoodleReset() {
    ItemShow->SkinObj = nullptr;
  }

 private:
  TSharedPtr<doodle_ue4::FFbxImport> ItemShow;
};

void SDoodleImportFbxUI::Construct(const FArguments& Arg) {
  const FSlateFontInfo Font = FEditorStyle::GetFontStyle(TEXT("SourceControl.LoginWindow.Font"));

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
          // 扫描的文件目录
          SNew(SHorizontalBox)
          +SHorizontalBox::Slot()
          .FillWidth(1.0f)
          [
            SNew(STextBlock)
            .Text(LOCTEXT("BinaryPathLabel", "search path"))
            .ToolTipText(LOCTEXT("BinaryPathLabel_Tooltip", "search path"))
            .Font(Font)
          ]
          +SHorizontalBox::Slot()
          .FillWidth(2.0f)
          [
            SNew(SDirectoryPicker)
            .OnDirectoryChanged_Raw(this,&SDoodleImportFbxUI::SearchPath)
          ]
        ]
        // 前缀槽
        + SVerticalBox::Slot()
        .AutoHeight()
        .VAlign(VAlign_Center)
        .Padding(2.0f)
        [
          /// 生成的前缀
          SNew(SEditableTextBox)
          .Text_Lambda([this]()->FText{
            return FText::FromString(this->Path_Prefix);
          })
          .OnTextChanged_Lambda([this](const FText& In_Text){
            GenPathPrefix(In_Text.ToString());
          })
          .OnTextCommitted_Lambda([this](const FText& In_Text, ETextCommit::Type){
            GenPathPrefix(In_Text.ToString());
          })
        ]

        /// 主要的列表小部件
        + SVerticalBox::Slot()
        .AutoHeight()
        .VAlign(VAlign_Center)
        .Padding(2.0f)
        [
          SAssignNew(ListImportFbx,SListView<TSharedPtr<doodle_ue4::FFbxImport>>)
          .ItemHeight(80) // 小部件高度
          .ListItemsSource(&ListImportFbxData)
          .OnGenerateRow_Lambda(// 生成小部件
            [](TSharedPtr<doodle_ue4::FFbxImport> InItem, 
               const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow> {
              return SNew(SDoodleImportFbxUiItem, OwnerTable)
                    .ItemShow(InItem);
            }
          )
          .SelectionMode(ESelectionMode::Type::Single) //单选
          .HeaderRow ///题头元素
          (
            SNew(SHeaderRow)
            + SHeaderRow::Column(TEXT("Fbx File"))
            [
              SNew(SBorder)
              .Padding(5)
              [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Fbx File")))
              ]
            ]
            +SHeaderRow::Column(TEXT("Skeleton Path"))
            .DefaultLabel(LOCTEXT("Skeleton Path","Skeleton Path"))

            +SHeaderRow::Column(TEXT("Import Path Dir"))
            .DefaultLabel(LOCTEXT("Import Path Dir","Import Path Dir"))
          )
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .VAlign(VAlign_Center)
        .Padding(2.0f)
        [
          SNew(SHorizontalBox)
          +SHorizontalBox::Slot()
          .FillWidth(1.0f)
          [
            SNew(SButton)
            .Text(LOCTEXT("Search USkeleton","Search USkeleton"))
            .ToolTipText(LOCTEXT("Search USkeleton Tip","寻找骨骼"))
            .OnClicked_Lambda([this](){
               GetAllSkinObjs();
               MatchFbx();
               ListImportFbx->RebuildList();
               return FReply::Handled();
            })
          ]
          +SHorizontalBox::Slot()
          .FillWidth(1.0f)
          [
            SNew(SButton)
            .Text(LOCTEXT("Search USkeleton And Import","Search USkeleton And Import"))
            .ToolTipText(LOCTEXT("Search USkeleton Tip2","寻找骨骼并导入Fbx"))
            .OnClicked_Lambda([this](){
               GetAllSkinObjs();
               MatchFbx();
               ImportFbx();
               ListImportFbx->RebuildList();
               return FReply::Handled();
            })
          ]
          +SHorizontalBox::Slot()
          .FillWidth(1.0f)
          [
            SNew(SButton)
            .Text(LOCTEXT("Search USkeleton Import","Search USkeleton Direct Import"))
            .ToolTipText(LOCTEXT("Search USkeleton Tip3","不寻找骨骼, 直接导入 Fbx, 如果已经寻找过则使用寻找的数据"))
            .OnClicked_Lambda([this](){
               ImportFbx();
               ListImportFbx->RebuildList();
               return FReply::Handled();
            })
          ]




        ]
      ]



    ];
  // clang-format on
}

void SDoodleImportFbxUI::SearchPath(const FString& in) {
  ListImportFbxData.Empty();
  AllSkinObjs.Empty();
  FScopedSlowTask L_Task_Scoped{5.0f, LOCTEXT("Import_Fbx", "加载Fbx")};
  L_Task_Scoped.MakeDialog();

  IFileManager::Get().IterateDirectoryRecursively(*in, [this](const TCHAR* InPath, bool in_) -> bool {
    if (FPaths::FileExists(InPath) && !IFileManager::Get().DirectoryExists(InPath) &&
        FPaths::GetExtension(InPath, true) == TEXT(".fbx")) {
      FString L_Path{InPath};
      TSharedPtr<doodle_ue4::FFbxImport> L_ptr = MakeShared<doodle_ue4::FFbxImport>(L_Path);
      L_ptr->ImportPathDir                     = this->GetImportPath(L_Path);
      ListImportFbxData.Emplace(L_ptr);
      if (this->Path_Prefix.IsEmpty()) {
        int32 L_Index      = INDEX_NONE;
        FString L_FileName = FPaths::GetBaseFilename(L_Path);
        if (L_FileName.FindChar('_', L_Index)) {
          L_FileName.LeftChopInline(L_FileName.Len() - L_Index, true);
          this->Path_Prefix = L_FileName;
        }
      }
    }
    return true;
  });
  L_Task_Scoped.EnterProgressFrame(1.0f);
  GetAllSkinObjs();
  L_Task_Scoped.EnterProgressFrame(3.0f);
  MatchFbx();
  L_Task_Scoped.EnterProgressFrame(1.0f);
  ListImportFbx->RebuildList();
}
void SDoodleImportFbxUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SDoodleImportFbxUI)];  // 这里创建我们自己的界面
}

void SDoodleImportFbxUI::GetAllSkinObjs() {
  FScopedSlowTask L_Task_Scoped{2.0f, LOCTEXT("Import_Fbx2", "扫描所有的Skin")};
  L_Task_Scoped.MakeDialog();

  this->AllSkinObjs.Empty();
  FARFilter LFilter{};
  LFilter.bIncludeOnlyOnDiskAssets = false;
  LFilter.bRecursivePaths          = true;
  LFilter.bRecursiveClasses        = true;
  LFilter.ClassNames.Add(FName{USkeleton::StaticClass()->GetName()});

  IAssetRegistry::Get()->EnumerateAssets(LFilter, [this](const FAssetData& InAss) -> bool {
    USkeleton* L_SK = Cast<USkeleton>(InAss.GetAsset());
    if (L_SK) {
      doodle_ue4::FUSkeletonData& L_Ref_Data = this->AllSkinObjs.Emplace_GetRef();
      L_Ref_Data.SkinObj                     = L_SK;

      for (auto&& L_Item : L_SK->GetReferenceSkeleton().GetRawRefBoneInfo())
        L_Ref_Data.BoneNames.Add(L_Item.ExportName);
    }
    return true;
  });
  L_Task_Scoped.EnterProgressFrame(1.0f);
  SetAllSkinTag();
  L_Task_Scoped.EnterProgressFrame(1.0f);
  // LFilter.ClassNames.Add(FName{USkeletalMesh::StaticClass()->GetName()});
  // IAssetRegistry::Get()->EnumerateAssets(LFilter, [&, this](const FAssetData& InAss) -> bool {
  //   USkeletalMesh* L_SK = Cast<USkeletalMesh>(InAss.GetAsset());
  //   if (L_SK && L_SK->GetSkeleton()) {
  //     FString L_BaseName = FPaths::GetBaseFilename(L_SK->GetAssetImportData()->GetFirstFilename());
  //     this->AllSkinObjs_Map.Add(MakeTuple(L_BaseName, L_SK->GetSkeleton()));
  //   }
  //   return true;
  // });
}

void SDoodleImportFbxUI::MatchFbx() {
  UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
  FbxImporter->ClearAllCaches();

  TArray<TSharedPtr<doodle_ue4::FFbxImport>> L_RemoveList;
  FScopedSlowTask L_Task_Scoped{(float_t)ListImportFbxData.Num(), LOCTEXT("DoingSlowWork1", "加载 fbx 文件中...")};
  L_Task_Scoped.MakeDialog();
  // FString L_Debug_str{};

  for (auto&& L_Fbx_Path : ListImportFbxData) {
    grop_SDoodleImportFbxUI L_grop_SDoodleImportFbxUI{FbxImporter};
    FbxImporter->ImportFromFile(L_Fbx_Path->ImportFbxPath, FPaths::GetExtension(L_Fbx_Path->ImportFbxPath));

    TArray<fbxsdk::FbxNode*> L_Fbx_Node_list{};
    FString L_NameSpace{};
    L_Task_Scoped.EnterProgressFrame(1.0f);

    FScopedSlowTask L_Task_Scoped2{(float_t)FbxImporter->Scene->GetNodeCount(), LOCTEXT("DoingSlowWork2", "扫描 fbx 文件骨骼中...")};
    for (size_t i = 0; i < FbxImporter->Scene->GetNodeCount(); ++i) {
      FString L_Name = MakeName(FbxImporter->Scene->GetNode(i)->GetName());
      L_Fbx_Path->FbxNodeNames.Add(L_Name);
      // 获取名称空间
      if (L_NameSpace.IsEmpty())
        L_NameSpace = GetNamepace(FbxImporter->Scene->GetNode(i)->GetName());

      L_Task_Scoped2.EnterProgressFrame(1.0f);
    }

    if (L_NameSpace.IsEmpty()) {
      L_RemoveList.Add(L_Fbx_Path);
      continue;
    }

    FScopedSlowTask L_Task_Scoped3{(float_t)FbxImporter->Scene->GetNodeCount(), LOCTEXT("DoingSlowWork3", "寻找 fbx 文件骨骼匹配的SK中...")};
    for (auto&& L_SK_Data : this->AllSkinObjs) {
      FString L_BaseName = FPaths::GetBaseFilename(L_Fbx_Path->ImportFbxPath);
      if (L_Fbx_Path->FbxNodeNames.Num() >= L_SK_Data.BoneNames.Num()) {
        if (
            (
                L_SK_Data.SkinTag.IsEmpty()
                    ? true
                    : L_BaseName.Find(L_SK_Data.SkinTag) != INDEX_NONE
            )  /// 先确认字串节省资源
            && Algo::AllOf(L_SK_Data.BoneNames, [&](const FString& IN_Str) {
                 return L_Fbx_Path->FbxNodeNames.Contains(IN_Str);
               })  /// 进一步确认骨骼内容
        )
          L_Fbx_Path->SkinObj = L_SK_Data.SkinObj;
        L_Task_Scoped3.EnterProgressFrame(1.0f);
      }
    }
  }

  /// 删除没有名称空间的
  for (auto&& L_R : L_RemoveList) {
    ListImportFbxData.RemoveSingle(L_R);
  }
}

void SDoodleImportFbxUI::ImportFbx() {
  TArray<UAssetImportTask*> ImportDataList{};
  for (auto&& L_Fbx : ListImportFbxData) {
    UAssetImportTask* l_task                                       = NewObject<UAssetImportTask>();
    l_task->bAutomated                                             = true;
    l_task->bReplaceExisting                                       = true;
    l_task->bReplaceExistingSettings                               = true;
    /// @fix 此处不需要自动保存, 否则会出现一个ue4.27中发现的bug 会将
    /// UAssetImportTask::ImportedObjectPaths 值转换为乱码

    // l_task->bSave = true;
    l_task->DestinationPath                                        = L_Fbx->ImportPathDir;
    l_task->Filename                                               = L_Fbx->ImportFbxPath;

    l_task->Factory                                                = DuplicateObject<UFbxFactory>(GetDefault<UFbxFactory>(), l_task);

    UFbxFactory* k_fbx_f                                           = Cast<UFbxFactory>(l_task->Factory);

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
    k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength     = FBXALIT_ExportedTime;
    k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks   = true;
    k_fbx_f->ImportUI->bAllowContentTypeImport                     = true;
    k_fbx_f->ImportUI->TextureImportData->MaterialSearchLocation   = EMaterialSearchLocation::UnderRoot;
    if (L_Fbx->SkinObj) {
      k_fbx_f->ImportUI->Skeleton                                    = L_Fbx->SkinObj;
      k_fbx_f->ImportUI->MeshTypeToImport                            = FBXIT_Animation;
      k_fbx_f->ImportUI->OriginalImportType                          = FBXIT_SkeletalMesh;
      k_fbx_f->ImportUI->bImportAsSkeletal                           = true;
      k_fbx_f->ImportUI->bImportMesh                                 = false;
      k_fbx_f->ImportUI->bImportAnimations                           = true;
      k_fbx_f->ImportUI->bImportRigidMesh                            = false;
      k_fbx_f->ImportUI->bImportMaterials                            = false;
      k_fbx_f->ImportUI->bImportTextures                             = false;
      k_fbx_f->ImportUI->bResetToFbxOnMaterialConflict               = false;

      k_fbx_f->ImportUI->SkeletalMeshImportData->bImportMorphTargets = true;
      k_fbx_f->ImportUI->bAutomatedImportShouldDetectType            = false;
      k_fbx_f->ImportUI->AnimSequenceImportData->AnimationLength     = FBXALIT_ExportedTime;
      k_fbx_f->ImportUI->AnimSequenceImportData->bImportBoneTracks   = true;
      k_fbx_f->ImportUI->bAllowContentTypeImport                     = true;
    }
    ImportDataList.Add(l_task);
  }
  FAssetToolsModule& AssetToolsModule =
      FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
  TArray<FString> import_Paths{};
  AssetToolsModule.Get().ImportAssetTasks(ImportDataList);
}

FString SDoodleImportFbxUI::GetImportPath(const FString& In_Path) {
  FRegexPattern L_Reg_Ep_Pattern{LR"(ep_?(\d+))"};
  FRegexMatcher L_Reg_Ep{L_Reg_Ep_Pattern, In_Path};
  int64 L_eps{};
  int64 L_sc{};
  FString L_Sc_ab{};

  if (L_Reg_Ep.FindNext()) {
    L_eps = FCString::Atoi64(*L_Reg_Ep.GetCaptureGroup(1));
  }

  FRegexPattern L_Reg_ScPattern{LR"(sc_?(\d+)([a-z])?)"};
  FRegexMatcher L_Reg_Sc{L_Reg_ScPattern, In_Path};

  if (L_Reg_Sc.FindNext()) {
    L_sc = FCString::Atoi64(*L_Reg_Sc.GetCaptureGroup(1));
    if (L_Reg_Sc.GetEndLimit() > 2) {
      L_Sc_ab = L_Reg_Sc.GetCaptureGroup(2);
    }
  }

  FString L_Path = FString::Printf(
      TEXT("/Game/Shot/ep%.4d/%s%.4d_sc%.4d%s"),
      L_eps, *Path_Prefix, L_eps, L_sc, *L_Sc_ab
  );
  return L_Path;
}

void SDoodleImportFbxUI::GenPathPrefix(const FString& In_Path_Prefix) {
  Path_Prefix = In_Path_Prefix;
  for (auto&& L_Fbx : ListImportFbxData) {
    L_Fbx->ImportPathDir = GetImportPath(L_Fbx->ImportFbxPath);
  }
  ListImportFbx->RebuildList();
}

void SDoodleImportFbxUI::SetAllSkinTag() {
  FRegexPattern L_Reg_Ep_Pattern{LR"(SK_(\w+)_Skeleton)"};
  for (auto&& L_Sk : AllSkinObjs) {
    FRegexMatcher L_Reg{L_Reg_Ep_Pattern, L_Sk.SkinObj->GetName()};
    if (L_Reg.FindNext())
      L_Sk.SkinTag = L_Reg.GetCaptureGroup(1);
  }
}
#undef LOCTEXT_NAMESPACE