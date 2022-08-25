#include "MapGenerator/SingleSceneMapUI.h"

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

#include "MapGenerator/CreateMap.h"
#include "MapGenerator/ConvertPath.h"

#ifdef LOCTEXT_NAMESPACE
#undef LOCTEXT_NAMESPACE
#endif

#define LOCTEXT_NAMESPACE "SSingleSceneMapUI"

void SSingleSceneMapUI::Construct(const FArguments &InArgs) {
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
                                  .Text(FText::FromString("UE4 Level Map Auto Create Tool"))
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

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)

                                     + SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                           [SNew(SButton)
                                                .Text(FText::FromString(TEXT("Save Settings")))
                                                .VAlign(VAlign_Center)
                                                .HAlign(HAlign_Center)
                                                .OnClicked(this, &SSingleSceneMapUI::SaveSetting)]]

                              // Working Project Info
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Project:"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)
                                           [

                                               SAssignNew(TextPorject, SEditableTextBox)
                                                   .Font(SlateFontInfoContent)
                                                   .Text(FText::FromString(TEXT("LJZ_VFX_project/EP000")))] +
                                     SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                         [SNew(SButton)
                                              .Text(FText::FromString(TEXT("Choose Dir")))
                                              .VAlign(VAlign_Center)
                                              .HAlign(HAlign_Center)
                                              .OnClicked(this, &SSingleSceneMapUI::OpenProjcetDir)]]

                              // LevelStreaming Info
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Fbx Camera Dir :"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)
                                           [SAssignNew(TextFbxDir, SEditableTextBox)
                                                .Font(SlateFontInfoContent)
                                                .Text(FText::FromString(TEXT("C:/")))] +
                                     SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                         [SNew(SButton)
                                              .Text(FText::FromString(TEXT("Choose Dir")))
                                              .VAlign(VAlign_Center)
                                              .HAlign(HAlign_Center)
                                              .OnClicked(this, &SSingleSceneMapUI::OpenFbxDir)]]

                              // Fbx Camera Dir
                              + SVerticalBox::Slot().Padding(2, 2).AutoHeight()
                                    [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).HAlign(HAlign_Center).Padding(2, 2)[SNew(STextBlock).Font(SlateFontInfoContent).Text(FText::FromString(TEXT("Scene Map :"))).ColorAndOpacity(FLinearColor::Black)]

                                     + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)
                                           [SAssignNew(TextSceneMap, SEditableTextBox)
                                                .Font(SlateFontInfoContent)
                                                .Text(FText::FromString(TEXT("Template_Default")))] +
                                     SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
                                         [SNew(SButton)
                                              .Text(FText::FromString(TEXT("Choose Map")))
                                              .OnClicked(this, &SSingleSceneMapUI::OpenSceneMap)
                                              .VAlign(VAlign_Center)
                                              .HAlign(HAlign_Center)]]

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
                                              .OnCheckStateChanged(this, &SSingleSceneMapUI::SaveInMap)] +
                                     SHorizontalBox::Slot().FillWidth(.6).Padding(2, 2).HAlign(HAlign_Right)
                                         [SNew(STextBlock)
                                              .Font(SlateFontInfoContent)
                                              .Text(FText::FromString(TEXT("Shot")))
                                              .ColorAndOpacity(FLinearColor::Black)] +
                                     SHorizontalBox::Slot().FillWidth(.2).Padding(2, 2)
                                         [SAssignNew(CheckBoxShot, SCheckBox)
                                              .OnCheckStateChanged(this, &SSingleSceneMapUI::SaveInShot)] +
                                     SHorizontalBox::Slot().FillWidth(3.9).Padding(2, 2)

  ]

  ]

                   + SCanvas::Slot()
                         .Position(FVector2D(420, 350))
                         .Size(FVector2D(250, 75))
                         .VAlign(VAlign_Center)
                             [SNew(SButton)
                                  .Text(FText::FromString(FString("Create Map")))
                                  .TextStyle(&FTextBlockStyle().SetFont(FSlateFontInfo(GEditor->GetSmallFont(), 25)).SetColorAndOpacity(FSlateColor(FLinearColor::Black)))
                                  .HAlign(HAlign_Center)
                                  .VAlign(VAlign_Center)
                                  .OnClicked(this, &SSingleSceneMapUI::CreateMap)]]];

  LoadSetting();
}

// Button Action SaveSetting
FReply SSingleSceneMapUI::SaveSetting() {
  WriteSetting();
  return FReply::Handled();
}

