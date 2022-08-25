#include "MapGenerator/LoadSkeletonAnimationUI.h"

#include "Styling/SlateTypes.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SEditableText.h"

#include "Engine.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Editor/UnrealEd/Public/EditorDirectories.h"
#include "AssetRegistryModule.h"
#include "LevelSequence.h"
#include "FileHelpers.h"

#include "MapGenerator/CreateMap.h"
#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/LoadBP.h"

#define LOCTEXT_NAMESPACE "SLoadSkeletonAnimationUI"
const FText GoodbyeWorld = LOCTEXT("GoodbyeWorld", "Goodbye World!");

void SLoadSkeletonAnimationUI::Construct(const FArguments &InArgs) {
  FSlateFontInfo SlateFontInfoContent = FSlateFontInfo(GEditor->GetSmallFont(), 14);

  ChildSlot
      [

          SNew(SBorder)
              .BorderBackgroundColor(FLinearColor(0.3, 0.3, 0.3, 1))
              .BorderImage(new FSlateBrush())
                  [SNew(SCanvas)
                   // Title
                   + SCanvas::Slot()
                         .Position(FVector2D(700, 60))
                         .Size(FVector2D(800, 100))
                         .HAlign(HAlign_Center)
                         .VAlign(VAlign_Center)
                             [SNew(STextBlock)
                                  .Text(FText::FromString(TEXT("UE4 Load Skeleton Animation Tool")))
                                  .Font(FSlateFontInfo(GEditor->GetSmallFont(), 26))
                                  .ColorAndOpacity(FLinearColor::Black)]

                   // Save Settings
                   + SCanvas::Slot()
                         .Position(FVector2D(100, 60))
                         .Size(FVector2D(920, 300))
                             [SNew(SVerticalBox)
                              // Content Row1
                              + SVerticalBox::Slot().Padding(5, 5).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).Padding(2, 2)[SNew(STextBlock).Font(FSlateFontInfo(GEditor->GetSmallFont(), 22)).Text(FText::FromString(TEXT("Tools Setting:"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(5.5).Padding(2, 2)

  ]

                              // Working Project Info
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Project:"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)
                                           [SAssignNew(TextPorject, SEditableTextBox)
                                                .Font(SlateFontInfoContent)
                                                .Text(FText::FromString(TEXT("LJZ_VFX_project/EP000")))] +
                                     SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                         [SNew(SButton)
                                              .Text(FText::FromString(TEXT("Choose Dir")))
                                              .VAlign(VAlign_Center)
                                              .HAlign(HAlign_Center)
                                              .OnClicked(this, &SLoadSkeletonAnimationUI::OpenProjcetDir)]]

                              // LevelStreaming Info
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Skeleton Asset :"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)
                                           [SAssignNew(TextAsset, SEditableTextBox)
                                                .Font(SlateFontInfoContent)] +
                                     SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                         [SNew(SButton)
                                              .Text(FText::FromString(TEXT("Choose Asset")))
                                              .VAlign(VAlign_Center)
                                              .HAlign(HAlign_Center)
                                              .OnClicked(this, &SLoadSkeletonAnimationUI::ChooseAsset)]]

                              // Map Save Path
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Map Save Path :"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(.6).Padding(2, 2).HAlign(HAlign_Right)
                                           [SNew(STextBlock)
                                                .Font(SlateFontInfoContent)
                                                .Text(FText::FromString(TEXT("Map")))
                                                .ColorAndOpacity(FLinearColor::Black)] +
                                     SHorizontalBox::Slot().FillWidth(.2).Padding(2, 2)
                                         [SAssignNew(CheckBoxMap, SCheckBox)
                                              .IsChecked(ECheckBoxState::Checked)
                                              .BorderBackgroundColor(FLinearColor::White)
                                              .OnCheckStateChanged(this, &SLoadSkeletonAnimationUI::SaveInMap)] +
                                     SHorizontalBox::Slot().FillWidth(.6).Padding(2, 2).HAlign(HAlign_Right)
                                         [SNew(STextBlock)
                                              .Font(SlateFontInfoContent)
                                              .Text(FText::FromString(TEXT("Shot")))
                                              .ColorAndOpacity(FLinearColor::Black)] +
                                     SHorizontalBox::Slot().FillWidth(.2).Padding(2, 2)
                                         [SAssignNew(CheckBoxShot, SCheckBox)
                                              .OnCheckStateChanged(this, &SLoadSkeletonAnimationUI::SaveInShot)] +
                                     SHorizontalBox::Slot().FillWidth(0.8).Padding(2, 2)

                                     + SHorizontalBox::Slot().FillWidth(1.6).Padding(2, 2)
                                           [SNew(STextBlock)
                                                .Font(SlateFontInfoContent)
                                                .Text(FText::FromString(TEXT("Animation Start Frame:")))
                                                .ColorAndOpacity(FLinearColor::Black)] +
                                     SHorizontalBox::Slot().FillWidth(0.45).Padding(2, 2).HAlign(HAlign_Left)
                                         [SAssignNew(TextStartFrame, SEditableTextBox)
                                              .Font(SlateFontInfoContent)
                                              .Text(FText::FromString(TEXT("71")))
                                              .MinDesiredWidth(100)]

                                     + SHorizontalBox::Slot().FillWidth(1.05).Padding(2, 2)]

  ]

                   // Listview label
                   + SCanvas::Slot()
                         .Position(FVector2D(100, 280))
                         .Size(FVector2D(920, 50))
                             [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Map Info:"))).ColorAndOpacity(FLinearColor::Black)] + SHorizontalBox::Slot().FillWidth(2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Sequence Info:"))).ColorAndOpacity(FLinearColor::Black)]

  ]

                   // Listview Content
                   + SCanvas::Slot()
                         .Position(FVector2D(100, 310))
                         .Size(FVector2D(920, 300))
                             [SNew(SBorder)
                                  .BorderBackgroundColor(FLinearColor(0.75, 0.75, 0.75, 1))
                                  .BorderImage(new FSlateBrush())
                                      [

                                          SNew(SScrollBox).ScrollBarAlwaysVisible(true) + SScrollBox::Slot()
                                                                                              [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1)

                                                                                                                          [SAssignNew(ListViewMapInfo, SListView<TSharedPtr<FMapInfo>>).ItemHeight(24).ListItemsSource(&ItemsMapInfo).SelectionMode(ESelectionMode::Multi).OnGenerateRow(this, &SLoadSkeletonAnimationUI::GenerateMapInfoList).OnSelectionChanged(this, &SLoadSkeletonAnimationUI::ShowSequence)] +
                                                                                               SHorizontalBox::Slot().FillWidth(0.025)

                                                                                                   [SNew(SBorder)
                                                                                                        .BorderBackgroundColor(FLinearColor::Blue)
                                                                                                        .VAlign(VAlign_Fill)
                                                                                                        .DesiredSizeScale(FVector2D(1, 75))] +
                                                                                               SHorizontalBox::Slot().FillWidth(2)
                                                                                                   [SAssignNew(ListViewSequence, SListView<TSharedPtr<FString>>)
                                                                                                        .ItemHeight(24)
                                                                                                        .ListItemsSource(&ItemsEmpty)
                                                                                                        .OnGenerateRow(this, &SLoadSkeletonAnimationUI::GenerateList)

  ]

  ]]

  ]

                   + SCanvas::Slot()
                         .Position(FVector2D(450, 675))
                         .Size(FVector2D(200, 75))
                         .VAlign(VAlign_Center)
                             [SNew(SButton)
                                  .Text(FText::FromString(FString("Load Anim")))
                                  .TextStyle(&FTextBlockStyle().SetFont(FSlateFontInfo(GEditor->GetSmallFont(), 25)).SetColorAndOpacity(FSlateColor(FLinearColor::Black)))
                                  .HAlign(HAlign_Center)
                                  .VAlign(VAlign_Center)
                                  .OnClicked(this, &SLoadSkeletonAnimationUI::LoadSkeletonAnim)]]];
}

