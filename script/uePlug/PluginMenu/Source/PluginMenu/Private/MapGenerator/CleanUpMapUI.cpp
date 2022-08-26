#include "MapGenerator/CleanUpMapUI.h"

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
#include "LevelSequence/Public/LevelSequence.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"

#include "MapGenerator/CreateMap.h"
#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/LoadBP.h"
#include "MapGenerator/CreateMapUtils.h"

#define LOCTEXT_NAMESPACE "SCleanUpMapUI"

void SCleanUpMapUI::Construct(const FArguments &InArgs)
{
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
                                  .Text(FText::FromString("UE4 Clean Up Map Tool"))
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
                                              .OnClicked(this, &SCleanUpMapUI::OpenProjcetDir)]]

                              // LevelStreaming Info
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Output Path :"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)
                                           [SAssignNew(TextOutput, SEditableTextBox)
                                                .Font(SlateFontInfoContent)] +
                                     SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                         [SNew(SButton)
                                              .Text(FText::FromString(TEXT("Choose Path")))
                                              .VAlign(VAlign_Center)
                                              .HAlign(HAlign_Center)
                                              .OnClicked(this, &SCleanUpMapUI::OpenOutputDir)]]

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
                                              .OnCheckStateChanged(this, &SCleanUpMapUI::SaveInMap)] +
                                     SHorizontalBox::Slot().FillWidth(.6).Padding(2, 2).HAlign(HAlign_Right)
                                         [SNew(STextBlock)
                                              .Font(SlateFontInfoContent)
                                              .Text(FText::FromString(TEXT("Shot")))
                                              .ColorAndOpacity(FLinearColor::Black)] +
                                     SHorizontalBox::Slot().FillWidth(.2).Padding(2, 2)
                                         [SAssignNew(CheckBoxShot, SCheckBox)
                                              .OnCheckStateChanged(this, &SCleanUpMapUI::SaveInShot)] +
                                     SHorizontalBox::Slot().FillWidth(1.2).Padding(2, 2)

                                     + SHorizontalBox::Slot().FillWidth(1).Padding(2, 2).HAlign(HAlign_Right)
                                           [SNew(STextBlock)
                                                .Font(SlateFontInfoContent)
                                                .Text(FText::FromString(TEXT("Overwrite")))
                                                .ColorAndOpacity(FLinearColor::Black)] +
                                     SHorizontalBox::Slot().FillWidth(.2).Padding(2, 2)
                                         [SAssignNew(CheckBoxOverwrite, SCheckBox)
                                              .IsChecked(ECheckBoxState::Checked)
                                              .BorderBackgroundColor(FLinearColor::White)] +
                                     SHorizontalBox::Slot().FillWidth(1.5).Padding(2, 2)

  ]

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

                                                                                                                          [SAssignNew(ListViewMapInfo, SListView<TSharedPtr<FMapInfo>>).ItemHeight(24).ListItemsSource(&ItemsMapInfo).SelectionMode(ESelectionMode::Multi).OnGenerateRow(this, &SCleanUpMapUI::GenerateMapInfoList).OnSelectionChanged(this, &SCleanUpMapUI::ShowSequence)] +
                                                                                               SHorizontalBox::Slot().FillWidth(0.025)

                                                                                                   [SNew(SBorder)
                                                                                                        .BorderBackgroundColor(FLinearColor::Blue)
                                                                                                        .VAlign(VAlign_Fill)
                                                                                                        .DesiredSizeScale(FVector2D(1, 75))] +
                                                                                               SHorizontalBox::Slot().FillWidth(2)
                                                                                                   [SAssignNew(ListViewSequence, SListView<TSharedPtr<FString>>)
                                                                                                        .ItemHeight(24)
                                                                                                        .ListItemsSource(&ItemsEmpty)
                                                                                                        .OnGenerateRow(this, &SCleanUpMapUI::GenerateList)

  ]

  ]]

  ]

                   + SCanvas::Slot()
                         .Position(FVector2D(450, 675))
                         .Size(FVector2D(200, 75))
                         .VAlign(VAlign_Center)
                             [SNew(SButton)
                                  .Text(FText::FromString(FString("Clean Up")))
                                  .TextStyle(&FTextBlockStyle().SetFont(FSlateFontInfo(GEditor->GetSmallFont(), 25)).SetColorAndOpacity(FSlateColor(FLinearColor::Black)))
                                  .HAlign(HAlign_Center)
                                  .VAlign(VAlign_Center)
                                  .OnClicked(this, &SCleanUpMapUI::CleanUpMap)]]];
}

