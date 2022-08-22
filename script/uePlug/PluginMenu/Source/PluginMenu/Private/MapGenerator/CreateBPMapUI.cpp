#include "MapGenerator/CreateBPMapUI.h"

#include "Styling/SlateTypes.h"
#include "SCanvas.h"
#include "SButton.h"
#include "SConstraintCanvas.h"
#include "SListView.h"
#include "SScrollBox.h"
#include "SEditableText.h"

#include "Engine.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Editor/UnrealEd/Public/EditorDirectories.h"

#include "MapGenerator/CreateMap.h"
#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/LoadBP.h"

#define LOCTEXT_NAMESPACE "SCreateBPMapUI"

void SCreateBPMapUI::Construct(const FArguments &InArgs)
{

	FSlateFontInfo SlateFontInfoContent = FSlateFontInfo("Arial", 14);
	FSlateFontInfo SlateSmallFontInfoContent = FSlateFontInfo("Arial", 12);

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
									.Font(FSlateFontInfo("Arial", 26))
									.ColorAndOpacity(FLinearColor::Black)]

					 // Save Settings
					 + SCanvas::Slot()
						   .Position(FVector2D(100, 60))
						   .Size(FVector2D(920, 300))
							   [SNew(SVerticalBox)
								// Content Row1
								+ SVerticalBox::Slot().Padding(5, 5).AutoHeight()
									  [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5).Padding(2, 2)[SNew(STextBlock).Font(FSlateFontInfo("Arial", 22)).Text(FText::FromString(TEXT("Tools Setting:"))).ColorAndOpacity(FLinearColor::Black)]

									   + SHorizontalBox::Slot().FillWidth(4.5).Padding(2, 2)

									   + SHorizontalBox::Slot().FillWidth(1).Padding(2, 2)
											 [SNew(SButton)
												  .Text(FText::FromString(TEXT("Save Settings")))
												  .VAlign(VAlign_Center)
												  .HAlign(HAlign_Center)
												  .OnClicked(this, &SCreateBPMapUI::SaveSetting)]]

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
												.OnClicked(this, &SCreateBPMapUI::OpenProjcetDir)]]

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
												.OnClicked(this, &SCreateBPMapUI::OpenFbxDir)]]

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
												.OnClicked(this, &SCreateBPMapUI::OpenSceneMap)
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
												.OnCheckStateChanged(this, &SCreateBPMapUI::SaveInMap)] +
									   SHorizontalBox::Slot().FillWidth(.6).Padding(2, 2).HAlign(HAlign_Right)
										   [SNew(STextBlock)
												.Font(SlateFontInfoContent)
												.Text(FText::FromString(TEXT("Shot")))
												.ColorAndOpacity(FLinearColor::Black)] +
									   SHorizontalBox::Slot().FillWidth(.2).Padding(2, 2)
										   [SAssignNew(CheckBoxShot, SCheckBox)
												.OnCheckStateChanged(this, &SCreateBPMapUI::SaveInShot)]

									   + SHorizontalBox::Slot().FillWidth(3.9).Padding(2, 2)

	]

	]

					 + SCanvas::Slot()
						   .Position(FVector2D(100, 280))
						   .Size(FVector2D(920, 20))
							   [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.02)[SNew(STextBlock).Font(SlateSmallFontInfoContent).Text(FText::FromString(TEXT("BP Reference:"))).ColorAndOpacity(FLinearColor::Black)]

								+ SHorizontalBox::Slot().FillWidth(1.02)
									  [SNew(STextBlock)
										   .Font(SlateSmallFontInfoContent)
										   .Text(FText::FromString(TEXT("Shot Map List:")))
										   .ColorAndOpacity(FLinearColor::Black)] +
								SHorizontalBox::Slot().FillWidth(1)
									[SNew(STextBlock)
										 .Font(SlateSmallFontInfoContent)
										 .Text(FText::FromString(TEXT("Scene Map List:")))
										 .ColorAndOpacity(FLinearColor::Black)]]

					 + SCanvas::Slot()
						   .Position(FVector2D(100, 305))
						   .Size(FVector2D(250, 300))
							   [SNew(SBorder)
									.BorderBackgroundColor(FLinearColor(0.75, 0.75, 0.75, 1))
									.BorderImage(new FSlateBrush())
										[

											SNew(SScrollBox).ScrollBarAlwaysVisible(true) + SScrollBox::Slot()
																								[SAssignNew(ListViewAllBP, SListView<TSharedPtr<FBPInfo>>)
																									 .ItemHeight(24)
																									 .ListItemsSource(&ItemsAllBP)
																									 .OnGenerateRow(this, &SCreateBPMapUI::GenerateBPList)]

	]]

					 + SCanvas::Slot()
						   .Position(FVector2D(410, 305))
						   .Size(FVector2D(610, 300))
							   [SNew(SBorder)
									.BorderBackgroundColor(FLinearColor(0.75, 0.75, 0.75, 1))
									.BorderImage(new FSlateBrush())
										[

											SNew(SScrollBox).ScrollBarAlwaysVisible(true) + SScrollBox::Slot()
																								[SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.2)

																															[SAssignNew(ListViewShot, SListView<TSharedPtr<FString>>).ItemHeight(24).ListItemsSource(&ItemsMap).OnGenerateRow(this, &SCreateBPMapUI::GenerateList).OnMouseButtonClick(this, &SCreateBPMapUI::LinkSelection_ClickL)

	] + SHorizontalBox::Slot().FillWidth(0.025)

																									 [SNew(SBorder).BorderBackgroundColor(FLinearColor::Blue).VAlign(VAlign_Fill).DesiredSizeScale(FVector2D(1, 75))] +
																								 SHorizontalBox::Slot().FillWidth(1.8)
																									 [SAssignNew(ListViewScene, SListView<TSharedPtr<FString>>)
																										  .ItemHeight(24)
																										  .ListItemsSource(&ItemsScene)
																										  //.OnMouseButtonClick(this, &SCreateBPMapUI::LinkSelection_ClickR)
																										  .OnGenerateRow(this, &SCreateBPMapUI::GenerateList)

	]

	]]

	]

					 + SCanvas::Slot()
						   .Position(FVector2D(100, 610))
						   .Size(FVector2D(920, 35))
							   [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(0.4)

								+ SHorizontalBox::Slot().FillWidth(0.4)
									  [SNew(SButton)
										   .Text(FText::FromString(FString("Show BP Assets")))
										   .HAlign(HAlign_Center)
										   .VAlign(VAlign_Center)
										   .OnClicked(this, &SCreateBPMapUI::ShowBPAssets)]

								+ SHorizontalBox::Slot().FillWidth(0.6)

								+ SHorizontalBox::Slot().FillWidth(.4).Padding(2, 0)
									  [SNew(SButton)
										   .OnClicked(this, &SCreateBPMapUI::ShowMapInfo)
										   .Text(FText::FromString(FString("Show Map Info")))
										   .HAlign(HAlign_Center)
										   .VAlign(VAlign_Center)

	] + SHorizontalBox::Slot().FillWidth(0.4)

								+ SHorizontalBox::Slot().FillWidth(0.4).Padding(2, 0)
									  [SNew(SButton)
										   .OnClicked(this, &SCreateBPMapUI::ChangeSceneMap)
										   .Text(FText::FromString(FString("Change Scene Map")))
										   .HAlign(HAlign_Center)
										   .VAlign(VAlign_Center)]

								+ SHorizontalBox::Slot().FillWidth(0.4).Padding(2, 0)
									  [SNew(SButton)
										   .OnClicked(this, &SCreateBPMapUI::RestSceneMap)
										   .Text(FText::FromString(FString("Reset Default")))
										   .HAlign(HAlign_Center)
										   .VAlign(VAlign_Center)]

	]

					 + SCanvas::Slot()
						   .Position(FVector2D(480, 690))
						   .Size(FVector2D(200, 60))
						   .VAlign(VAlign_Center)
							   [SNew(SButton)
									.Text(FText::FromString(FString("Create Map")))
									.TextStyle(&FTextBlockStyle().SetFont(FSlateFontInfo("Arial", 20)).SetColorAndOpacity(FSlateColor(FLinearColor::Black)))
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.OnClicked(this, &SCreateBPMapUI::CreateMap)]]];

	LoadSetting();
}

