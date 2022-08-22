#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Misc/Paths.h"
#include "Widgets/SUserWidget.h"
#include "MapGenerator/DataType.h"

class SCleanUpFolderUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCleanUpFolderUI)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments &InArgs);

	FString DefaultOpenProjectDir = FPaths::ProjectContentDir();
	FString DefaultOpenOutputDir = FPaths::ProjectContentDir();

	TArray<TSharedPtr<FFolderInfo>> ItemsFolderInfo;

	TSharedPtr<class SEditableTextBox> TextPorject, TextOutput;

	TSharedPtr<class SListView<TSharedPtr<FFolderInfo>>> ListViewFolderInfo;

	TSharedRef<ITableRow> GenerateFolderInfoList(TSharedPtr<struct FFolderInfo> Item, const TSharedRef<STableViewBase> &OwnerTable);
	void SetByFolder(ECheckBoxState State);
	void SetByAssetType(ECheckBoxState State);

	void ItemsUpdateContent();

	FReply OpenProjcetDir();
	FReply OpenOutputDir();

	TArray<FString> FoundDirs;
	void CleanUpFolder(FString &SourceFolder, FString &DestFolder, bool bFolder);
	FReply CleanUpFolders();

	TArray<FString> FindAllSubDirs(FString Folder);
	TArray<FString> AllSubDirsInFolder;
};