// Button Action OpenProjectDir
FReply SCleanUpMapUI::OpenProjcetDir()
{
  FString OpenDirectory;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  bool bOpen = false;

  if (DesktopPlatform)
  {
    bOpen = DesktopPlatform->OpenDirectoryDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        NSLOCTEXT("MapCreateTool", "", "").ToString(),
        *DefaultOpenProjectDir,
        OpenDirectory);
  }

  if (bOpen)
  {
    if (OpenDirectory.Contains(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir())))
    {
      DefaultOpenProjectDir = OpenDirectory;
      TextPorject->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), TEXT(""), ESearchCase::IgnoreCase)));
      TextOutput->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), TEXT(""), ESearchCase::IgnoreCase) + "/map"));
      ItemsUpdateContent();
    }
  }

  return FReply::Handled();
}

// Button Action Open FbxDir
FReply SCleanUpMapUI::OpenOutputDir()
{
  FString OpenDirectory;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  bool bOpen = false;
  FString SceneMap;

  if (DesktopPlatform)
  {
    bOpen = DesktopPlatform->OpenDirectoryDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        NSLOCTEXT("MapCreateTool", "", "").ToString(),
        *DefaultOpenOutputDir,
        OpenDirectory);
  }

  if (bOpen)
  {
    if (OpenDirectory.Contains(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir())))
    {
      DefaultOpenOutputDir = OpenDirectory;
      TextOutput->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), TEXT(""), ESearchCase::IgnoreCase)));
    }
  }
  return FReply::Handled();
}

TSharedRef<ITableRow> SCleanUpMapUI::GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase> &OwnerTable)
{
  return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
      .Padding(2.0f)
          [SNew(STextBlock)
               .ColorAndOpacity(FLinearColor(0, 0, 0, 1))
               .Text(FText::FromString(*Item))
           //		.Font(FSlateFontInfo(GEditor->GetSmallFont(), 12))
  ];
}

TSharedRef<ITableRow> SCleanUpMapUI::GenerateMapInfoList(TSharedPtr<FMapInfo> Item, const TSharedRef<STableViewBase> &OwnerTable)
{
  return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
      .Padding(2.0f)
          [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).ColorAndOpacity(FLinearColor(0, 0, 0, 1)).Text(FText::FromString(*FConvertPath::GetPackageName(Item->MapPackage)))] + SHorizontalBox::Slot().FillWidth(0.1)[SNew(SCheckBox).ForegroundColor(FLinearColor(0.1, 0.1, 0.2, 1)).IsChecked(Item->bLoaded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked).OnCheckStateChanged(this, &SCleanUpMapUI::SetMapLoad)]
           //		.Font(FSlateFontInfo(GEditor->GetSmallFont(), 12))
  ];
}

void SCleanUpMapUI::SetMapLoad(ECheckBoxState State)
{
  TArray<TSharedPtr<FMapInfo>> CurrentSelcetedItems = ListViewMapInfo->GetSelectedItems();

  for (auto Item : CurrentSelcetedItems)
  {
    if (Item->bLoaded)
      Item->bLoaded = false;
    else
      Item->bLoaded = true;
  }
  ListViewMapInfo->RebuildList();
  ListViewMapInfo->RequestListRefresh();
}

void SCleanUpMapUI::SaveInMap(ECheckBoxState State)
{
  IsSaveInMap = CheckBoxMap->IsChecked();
  if (IsSaveInMap)
    CheckBoxShot->SetIsChecked(ECheckBoxState::Unchecked);
  else
    CheckBoxShot->SetIsChecked(ECheckBoxState::Checked);

  ItemsUpdateContent();
}

void SCleanUpMapUI::SaveInShot(ECheckBoxState State)
{
  IsSaveInMap = !CheckBoxShot->IsChecked();
  if (IsSaveInMap)
    CheckBoxMap->SetIsChecked(ECheckBoxState::Checked);
  else
    CheckBoxMap->SetIsChecked(ECheckBoxState::Unchecked);

  ItemsUpdateContent();
}