// Listview GenarateRow Action
TSharedRef<ITableRow> SCreateBPMapUI::GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase> &OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(2.0f)
			[SNew(STextBlock)
				 .ColorAndOpacity(FLinearColor(0, 0, 0, 1))
				 .Text(FText::FromString(*Item))];
}

TSharedRef<ITableRow> SCreateBPMapUI::GenerateBPList(TSharedPtr<FBPInfo> BPItem, const TSharedRef<STableViewBase> &OwnerTable)
{

	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(2.0f)
			[SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.2)[SNew(STextBlock).ColorAndOpacity(FLinearColor(0, 0, 0, 1)).Text(FText::FromString(*FConvertPath::GetPackageName(BPItem->BPPackage)))] + SHorizontalBox::Slot().FillWidth(0.3)[SNew(SEditableText).ColorAndOpacity(FLinearColor(0, 0, 0, 1)).Text(FText::FromString(*FString::FromInt(BPItem->StartFrame))).OnTextCommitted(this, &SCreateBPMapUI::SetBPAnimStart1)] + SHorizontalBox::Slot().FillWidth(0.1)[SNew(SCheckBox).ForegroundColor(FLinearColor(0.1, 0.1, 0.2, 1)).IsChecked(BPItem->bLoaded ? ECheckBoxState::Checked : ECheckBoxState::Unchecked).OnCheckStateChanged(this, &SCreateBPMapUI::SetBPLoad).IsFocusable(true)]];
}

