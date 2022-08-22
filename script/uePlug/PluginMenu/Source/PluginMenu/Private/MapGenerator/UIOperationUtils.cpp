#include "MapGenerator/UIOperationUtils.h"

#include "SlateCore.h"
#include "SEditableTextBox.h"

#include "Engine.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "SlateApplication.h"

#include "MapGenerator/ConvertPath.h"

#define LOCTEXT_NAMESPACE "FUIOperationUtils"


void FUIOperationUtils::ChooseProjectFolderAndDisplay(TSharedPtr<SEditableTextBox> TextBox,FString& DefaultOpenDirectory)
{
	FString OpenDirectory;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	bool bOpen = false;

	if (DesktopPlatform)
	{
		bOpen = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			NSLOCTEXT("MapCreateTool", "", "").ToString(),
			*DefaultOpenDirectory,
			OpenDirectory
		);
	}

	if (bOpen)
	{

		if (OpenDirectory.Contains(FPaths::ProjectContentDir()))
		{
			DefaultOpenDirectory = OpenDirectory;
			TextBox->SetText(FText::FromString(OpenDirectory.Replace(*FPaths::ProjectContentDir(), TEXT("/Game/"), ESearchCase::IgnoreCase)));
		}
	}
}

void FUIOperationUtils::ChooseProjectFileAndDisplay(TSharedPtr<SEditableTextBox> TextBox, FString& DefaultOpenDirectory,FString& FileType)
{
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (IFileManager::Get().DirectoryExists(*TextBox->GetText().ToString()))
		DefaultOpenDirectory = TextBox->GetText().ToString();

	bool bOpen = false;

	if (DesktopPlatform)
	{
		FString ExtensionStr;
		if (FileType.ToUpper() == "MAP")
			ExtensionStr += TEXT("Map (*.umap)|*.umap|");
		else
			ExtensionStr += TEXT("UAsset (*.uasset)|*.uasset|");

		bOpen = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			NSLOCTEXT("MapCreateTool", "", "").ToString(),
			*DefaultOpenDirectory,
			TEXT(""),
			*ExtensionStr,
			EFileDialogFlags::None,
			OpenFilenames
		);
	}

	if (bOpen)
	{
		DefaultOpenDirectory = FPaths::GetPath(OpenFilenames[0]);

		if (OpenFilenames[0].Contains(FPaths::ProjectContentDir()))
		{
			FString RelativeFilePath = FConvertPath::ToRelativePath(OpenFilenames[0], false);
			TextBox->SetText(FText::FromString(RelativeFilePath));
		}

	}
}

TArray<TSharedPtr<FMapInfo>> FUIOperationUtils::FindMapsInProject(FString& RelativeProjectPath, bool bInMap)
{

	TArray<TSharedPtr<FMapInfo>> MapInfo;
	// Find UWorld  In Directory
	UObjectLibrary * WorldLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, GIsEditor);
	if (WorldLibrary)
	{
		WorldLibrary->AddToRoot();
	}

	FString WorldAssetsPath;

	if (bInMap)
	{
		WorldAssetsPath = RelativeProjectPath + "/map";
		WorldLibrary->LoadAssetDataFromPath(*WorldAssetsPath);

		TArray<FAssetData> WorldAssetsData;
		WorldLibrary->GetAssetDataList(WorldAssetsData);

		for (auto World : WorldAssetsData)
		{
			MapInfo.Add(MakeShareable(new FMapInfo(World.PackageName.ToString())));
		}
	}
	else
	{
		TArray<FString> SubDirs;
		FString AbsoluteProjectPath = FConvertPath::ToAbsolutePath(RelativeProjectPath) + "/" + TEXT("*");
		IFileManager::Get().FindFiles(SubDirs, *AbsoluteProjectPath, false, true);
		for (auto ShotDir : SubDirs)
		{
			if (ShotDir.StartsWith("SC", ESearchCase::IgnoreCase))
			{
				WorldAssetsPath = RelativeProjectPath + "/" + ShotDir;
				WorldLibrary->LoadAssetDataFromPath(*WorldAssetsPath);

				TArray<FAssetData> WorldAssetsData;
				WorldLibrary->GetAssetDataList(WorldAssetsData);

				for (auto World : WorldAssetsData)
				{
					MapInfo.Add(MakeShareable(new FMapInfo(World.PackageName.ToString())));
				}
			}
		}

	}

	return MapInfo;
	
}