// Write Setting.json
void SSingleSceneMapUI::WriteSetting() {
  TSharedPtr<FJsonObject> RootObject  = MakeShareable(new FJsonObject);
  TSharedPtr<FJsonObject> EntryObject = MakeShareable(new FJsonObject);

  EntryObject->SetStringField(TEXT("Project"), *(TextPorject->GetText().ToString()));
  EntryObject->SetStringField(TEXT("Fbx Camera Dir"), *(TextFbxDir->GetText().ToString()));
  EntryObject->SetStringField(TEXT("Scene Map"), *(TextSceneMap->GetText().ToString()));
  EntryObject->SetBoolField(TEXT("Map Save Path"), true);

  TSharedPtr<FJsonValue> EntryValue = MakeShareable(new FJsonValueObject(EntryObject));
  TArray<TSharedPtr<FJsonValue>> JsonData;
  JsonData.Add(EntryValue);

  RootObject->SetArrayField(TEXT("Settings"), JsonData);
  FString JsonFileName = FPaths::ProjectDir() / TEXT("Settings.json");
  FArchive *JsonFile   = IFileManager::Get().CreateFileWriter(*JsonFileName, FILEWRITE_EvenIfReadOnly);

  if (JsonFile) {
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(JsonFile);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    JsonFile->Close();
  }
}

// If Setting.json Exist , Loading Settings
void SSingleSceneMapUI::LoadSetting() {
  FString JsonFileName = FPaths::ProjectDir() / TEXT("Settings.json");
  if (IFileManager::Get().FileExists(*JsonFileName)) {
    FArchive *JsonFile                    = IFileManager::Get().CreateFileReader(*JsonFileName);
    TSharedPtr<FJsonObject> RootObject    = MakeShareable(new FJsonObject);
    bool bJsonLoaded                      = false;

    TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonFile);
    bJsonLoaded                           = FJsonSerializer::Deserialize(Reader, RootObject);
    JsonFile->Close();

    if (bJsonLoaded) {
      TArray<TSharedPtr<FJsonValue>> JsonData = RootObject->GetArrayField(TEXT("Settings"));
      FString Project, FbxCameraDir, SceneMap;
      Project = JsonData[0]->AsObject()->GetStringField(TEXT("Project"));
      TextPorject->SetText(FText::FromString(Project));
      FbxCameraDir = JsonData[0]->AsObject()->GetStringField(TEXT("Fbx Camera Dir"));
      TextFbxDir->SetText(FText::FromString(FbxCameraDir));

      // SceneMap = JsonData[0]->AsObject()->GetStringField(TEXT("Scene Map"));
      // TextSceneMap->SetText(FText::FromString(SceneMap));

      // bool SavePath = JsonData[0]->AsObject()->GetBoolField(TEXT("Map Save Path"));
    }
  }
}

// Button Action OpenProjectDir
FReply SSingleSceneMapUI::OpenProjcetDir() {
  FString OpenDirectory;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  bool bOpen                        = false;

  if (DesktopPlatform) {
    bOpen = DesktopPlatform->OpenDirectoryDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        NSLOCTEXT("MapCreateTool", "", "").ToString(),
        *FPaths::ProjectContentDir(),
        OpenDirectory
    );
  }

  if (bOpen) {
    if (OpenDirectory.Contains(FPaths::ProjectContentDir())) {
      TextPorject->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ProjectContentDir(), TEXT(""), ESearchCase::IgnoreCase)));

      ItemsAddContent(OpenDirectory);
    }
  }

  return FReply::Handled();
}

// Button Action Open FbxDir
FReply SSingleSceneMapUI::OpenFbxDir() {
  FString OpenDirectoryName;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  if (IFileManager::Get().DirectoryExists(*TextFbxDir->GetText().ToString()))
    DefaultOpenFbxDir = TextFbxDir->GetText().ToString();

  bool bOpen = false;

  if (DesktopPlatform) {
    bOpen = DesktopPlatform->OpenDirectoryDialog(
        FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
        NSLOCTEXT("MapCreateTool", "", "").ToString(),
        *DefaultOpenFbxDir,
        OpenDirectoryName
    );
  }

  if (bOpen) {
    TextFbxDir->SetText(FText::FromString(OpenDirectoryName));
    DefaultOpenFbxDir = OpenDirectoryName;
  }
  return FReply::Handled();
}

// Button Action OpenSceneMap
FReply SSingleSceneMapUI::OpenSceneMap() {
  TArray<FString> OpenFilenames;
  IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

  bool bOpen                        = false;

  FString SceneMap;

  if (DesktopPlatform) {
    FString ExtensionStr;
    ExtensionStr += TEXT("Map (*.umap)|*.umap|");

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
      for (auto Item : ItemsScene) {
        *Item = DisplaySceneMap;
      }
    }

    TextSceneMap->SetText(FText::FromString(DisplaySceneMap));
  }

  return FReply::Handled();
}