// Listview MouseCilcked Action ShotMap
void SCreateBPMapUI::LinkSelection_ClickL(TSharedPtr<FString> Item)
{
	TArray<TSharedPtr<FString>> CurrentSelcetedItems = ListViewShot->GetSelectedItems();

	ListViewScene->ClearSelection();

	for (auto SelectedItem : CurrentSelcetedItems)
	{
		int32 index;
		if (ItemsMap.Find(SelectedItem, index))
			ListViewScene->SetItemSelection(ItemsScene[index], true, ESelectInfo::Direct);
	}
	ListViewScene->RebuildList();
	ListViewScene->RequestListRefresh();
}

// Listview MouseCilcked Action SceneMap
void SCreateBPMapUI::LinkSelection_ClickR(TSharedPtr<FString> Item)
{
	TArray<TSharedPtr<FString>> CurrentSelcetedItems = ListViewScene->GetSelectedItems();

	ListViewShot->ClearSelection();

	for (auto SelectedItem : CurrentSelcetedItems)
	{
		int32 index;
		if (ItemsScene.Find(SelectedItem, index))
			ListViewShot->SetItemSelection(ItemsMap[index], true, ESelectInfo::Direct);
	}

	ListViewShot->RebuildList();
	ListViewShot->RequestListRefresh();
}

void SCreateBPMapUI::BPItemsAddContent(FString ProjectPath)
{
	ItemsAllBP.Reset();

	TArray<FString> ReferenceBP = FLoadBP::GetReferenceBP(ProjectPath);
	if (ReferenceBP.Num())
	{
		for (auto BP : ReferenceBP)
		{
			ItemsAllBP.Add(MakeShareable(new FBPInfo(BP)));
		}
	}
	ItemsAllBP.Sort([](TSharedPtr<FBPInfo> A, TSharedPtr<FBPInfo> B)
					{

		FString BPNameA = A->BPPackage;
		FString BPNameB = B->BPPackage;

		TArray<FString> NameSplitA, NameSplitB;
		BPNameA.ParseIntoArray(NameSplitA, TEXT("/"), true);
		BPNameB.ParseIntoArray(NameSplitB, TEXT("/"), true);

		if (NameSplitA.Num())
			BPNameA = NameSplitA[NameSplitA.Num() - 1];
		if (NameSplitB.Num())
			BPNameB = NameSplitB[NameSplitB.Num() - 1];

		int32 Length = BPNameA.Len() <= BPNameB.Len() ? BPNameA.Len() : BPNameB.Len();

		for (int32 i = 0; i < Length; i++)
			if (BPNameA.GetCharArray()[i] != BPNameB.GetCharArray()[i])
				return BPNameA.GetCharArray()[i] < BPNameB.GetCharArray()[i];
		return BPNameA.Len() < BPNameB.Len(); });

	ListViewAllBP->RequestListRefresh();
}

FReply SCreateBPMapUI::ShowBPAssets()
{
	FString ProjectPath = TextPorject->GetText().ToString();

	BPItemsAddContent(ProjectPath);

	ListViewAllBP->RequestListRefresh();

	return FReply::Handled();
}