// Button Action OpenProjectDir

FReply SLoadSkeletonAnimationUI::OpenProjcetDir() {
  FString OpenDirectory;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  bool bOpen                        = false;

  if (DesktopPlatform) {
    bOpen = DesktopPlatform->OpenDirectoryDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        NSLOCTEXT("MapCreateTool", "", "").ToString(),
        *FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()),
        OpenDirectory
    );
  }

  if (bOpen) {
    UE_LOG(LogTemp, Log, TEXT("工程目录 %s"), *FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()));
    if (OpenDirectory.Contains(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()))) {
      TextPorject->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), TEXT(""), ESearchCase::IgnoreCase)));
      ItemsUpdateContent();
    }
  }

  return FReply::Handled();
}

// Button Action Open FbxDir
FReply SLoadSkeletonAnimationUI::ChooseAsset() {
  TArray<FString> OpenFilenames;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  if (IFileManager::Get().DirectoryExists(*TextAsset->GetText().ToString()))
    DefaultOpenFbxDir = TextAsset->GetText().ToString();

  bool bOpen = false;
  FString SceneMap;

  if (DesktopPlatform) {
    FString ExtensionStr;
    ExtensionStr += TEXT("UAsset (*.uasset)|*.uasset|");

    bOpen = DesktopPlatform->OpenFileDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        NSLOCTEXT("MapCreateTool", "", "").ToString(),
        *DefaultOpenFileDir,
        TEXT(""),
        *ExtensionStr,
        EFileDialogFlags::None,
        OpenFilenames
    );
  }

  if (bOpen) {
    DefaultOpenFileDir = FPaths::GetPath(OpenFilenames[0]);
    SceneMap           = OpenFilenames[0];

    FString DisplaySceneMap;

    if (SceneMap.Contains(FPaths::ProjectContentDir())) {
      DisplaySceneMap = FConvertPath::ToRelativePath(SceneMap, false);
      for (auto Item : ItemsSequence) {
        *Item = DisplaySceneMap;
      }
    }

    TextAsset->SetText(FText::FromString(DisplaySceneMap));
  }
  return FReply::Handled();
}

