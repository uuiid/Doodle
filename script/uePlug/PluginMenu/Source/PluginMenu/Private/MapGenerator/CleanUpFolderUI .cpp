#include "MapGenerator/CleanUpFolderUI.h"

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
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
// #include "Private/ContentBrowserSingleton.h"
#include "AssetToolsModule.h"
#include "AssetTools/Public/IAssetTools.h"

#include "MapGenerator/CreateMap.h"
#include "MapGenerator/ConvertPath.h"
#include "MapGenerator/LoadBP.h"
#include "MapGenerator/CreateMapUtils.h"

#define LOCTEXT_NAMESPACE "SCleanUpFolderUI"

void SCleanUpFolderUI::Construct(const FArguments &InArgs)
{
	GEditor->GetSmallFont();

	FSlateFontInfo SlateFontInfoContent = FSlateFontInfo(GEditor->GetSmallFont(), 14);


	FSlateFontInfo{};
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
									.Text(FText::FromString("UE4 Clean Up Folder Tool"))
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
												.OnClicked(this, &SCleanUpFolderUI::OpenProjcetDir)]]

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
												.OnClicked(this, &SCleanUpFolderUI::OpenOutputDir)]]]

					 // Map Save Path

					 // Listview label
					 + SCanvas::Slot()
						   .Position(FVector2D(100, 230))
						   .Size(FVector2D(920, 50))
							   [SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1)
														   [SNew(STextBlock)
																.Font(SlateFontInfoContent)
																.Text(FText::FromString(TEXT("Need Clean up Folders:")))
																.ColorAndOpacity(FLinearColor::Black)]

	]

					 // Listview Content
					 + SCanvas::Slot()
						   .Position(FVector2D(100, 260))
						   .Size(FVector2D(920, 350))
							   [SNew(SBorder)
									.BorderBackgroundColor(FLinearColor(0.75, 0.75, 0.75, 1))
									.BorderImage(new FSlateBrush())
										[

											SNew(SScrollBox).ScrollBarAlwaysVisible(true) + SScrollBox::Slot()
																								[SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1)

																															[SAssignNew(ListViewFolderInfo, SListView<TSharedPtr<FFolderInfo>>)
																																 .ItemHeight(24)
																																 .ListItemsSource(&ItemsFolderInfo)
																																 .SelectionMode(ESelectionMode::Multi)
																																 .OnGenerateRow(this, &SCleanUpFolderUI::GenerateFolderInfoList)]

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
									.OnClicked(this, &SCleanUpFolderUI::CleanUpFolders)]]

	];
}

// Button Action OpenProjectDir
FReply SCleanUpFolderUI::OpenProjcetDir()
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

		if (OpenDirectory.Contains(FPaths::ProjectContentDir()))
		{
			DefaultOpenProjectDir = OpenDirectory;
			TextPorject->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ProjectContentDir(), TEXT(""), ESearchCase::IgnoreCase)));
			ItemsUpdateContent();
		}
	}

	return FReply::Handled();
}

// Button Action Open FbxDir
FReply SCleanUpFolderUI::OpenOutputDir()
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

		if (OpenDirectory.Contains(FPaths::ProjectContentDir()))
		{
			DefaultOpenOutputDir = OpenDirectory;
			TextOutput->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ProjectContentDir(), TEXT(""), ESearchCase::IgnoreCase)));
		}
	}
	return FReply::Handled();
}