// Button Action ChangeSceneMap
FReply SCreateBPMapUI::ChangeSceneMap()
{
	TArray<FString> OpenFilenames;
	IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

	bool bOpen = false;

	FString SceneMap;

	if (DesktopPlatform)
	{
		FString ExtensionStr;
		ExtensionStr += TEXT("Map (*.umap)|*.umap|");

		bOpen = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			NSLOCTEXT("MapCreateTool", "", "Choose Scene Map").ToString(),
			*DefaultOpenFileDir,
			TEXT(""),
			*ExtensionStr,
			EFileDialogFlags::None,
			OpenFilenames);
	}

	if (bOpen)
	{
		DefaultOpenFileDir = FPaths::GetPath(OpenFilenames[0]);
		SceneMap = OpenFilenames[0];

		TArray<TSharedPtr<FString>> CurrentSelcetedItems = ListViewScene->GetSelectedItems();

		if (SceneMap.Contains(FPaths::ProjectContentDir()))
		{
			FString DisplaySceneMap = FConvertPath::ToRelativePath(SceneMap, false);
			for (auto Item : CurrentSelcetedItems)
			{
				*Item = DisplaySceneMap;
			}
		}

		ListViewScene->RebuildList();
		ListViewScene->RequestListRefresh();
	}

	return FReply::Handled();
}

// Button  Action RestMap
FReply SCreateBPMapUI::RestSceneMap()
{
	TArray<TSharedPtr<FString>> CurrentSelcetedItems = ListViewScene->GetSelectedItems();
	if (CurrentSelcetedItems.Num())
	{
		for (auto SelectedItem : CurrentSelcetedItems)
		{
			int32 Index;
			ItemsScene.Find(SelectedItem, Index);
			//	*SelectedItem = "Template_Default";
			*ItemsScene[Index] = "Template_Default";
		}
		ListViewScene->RebuildList();
		ListViewScene->RequestListRefresh();
	}
	return FReply::Handled();
}

// Button Action ShowMapInfo
FReply SCreateBPMapUI::ShowMapInfo()
{

	FString ProjectPath = TextPorject->GetText().ToString();
	FString ProjectFullPath = FPaths::ProjectContentDir() + "/" + ProjectPath;

	ItemsAddContent(ProjectFullPath);

	RefreshList();

	return FReply::Handled();
}

// Button Action SaveSetting
FReply SCreateBPMapUI::SaveSetting()
{
	WriteSetting();
	return FReply::Handled();
}

// Write Setting.json
void SCreateBPMapUI::WriteSetting()
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
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
	FArchive *JsonFile = IFileManager::Get().CreateFileWriter(*JsonFileName, FILEWRITE_EvenIfReadOnly);

	if (JsonFile)
	{
		TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(JsonFile);
		FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
		JsonFile->Close();
	}
}

// If Setting.json Exist , Loading Settings
void SCreateBPMapUI::LoadSetting()
{
	FString JsonFileName = FPaths::ProjectDir() / TEXT("Settings.json");
	if (IFileManager::Get().FileExists(*JsonFileName))
	{
		FArchive *JsonFile = IFileManager::Get().CreateFileReader(*JsonFileName);
		TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
		bool bJsonLoaded = false;

		TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonFile);
		bJsonLoaded = FJsonSerializer::Deserialize(Reader, RootObject);
		JsonFile->Close();

		if (bJsonLoaded)
		{
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
FReply SCreateBPMapUI::OpenProjcetDir()
{
	FString OpenDirectory;
	IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

	bool bOpen = false;

	if (DesktopPlatform)
	{
		bOpen = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			NSLOCTEXT("MapCreateTool", "", "").ToString(),
			*FPaths::ProjectContentDir(),
			OpenDirectory);
	}

	if (bOpen)
	{

		if (OpenDirectory.Contains(FPaths::ProjectContentDir()))
		{
			FString ProjectPath = OpenDirectory.Replace(*FPaths::ProjectContentDir(), TEXT(""), ESearchCase::IgnoreCase);
			TextPorject->SetText(FText::FromString(ProjectPath));

			ItemsAddContent(OpenDirectory);
			BPItemsAddContent(ProjectPath);

			RefreshList();
		}
	}

	return FReply::Handled();
}

// Button Action Open FbxDir
FReply SCreateBPMapUI::OpenFbxDir()
{
	FString OpenDirectoryName;
	IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

	if (IFileManager::Get().DirectoryExists(*TextFbxDir->GetText().ToString()))
		DefaultOpenFbxDir = TextFbxDir->GetText().ToString();

	bool bOpen = false;

	if (DesktopPlatform)
	{
		bOpen = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			NSLOCTEXT("MapCreateTool", "", "").ToString(),
			*DefaultOpenFbxDir,
			OpenDirectoryName);
	}

	if (bOpen)
	{
		TextFbxDir->SetText(FText::FromString(OpenDirectoryName));
		DefaultOpenFbxDir = OpenDirectoryName;
	}
	return FReply::Handled();
}