TSharedRef<ITableRow> SLoadSkeletonAnimationUI::GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase> &OwnerTable) {
  return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
      .Padding(2.0f)
          [SNew(STextBlock)
               .ColorAndOpacity(FLinearColor(0, 0, 0, 1))
               .Text(FText::FromString(*Item))
           //		.Font(FSlateFontInfo(GEditor->GetSmallFont(), 12))
  ];
}

TSharedRef<ITableRow> SLoadSkeletonAnimationUI::GenerateMapInfoList(TSharedPtr<FMapInfo> Item, const TSharedRef<STableViewBase> &OwnerTable) {
  return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
      .Padding(2.0f)
          [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).ColorAndOpacity(FLinearColor(0, 0, 0, 1)).Text(FText::FromString(*FConvertPath::GetPackageName(Item->MapPackage)))] + SHorizontalBox::Slot().FillWidth(0.1)[SNew(SCheckBox).ForegroundColor(FLinearColor(0.1, 0.1, 0.2, 1)).IsChecked(Item->bLoaded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked).OnCheckStateChanged(this, &SLoadSkeletonAnimationUI::SetMapLoad)]
           //		.Font(FSlateFontInfo(GEditor->GetSmallFont(), 12))
  ];
}

void SLoadSkeletonAnimationUI::SetMapLoad(ECheckBoxState State) {
  TArray<TSharedPtr<FMapInfo>> CurrentSelcetedItems = ListViewMapInfo->GetSelectedItems();

  for (auto Item : CurrentSelcetedItems) {
    if (Item->bLoaded)
      Item->bLoaded = false;
    else
      Item->bLoaded = true;
  }
  ListViewMapInfo->RebuildList();
  ListViewMapInfo->RequestListRefresh();
}

void SLoadSkeletonAnimationUI::SaveInMap(ECheckBoxState State) {
  IsSaveInMap = CheckBoxMap->IsChecked();
  if (IsSaveInMap)
    CheckBoxShot->SetIsChecked(ECheckBoxState::Unchecked);
  else
    CheckBoxShot->SetIsChecked(ECheckBoxState::Checked);

  ItemsUpdateContent();
}

void SLoadSkeletonAnimationUI::SaveInShot(ECheckBoxState State) {
  IsSaveInMap = !CheckBoxShot->IsChecked();
  if (IsSaveInMap)
    CheckBoxMap->SetIsChecked(ECheckBoxState::Checked);
  else
    CheckBoxMap->SetIsChecked(ECheckBoxState::Unchecked);

  ItemsUpdateContent();
}

