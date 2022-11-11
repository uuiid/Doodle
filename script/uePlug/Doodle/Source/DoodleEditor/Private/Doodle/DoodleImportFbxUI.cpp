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

  return TmpName;
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
  return TmpName;
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
    } else {
      if (ItemShow->SkinObj)
        return SNew(STextBlock)
            .Text(FText::FromString(FString::Printf(TEXT("%s"), *ItemShow->SkinObj->GetPackage()->GetPathName())));
      else
        return SNew(STextBlock).Text(FText::FromString(TEXT(" ")));
    }
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
      [
        SNew(SVerticalBox) 
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
          )
        ]
      ]



    ];
  // clang-format on
  // SNew(SDirectoryPicker)
}
void SDoodleImportFbxUI::SearchPath(const FString& in) {
  ListImportFbxData.Empty();
  IFileManager::Get().IterateDirectoryRecursively(*in, [this](const TCHAR* InPath, bool in_) -> bool {
    if (FPaths::FileExists(InPath) && !IFileManager::Get().DirectoryExists(InPath) && FPaths::GetExtension(InPath, true) == TEXT(".fbx")) {
      TSharedPtr<doodle_ue4::FFbxImport> L_ptr = MakeShared<doodle_ue4::FFbxImport>(FString{InPath});
      ListImportFbxData.Emplace(L_ptr);
    }
    return true;
  });
  GetAllSkinObjs();
  MatchFbx();
  ListImportFbx->RebuildList();
}
void SDoodleImportFbxUI::AddReferencedObjects(FReferenceCollector& collector) {}

TSharedRef<SDockTab> SDoodleImportFbxUI::OnSpawnAction(const FSpawnTabArgs& SpawnTabArgs) {
  return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SDoodleImportFbxUI)];  // 这里创建我们自己的界面
}

void SDoodleImportFbxUI::GetAllSkinObjs() {
  this->AllSkinObjs.Empty();
  this->AllSkinObjs_Map.Empty();
  FARFilter LFilter{};
  LFilter.bIncludeOnlyOnDiskAssets = false;
  LFilter.bRecursivePaths          = true;
  LFilter.bRecursiveClasses        = true;
  LFilter.ClassNames.Add(FName{USkeleton::StaticClass()->GetName()});
  IAssetRegistry::Get()->EnumerateAssets(LFilter, [this](const FAssetData& InAss) -> bool {
    USkeleton* L_SK = Cast<USkeleton>(InAss.GetAsset());
    if (L_SK) {
      this->AllSkinObjs.Add(L_SK);
    }
    return true;
  });
  LFilter.ClassNames.Add(FName{USkeletalMesh::StaticClass()->GetName()});

  FString DeBug_Str{};
  IAssetRegistry::Get()->EnumerateAssets(LFilter, [&, this](const FAssetData& InAss) -> bool {
    USkeletalMesh* L_SK = Cast<USkeletalMesh>(InAss.GetAsset());
    if (L_SK && L_SK->GetSkeleton()) {
      FString L_BaseName = FPaths::GetBaseFilename(L_SK->GetAssetImportData()->GetFirstFilename());
      this->AllSkinObjs_Map.Add(MakeTuple(L_BaseName, L_SK->GetSkeleton()));
      DeBug_Str += L_SK->GetAssetImportData()->GetFirstFilename();
      DeBug_Str += " ";
      DeBug_Str += L_BaseName;
      DeBug_Str += "\n";
    }
    return true;
  });
  Debug_To_File(DeBug_Str);
}

void SDoodleImportFbxUI::MatchFbx() {
  UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();
  FbxImporter->ClearAllCaches();

  for (auto&& L_Fbx_Path : ListImportFbxData) {
    grop_SDoodleImportFbxUI L_grop_SDoodleImportFbxUI{FbxImporter};
    FbxImporter->ImportFromFile(L_Fbx_Path->ImportFbxPath, FPaths::GetExtension(L_Fbx_Path->ImportFbxPath));

    FString DeBug_Str{};
    TArray<fbxsdk::FbxNode*> L_Fbx_Node_list{};
    FindSkeletonNode(FbxImporter->Scene->GetRootNode(), L_Fbx_Node_list);
    for (auto&& L_1 : L_Fbx_Node_list) {
      L_1->GetNameSpaceOnly();
      FString L_Name = FSkeletalMeshImportData::FixupBoneName(MakeName(L_1->GetName()));
      DeBug_Str += "\n";
      DeBug_Str += L_Name;
      DeBug_Str += " ";
      FString L_NameSpace = GetNamepace(L_1->GetName());
      DeBug_Str += L_NameSpace;
      DeBug_Str += " ";
      DeBug_Str += L_1->GetName();
      if (this->AllSkinObjs_Map.Contains(L_NameSpace)) {
        L_Fbx_Path->SkinObj = this->AllSkinObjs_Map[L_NameSpace];
        break;
      }
    }
    Debug_To_File(DeBug_Str);
  }
}

#undef LOCTEXT_NAMESPACE