// Button Action OpenSceneMap
FReply SCreateBPMapUI::OpenSceneMap()
{
	TArray<FString> OpenFilenames;
	IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();

	bool bOpen = false;

	FString SceneMap;

	if (DesktopPlatform)
	{
		FString ExtensionStr;
		ExtensionStr += TEXT("Map (*.umap)|*.umap|");

		bOpen = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			NSLOCTEXT("MapCreateTool", "", "").ToString(),
			*DefaultOpenFileDir,
			TEXT(""),
			*ExtensionStr,
			EFileDialogFlags::None,
			OpenFilenames);
	}

	if (bOpen)
	{
		DefaultOpenFileDir = FPaths::GetPath(OpenFilenames[0]);
		SceneMap = OpenFilenames[0];

		FString DisplaySceneMap;

		if (SceneMap.Contains(FPaths::ProjectContentDir()))
		{
			DisplaySceneMap = FConvertPath::ToRelativePath(SceneMap, false);
			for (auto Item : ItemsScene)
			{
				*Item = DisplaySceneMap;
			}
		}

		TextSceneMap->SetText(FText::FromString(DisplaySceneMap));
		ListViewScene->RebuildList();
		ListViewScene->RequestListRefresh();
	}

	return FReply::Handled();
}

// Check Map Exists
bool SCreateBPMapUI::MapExist(bool SaveInMap, FString Shot, FString MapName)
{
	bool bMapExist = false;
	FString SaveMapPath;
	if (SaveInMap)
		SaveMapPath = FPaths::ProjectContentDir() + TextPorject->GetText().ToString() + "/map";
	else
		SaveMapPath = FPaths::ProjectContentDir() + TextPorject->GetText().ToString() + "/" + Shot;

	FString MapFullPath = SaveMapPath + "/" + MapName + ".umap";

	if (IFileManager::Get().DirectoryExists(*SaveMapPath))
	{
		bMapExist = IFileManager::Get().FileExists(*MapFullPath);
	}
	return bMapExist;
}

// Set Map Name
FString SCreateBPMapUI::SetMapName(FString Shot)
{
	FString ProjectName = TextPorject->GetText().ToString();
	FString MapName;
	TArray<FString> NameSplit;
	ProjectName.ParseIntoArray(NameSplit, TEXT("_"), true);
	if (NameSplit.Num())
		MapName = NameSplit[0] + "_" + TextPorject->GetText().ToString().Right(3) + "_" + Shot.Right(3) + "_VFX";
	return MapName;
}

// Add Map Info to Items Array
void SCreateBPMapUI::ItemsAddContent(FString ProjectPath)
{

	ItemsMap.Reset();
	ItemsShot.Reset();
	ItemsScene.Reset();

	TArray<FString> SubDirs;
	FString FinalPath = ProjectPath + "/" + TEXT("*");
	IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);
	for (auto ShotDir : SubDirs)
	{
		if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase))
		{
			FString MapName = SetMapName(ShotDir);

			if (!MapExist(IsSaveInMap, ShotDir, MapName))
			{
				ItemsMap.Add(MakeShareable(new FString(MapName)));
				ItemsShot.Add(MakeShareable(new FString(ShotDir)));
				ItemsScene.Add(MakeShareable(new FString(TextSceneMap->GetText().ToString())));
			}
		}
	}
}

void SCreateBPMapUI::SaveInMap(ECheckBoxState State)
{
	IsSaveInMap = CheckBoxMap->IsChecked();
	if (IsSaveInMap)
		CheckBoxShot->SetIsChecked(ECheckBoxState::Unchecked);
	else
		CheckBoxShot->SetIsChecked(ECheckBoxState::Checked);

	ItemsUpdateContent();

	RefreshList();
}

