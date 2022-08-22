#pragma once

#include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "Reply.h"
#include "SUserWidget.h"
#include "MapGenerator/DataType.h"

class SCreateBPMapUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateBPMapUI)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments &InArgs);

	TSharedPtr<class SCheckBox> CheckBoxShot, CheckBoxMap, CheckBoxBP;
	void LinkSelection_ClickL(TSharedPtr<FString> Item);
	void LinkSelection_ClickR(TSharedPtr<FString> Item);

	FString DefaultOpenFileDir = FPaths::ProjectContentDir();
	FString DefaultOpenFbxDir = "C:/";

	TSharedPtr<class SListView<TSharedPtr<FString>>> ListViewShot, ListViewScene;
	TArray<TSharedPtr<FString>> ItemsMap, ItemsScene, ItemsShot, ItemsEmpty;
	TSharedRef<ITableRow> GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase> &OwnerTable);

	TSharedPtr<class SListView<TSharedPtr<struct FBPInfo>>> ListViewAllBP;
	TArray<TSharedPtr<struct FBPInfo>> ItemsAllBP;
	TSharedRef<ITableRow> GenerateBPList(TSharedPtr<struct FBPInfo> BPItem, const TSharedRef<STableViewBase> &OwnerTable);
	void SetBPLoad(ECheckBoxState State);
	void SetBPAnimStart1(const FText &Text, ETextCommit::Type Type);

	TSharedPtr<class SEditableTextBox> TextPorject, TextFbxDir, TextSceneMap;

	bool IsSaveInMap = true;
	bool MapExist(bool SaveInPath, FString Shot, FString MapName);
	FString SetMapName(FString Shot);
	void ItemsAddContent(FString ProjectPath);
	void ItemsUpdateContent();
	void BPItemsAddContent(FString ProjectPath);

	FReply ShowBPAssets();

	FReply ChangeSceneMap();
	FReply RestSceneMap();
	FReply ShowMapInfo();

	FReply OpenProjcetDir();
	FReply OpenFbxDir();
	FReply OpenSceneMap();

	// Save and Load Setting.json
	void WriteSetting();
	void LoadSetting();
	FReply SaveSetting();

	void SaveInMap(ECheckBoxState State);
	void SaveInShot(ECheckBoxState State);

	void RefreshList();
	void RefreshBPList();

	FReply CreateMap();
};