// Check Map Exists
bool SSingleSceneMapUI::MapExist(bool SaveInMap, FString Shot, FString MapName) {
  bool bMapExist = false;
  FString SaveMapPath;
  if (SaveInMap)
    SaveMapPath = FPaths::ProjectContentDir() + TextPorject->GetText().ToString() + "/map";
  else
    SaveMapPath = FPaths::ProjectContentDir() + TextPorject->GetText().ToString() + "/" + Shot;

  FString MapFullPath = SaveMapPath + "/" + MapName + ".umap";

  if (IFileManager::Get().DirectoryExists(*SaveMapPath)) {
    bMapExist = IFileManager::Get().FileExists(*MapFullPath);
  }
  return bMapExist;
}

// Set Map Name
FString SSingleSceneMapUI::SetMapName(FString Shot) {
  FString ProjectName = TextPorject->GetText().ToString();
  FString MapName;
  TArray<FString> NameSplit;
  ProjectName.ParseIntoArray(NameSplit, TEXT("_"), true);
  if (NameSplit.Num())
    MapName = NameSplit[0] + "_" + TextPorject->GetText().ToString().Right(3) + "_" + Shot.Right(3) + "_VFX";
  return MapName;
}

// Add Map Info to Items Array
void SSingleSceneMapUI::ItemsAddContent(FString ProjectPath) {
  ItemsMap.Reset();
  ItemsShot.Reset();
  ItemsScene.Reset();

  TArray<FString> SubDirs;
  FString FinalPath = ProjectPath + "/" + TEXT("*");
  IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);
  for (auto ShotDir : SubDirs) {
    if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase)) {
      FString MapName = SetMapName(ShotDir);

      if (!MapExist(IsSaveInMap, ShotDir, MapName)) {
        ItemsMap.Add(MakeShareable(new FString(MapName)));
        ItemsShot.Add(MakeShareable(new FString(ShotDir)));
        ItemsScene.Add(MakeShareable(new FString(TextSceneMap->GetText().ToString())));
      }
    }
  }
}

void SSingleSceneMapUI::SaveInMap(ECheckBoxState State) {
  IsSaveInMap = CheckBoxMap->IsChecked();
  if (IsSaveInMap)
    CheckBoxShot->SetIsChecked(ECheckBoxState::Unchecked);
  else
    CheckBoxShot->SetIsChecked(ECheckBoxState::Checked);

  ItemsUpdateContent();
}

void SSingleSceneMapUI::SaveInShot(ECheckBoxState State) {
  IsSaveInMap = !CheckBoxShot->IsChecked();
  if (IsSaveInMap)
    CheckBoxMap->SetIsChecked(ECheckBoxState::Checked);
  else
    CheckBoxMap->SetIsChecked(ECheckBoxState::Unchecked);

  ItemsUpdateContent();
}

void SSingleSceneMapUI::ItemsUpdateContent() {
  TArray<TSharedPtr<FString>> ItemsMapNew, ItemsShotNew, ItemsSceneNew;

  TArray<FString> SubDirs;
  FString FinalPath = FPaths::ProjectContentDir() + "/" + TextPorject->GetText().ToString() + "/" + TEXT("*");
  IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);
  for (auto ShotDir : SubDirs) {
    if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase)) {
      FString MapName = SetMapName(ShotDir);
      if (!MapExist(IsSaveInMap, ShotDir, MapName)) {
        bool bFind = false;
        int32 Index;
        for (auto Item1 : ItemsMap) {
          if (MapName == *Item1) {
            bFind = true;
            ItemsMap.Find(Item1, Index);
            break;
          }
        }

        if (bFind) {
          ItemsMapNew.Add(ItemsMap[Index]);
          ItemsShotNew.Add(ItemsShot[Index]);
          ItemsSceneNew.Add(ItemsScene[Index]);
        } else {
          ItemsMapNew.Add(MakeShareable(new FString(MapName)));
          ItemsShot.Add(MakeShareable(new FString(ShotDir)));
          ItemsSceneNew.Add(MakeShareable(new FString(TextSceneMap->GetText().ToString())));
        }
      }
    }
  }

  ItemsMap   = ItemsMapNew;
  ItemsScene = ItemsSceneNew;
}

FReply SSingleSceneMapUI::CreateMap() {
  FString ProjectPath, FbxCameraPath;

  FbxCameraPath = TextFbxDir->GetText().ToString();
  ProjectPath   = TextPorject->GetText().ToString();

  if (ItemsMap.Num() == 0) {
    FString ProjectFullPath = FPaths::ProjectContentDir() + "/" + ProjectPath;
    ItemsAddContent(ProjectFullPath);
  }

  for (int i = 0; i < ItemsMap.Num(); i++) {
    FCreateMap::CreateMap(ProjectPath, FbxCameraPath, *ItemsShot[i], *ItemsMap[i], *ItemsScene[i], IsSaveInMap);
  }

  ItemsUpdateContent();

  return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);