TSharedRef<ITableRow> SCleanUpFolderUI::GenerateFolderInfoList(TSharedPtr<FFolderInfo> Item, const TSharedRef<STableViewBase> &OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Padding(2.0f)
			[SNew(SHorizontalBox) + SHorizontalBox::Slot().FillWidth(1.5)[SNew(STextBlock).ColorAndOpacity(FLinearColor(0, 0, 0, 1)).Text(FText::FromString(*Item->FolderPackage))] + SHorizontalBox::Slot().FillWidth(0.3)[SNew(STextBlock).ColorAndOpacity(FLinearColor(0, 0, 0, 1)).Text(FText::FromString("Move Folder"))]

			 + SHorizontalBox::Slot().FillWidth(0.1)
				   [SNew(SCheckBox)
						.ForegroundColor(FLinearColor(0.1, 0.1, 0.2, 1))
						.IsChecked(Item->bFolder ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
						.OnCheckStateChanged(this, &SCleanUpFolderUI::SetByFolder)] +
			 SHorizontalBox::Slot().FillWidth(0.5)

			 + SHorizontalBox::Slot().FillWidth(0.3)
				   [SNew(STextBlock)
						.ColorAndOpacity(FLinearColor(0, 0, 0, 1))
						.Text(FText::FromString("Move Assets"))] +
			 SHorizontalBox::Slot().FillWidth(0.1)
				 [SNew(SCheckBox)
					  .ForegroundColor(FLinearColor(0.1, 0.1, 0.2, 1))
					  .IsChecked(Item->bAssetType ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
					  .OnCheckStateChanged(this, &SCleanUpFolderUI::SetByAssetType)]

			 + SHorizontalBox::Slot().FillWidth(0.5)
			 //		.Font(FSlateFontInfo(GEditor->GetSmallFont(), 12))
	];
}

void SCleanUpFolderUI::SetByFolder(ECheckBoxState State)
{
	TArray<TSharedPtr<FFolderInfo>> CurrentSelcetedItems = ListViewFolderInfo->GetSelectedItems();

	if (CurrentSelcetedItems.Num())
	{
		bool CheckState = CurrentSelcetedItems[0]->bFolder;
		for (auto FolderInfo : CurrentSelcetedItems)
		{
			FolderInfo->bFolder = !CheckState;
		}
	}
	ListViewFolderInfo->RebuildList();
	ListViewFolderInfo->RequestListRefresh();
}

void SCleanUpFolderUI::SetByAssetType(ECheckBoxState State)
{
	TArray<TSharedPtr<FFolderInfo>> CurrentSelcetedItems = ListViewFolderInfo->GetSelectedItems();

	if (CurrentSelcetedItems.Num())
	{
		bool CheckState = CurrentSelcetedItems[0]->bAssetType;
		for (auto FolderInfo : CurrentSelcetedItems)
		{
			FolderInfo->bAssetType = !CheckState;
		}
	}
	ListViewFolderInfo->RebuildList();
	ListViewFolderInfo->RequestListRefresh();
}

void SCleanUpFolderUI::ItemsUpdateContent()
{
	FString Project = TextPorject->GetText().ToString();

	TArray<FString> SplitProject;
	Project.ParseIntoArray(SplitProject, TEXT("/"), ESearchCase::IgnoreCase);

	if (SplitProject.Num() > 1)
	{
		ItemsFolderInfo.Reset();

		// Find Folders in Content Dir
		TArray<FString> SubDirsInContent, SubDirsInProject, SubDirsInEpisode;
		FString ParentFolder;

		ParentFolder = FPaths::ProjectContentDir() + "*";
		IFileManager::Get().FindFiles(SubDirsInContent, *ParentFolder, false, true);
		if (SubDirsInContent.Num())
			for (auto Dir : SubDirsInContent)
			{
				if (Dir != SplitProject[0] && Dir != "Collections" && Dir != "Developers")
					ItemsFolderInfo.Add(MakeShareable(new FFolderInfo(Dir)));
			}

		// Find Folders in Project Folder
		ParentFolder = FPaths::ProjectContentDir() + SplitProject[0] + "/*";
		IFileManager::Get().FindFiles(SubDirsInProject, *ParentFolder, false, true);
		if (SubDirsInProject.Num())
			for (auto Dir : SubDirsInProject)
			{
				if (Dir != SplitProject[1])
					ItemsFolderInfo.Add(MakeShareable(new FFolderInfo(SplitProject[0] + "/" + Dir, false, true)));
			}

		// Find Folder in Episode Folder
		ParentFolder = FPaths::ProjectContentDir() + SplitProject[0] + "/" + SplitProject[1] + "/*";
		IFileManager::Get().FindFiles(SubDirsInEpisode, *ParentFolder, false, true);
		if (SubDirsInEpisode.Num())
		{
			for (auto Dir : SubDirsInEpisode)
			{
				if (!Dir.StartsWith(TEXT("SC"), ESearchCase::IgnoreCase) && Dir.ToLower() != "map")
					ItemsFolderInfo.Add(MakeShareable(new FFolderInfo(SplitProject[0] + "/" + SplitProject[1] + "/" + Dir, false, true)));
			}
		}

		ListViewFolderInfo->RebuildList();
		ListViewFolderInfo->RequestListRefresh();
	}
}

TArray<FString> SCleanUpFolderUI::FindAllSubDirs(FString Folder)
{
	TArray<FString> SubDirs;
	FString FolderPath = Folder + "/*";
	IFileManager::Get().FindFiles(SubDirs, *FolderPath, false, true);
	if (SubDirs.Num())
		for (auto Dir : SubDirs)
		{
			AllSubDirsInFolder.Add(Folder + "/" + Dir);
			FindAllSubDirs(Folder + "/" + Dir);
		}
	return AllSubDirsInFolder;
}

void SCleanUpFolderUI::CleanUpFolder(FString &SourceFolder, FString &DestFolder, bool bFolder)
{
	FCreateMapUtils::MoveAssetInFolder(SourceFolder, DestFolder, bFolder);
	FCreateMapUtils::FixRedirectorsInFolder(SourceFolder);
}

FReply SCleanUpFolderUI::CleanUpFolders()
{
	FString OutPath = TextOutput->GetText().ToString();
	FString Project = TextPorject->GetText().ToString();

	FString ContentFolder = "";
	if (OutPath != "")
		FCreateMapUtils::MoveAssetInCurrentFolder(ContentFolder, OutPath, false);

	if (ItemsFolderInfo.Num() && OutPath != "")
	{
		for (auto FolderInfo : ItemsFolderInfo)
			if (FolderInfo->bFolder || FolderInfo->bAssetType)
				CleanUpFolder(FolderInfo->FolderPackage, OutPath, FolderInfo->bFolder);

		FCreateMapUtils::FixRedirectorsInFolder(ContentFolder);
		FCreateMapUtils::SaveMaterialsInFolder(Project);

		FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry &AssetTool = AssetRegistryModule.Get();
		for (auto FolderInfo : ItemsFolderInfo)
		{
			if (FolderInfo->bFolder || FolderInfo->bAssetType)
			{
				FString FolderPath = "/Game/" + FolderInfo->FolderPackage;
				AssetTool.RemovePath(*FolderPath);
				IFileManager::Get().DeleteDirectory(*FConvertPath::ToAbsolutePath(FolderPath), false, true);
			}
		}

		ItemsUpdateContent();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *OpenDirectory);