void SCreateBPMapUI::SaveInShot(ECheckBoxState State)
{
	IsSaveInMap = !CheckBoxShot->IsChecked();
	if (IsSaveInMap)
		CheckBoxMap->SetIsChecked(ECheckBoxState::Checked);
	else
		CheckBoxMap->SetIsChecked(ECheckBoxState::Unchecked);

	ItemsUpdateContent();

	RefreshList();
}

void SCreateBPMapUI::SetBPLoad(ECheckBoxState State)
{
	TArray<TSharedPtr<FBPInfo>> CurrentSelcetedItems = ListViewAllBP->GetSelectedItems();

	if (CurrentSelcetedItems.Num())
	{
		for (auto Item : CurrentSelcetedItems)
		{
			/*if (Item->bLoaded)
				Item->bLoaded = false;
			else
				Item->bLoaded = true;*/
			if (State == ECheckBoxState::Checked)
				Item->bLoaded = true;
			else
				Item->bLoaded = false;
		}
	}

	RefreshBPList();
}

void SCreateBPMapUI::SetBPAnimStart1(const FText &InText, ETextCommit::Type Type)
{
	TArray<TSharedPtr<FBPInfo>> CurrentSelcetedItems = ListViewAllBP->GetSelectedItems();
	if (CurrentSelcetedItems.Num())
	{
		for (auto Item : CurrentSelcetedItems)
			if (FCString::Atoi(*InText.ToString()) != 0)
				Item->StartFrame = FCString::Atoi(*InText.ToString());
	}

	RefreshBPList();
}

void SCreateBPMapUI::RefreshBPList()
{
	ListViewAllBP->RebuildList();
	ListViewAllBP->RequestListRefresh();
}

void SCreateBPMapUI::RefreshList()
{
	ListViewShot->RebuildList();
	ListViewShot->RequestListRefresh();
	ListViewScene->RebuildList();
	ListViewScene->RequestListRefresh();
}

void SCreateBPMapUI::ItemsUpdateContent()
{
	TArray<TSharedPtr<FString>> ItemsMapNew, ItemsShotNew, ItemsSceneNew;

	TArray<FString> SubDirs;
	FString FinalPath = FPaths::ProjectContentDir() + "/" + TextPorject->GetText().ToString() + "/" + TEXT("*");
	IFileManager::Get().FindFiles(SubDirs, *FinalPath, false, true);
	for (auto ShotDir : SubDirs)
	{
		if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase))
		{
			FString MapName = SetMapName(ShotDir);
			if (!MapExist(IsSaveInMap, ShotDir, MapName))
			{
				bool bFind = false;
				int32 Index;
				for (auto Item1 : ItemsMap)
				{
					if (MapName == *Item1)
					{
						bFind = true;
						ItemsMap.Find(Item1, Index);
						break;
					}
				}

				if (bFind)
				{
					ItemsMapNew.Add(ItemsMap[Index]);
					ItemsShotNew.Add(ItemsShot[Index]);
					ItemsSceneNew.Add(ItemsScene[Index]);
				}
				else
				{
					ItemsMapNew.Add(MakeShareable(new FString(MapName)));
					ItemsShot.Add(MakeShareable(new FString(ShotDir)));
					ItemsSceneNew.Add(MakeShareable(new FString(TextSceneMap->GetText().ToString())));
				}
			}
		}
	}

	ItemsMap = ItemsMapNew;
	ItemsScene = ItemsSceneNew;
}

FReply SCreateBPMapUI::CreateMap()
{
	FString ProjectPath, FbxCameraPath;

	FbxCameraPath = TextFbxDir->GetText().ToString();
	ProjectPath = TextPorject->GetText().ToString();

	if (ItemsMap.Num() == 0)
	{
		FString ProjectFullPath = FPaths::ProjectContentDir() + "/" + ProjectPath;
		ItemsAddContent(ProjectFullPath);
	}

	/*TArray<TSharedPtr<FString>> ItemsLoadedBP;
	for (auto BP : ItemsAllBP)
	{
		if (BP->bLoaded)
			ItemsLoadedBP.Add(MakeShareable(new FString(BP->BPPackage)));
	}*/

	for (int i = 0; i < ItemsMap.Num(); i++)
	{
		FCreateMap::CreateBPMap(ProjectPath, FbxCameraPath, *ItemsShot[i], *ItemsMap[i], *ItemsScene[i], IsSaveInMap, ItemsAllBP);
	}

	ItemsUpdateContent();
	RefreshList();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);