void SCleanUpMapUI::ItemsUpdateContent()
{
  // TArray<TSharedPtr<FMapInfo>> ItemsMapInfoNew;
  // TArray<TSharedPtr<FString>>  ItemsSequenceNew, ItemsMapPackageNew;

  ItemsMapInfo.Reset();

  // Load AnimSequecne From Anim Directory in ProjectPath
  UObjectLibrary *WorldLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, GIsEditor);
  if (WorldLibrary)
  {
    WorldLibrary->AddToRoot();
  }

  FString ProjectPath = TextPorject->GetText().ToString();
  FString WorldAssetsPath;

  if (IsSaveInMap)
  {
    WorldAssetsPath = "/Game/" + ProjectPath + "/map";
    WorldLibrary->LoadAssetDataFromPath(*WorldAssetsPath);
    TArray<FAssetData> WorldAssetsData;
    WorldLibrary->GetAssetDataList(WorldAssetsData);
    for (auto World : WorldAssetsData)
    {
      ItemsMapInfo.Add(MakeShareable(new FMapInfo(World.PackageName.ToString())));
    }
  }
  else
  {
    TArray<FString> SubDirs;
    FString FinalPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "/" + ProjectPath + "/" + TEXT("*");
    IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);
    for (auto ShotDir : SubDirs)
    {
      if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase))
      {
        WorldAssetsPath = "/Game/" + ProjectPath + "/" + ShotDir;
        WorldLibrary->LoadAssetDataFromPath(*WorldAssetsPath);
        TArray<FAssetData> WorldAssetsData;
        WorldLibrary->GetAssetDataList(WorldAssetsData);
        for (auto World : WorldAssetsData)
        {
          ItemsMapInfo.Add(MakeShareable(new FMapInfo(World.PackageName.ToString())));
        }
      }
    }
  }
  if (ItemsMapInfo.Num())
  {
    ItemsAllSequence.Reset();

    FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    for (int i = 0; i < ItemsMapInfo.Num(); i++)
    {
      TArray<FName> SoftdReferencers;
      AssetRegistryModule.Get().GetDependencies(
          FName(*ItemsMapInfo[i]->MapPackage),
          SoftdReferencers,
          UE::AssetRegistry::EDependencyCategory::All,
          UE::AssetRegistry::EDependencyQuery::Hard |
              UE::AssetRegistry::EDependencyQuery::Build |
              UE::AssetRegistry::EDependencyQuery::Game);

      AssetRegistryModule.Get().GetDependencies(
          FName(*ItemsMapInfo[i]->MapPackage),
          SoftdReferencers,
          UE::AssetRegistry::EDependencyCategory::All,
          UE::AssetRegistry::EDependencyQuery::Soft);

      TArray<TSharedPtr<FString>> ItemsSequenceName, ItemsSequencePackage;

      if (SoftdReferencers.Num())
      {
        for (auto Referencer : SoftdReferencers)
        {
          UE_LOG(LogTemp, Log, TEXT("%s"), *Referencer.ToString());
          ULevelSequence *Sequence = LoadObject<ULevelSequence>(NULL, *Referencer.ToString());
          if (Sequence != nullptr)
          {
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

  ListViewMapInfo->RebuildList();
  ListViewMapInfo->RequestListRefresh();
}

void SCleanUpMapUI::ShowSequence(TSharedPtr<FMapInfo> Item, ESelectInfo::Type Direct)
{
  TArray<TSharedPtr<FMapInfo>> CurrentSelcetedItem = ListViewMapInfo->GetSelectedItems();

  int32 index;
  if (CurrentSelcetedItem.Num() == 1)
  {
    if (ItemsMapInfo.Find(CurrentSelcetedItem[0], index))
      ListViewSequence->SetListItemsSource(ItemsAllSequence[index]);
  }
  else
    ListViewSequence->SetListItemsSource(ItemsEmpty);

  ListViewSequence->RequestListRefresh();
}

FReply SCleanUpMapUI::CleanUpMap()
{
  FString OutPath = TextOutput->GetText().ToString();
  bool bOverwrite = CheckBoxOverwrite->IsChecked();

  if (IFileManager::Get().DirectoryExists(*OutPath))
    IFileManager::Get().MakeDirectory(*OutPath, true);

  if (ItemsMapInfo.Num())
  {
    for (int32 i = 0; i < ItemsMapInfo.Num(); i++)
      if ((ItemsAllSequencePackage[i].Num()) && (ItemsMapInfo[i]->bLoaded))
      {
        FString MapPackage = ItemsMapInfo[i]->MapPackage;
        FCreateMapUtils::CleanUpMap(MapPackage, *ItemsAllSequencePackage[i][0], OutPath, bOverwrite);
      }
  }

  return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);