void SLoadSkeletonAnimationUI::ItemsUpdateContent() {
  // TArray<TSharedPtr<FMapInfo>> ItemsMapInfoNew;
  // TArray<TSharedPtr<FString>>  ItemsSequenceNew, ItemsMapPackageNew;

  ItemsMapInfo.Reset();

  // Load AnimSequecne From Anim Directory in ProjectPath
  UObjectLibrary *WorldLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, GIsEditor);
  if (WorldLibrary) {
    WorldLibrary->AddToRoot();
  }

  FString ProjectPath = TextPorject->GetText().ToString();
  FString WorldAssetsPath;

  if (IsSaveInMap) {
    WorldAssetsPath = "/Game/" + ProjectPath + "/map";
    WorldLibrary->LoadAssetDataFromPath(*WorldAssetsPath);
    TArray<FAssetData> WorldAssetsData;
    WorldLibrary->GetAssetDataList(WorldAssetsData);
    for (auto World : WorldAssetsData) {
      ItemsMapInfo.Add(MakeShareable(new FMapInfo(World.PackageName.ToString())));
    }
  } else {
    TArray<FString> SubDirs;
    FString FinalPath = FPaths::ProjectContentDir() + "/" + ProjectPath + "/" + TEXT("*");
    IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);
    for (auto ShotDir : SubDirs) {
      if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase)) {
        WorldAssetsPath = "/Game/" + ProjectPath + "/" + ShotDir;
        WorldLibrary->LoadAssetDataFromPath(*WorldAssetsPath);
        TArray<FAssetData> WorldAssetsData;
        WorldLibrary->GetAssetDataList(WorldAssetsData);
        for (auto World : WorldAssetsData) {
          ItemsMapInfo.Add(MakeShareable(new FMapInfo(World.PackageName.ToString())));
        }
      }
    }
  }
  if (ItemsMapInfo.Num()) {
    ItemsAllSequence.Reset();

    FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    for (int i = 0; i < ItemsMapInfo.Num(); i++) {
      TArray<FName> SoftdReferencers;
      AssetRegistryModule.Get().GetReferencers(
          FName(*ItemsMapInfo[i]->MapPackage),
          SoftdReferencers,
          UE::AssetRegistry::EDependencyCategory::All  // EAssetRegistryDependencyType::Soft
      );
      TArray<TSharedPtr<FString>> ItemsSequenceName, ItemsSequencePackage;

      if (SoftdReferencers.Num()) {
        for (auto Referencer : SoftdReferencers) {
          ULevelSequence *Sequence = LoadObject<ULevelSequence>(NULL, *Referencer.ToString());
          if (Sequence != nullptr) {
            ItemsSequencePackage.Add(MakeShareable(new FString(Referencer.ToString())));
            TArray<FString> NameSplit;
            Referencer.ToString().ParseIntoArray(NameSplit, TEXT("/"), true);
            ItemsSequenceName.Add(MakeShareable(new FString(NameSplit[NameSplit.Num() - 1])));
          }
        }
      }
      ItemsAllSequence.Add(ItemsSequenceName);
      ItemsAllSequencePackage.Add(ItemsSequencePackage);
    }
  }

  ListViewSequence->SetListItemsSource(ItemsEmpty);
  ListViewSequence->RequestListRefresh();

  // GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *FString::FromInt(ItemsMapInfoNew.Num()));
  ListViewMapInfo->RebuildList();
  ListViewMapInfo->RequestListRefresh();

  //	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *FString::FromInt(ItemsMap.Num()));
}

void SLoadSkeletonAnimationUI::ShowSequence(TSharedPtr<FMapInfo> Item, ESelectInfo::Type Direct) {
  TArray<TSharedPtr<FMapInfo>> CurrentSelcetedItem = ListViewMapInfo->GetSelectedItems();

  int32 index;
  if (CurrentSelcetedItem.Num() == 1) {
    if (ItemsMapInfo.Find(CurrentSelcetedItem[0], index))
      ListViewSequence->SetListItemsSource(ItemsAllSequence[index]);
  } else
    ListViewSequence->SetListItemsSource(ItemsEmpty);

  ListViewSequence->RequestListRefresh();
}

FReply SLoadSkeletonAnimationUI::LoadSkeletonAnim() {
  FString SkeletonPackage = TextAsset->GetText().ToString();
  if (SkeletonPackage != "") {
    if (ItemsMapInfo.Num()) {
      FString ProjectPath = TextPorject->GetText().ToString();

      int32 StartFrame    = FCString::Atoi(*TextStartFrame->GetText().ToString());
      if (StartFrame == 0)
        StartFrame = 71;

      for (int32 i = 0; i < ItemsMapInfo.Num(); i++)
        if ((ItemsAllSequencePackage[i].Num()) && (ItemsMapInfo[i]->bLoaded)) {
          FString MapPackage = ItemsMapInfo[i]->MapPackage;
          FString MapName    = FConvertPath::GetPackageName(MapPackage);
          FLoadBP::LoadSkeletonAnim(SkeletonPackage, ProjectPath, MapPackage, MapName, *ItemsAllSequencePackage[i][0], StartFrame);
        }
    }
  }
  return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);
