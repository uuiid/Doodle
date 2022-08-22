#pragma once

#include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "Reply.h"
#include "SUserWidget.h"

class SMutiSceneMapUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMutiSceneMapUI)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void LinkSelection_ClickL(TSharedPtr<FString> Item);
	void LinkSelection_ClickR(TSharedPtr<FString> Item);
	void LinkSelectionL(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	


	FString DefaultOpenFileDir = FPaths::GameContentDir();
	FString DefaultOpenFbxDir = "C:/";

	TSharedRef<ITableRow> GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);

	TSharedPtr<class SListView<TSharedPtr<FString>>> ListViewShot,ListViewScene;
	TArray<TSharedPtr<FString>> ItemsMap, ItemsScene, ItemsShot,ItemsEmpty;

	TSharedPtr<class SEditableTextBox> TextPorject, TextFbxDir, TextSceneMap;
	TSharedPtr<class SCheckBox> CheckBoxShot, CheckBoxMap;


	bool IsSaveInMap = true;
	bool MapExist(bool SaveInPath,FString Shot,FString MapName);
	FString SetMapName(FString Shot);
	void ItemsAddContent(FString ProjectPath);
	void ItemsUpdateContent();

	FReply ChangeSceneMap();
	FReply RestSceneMap();
	FReply ShowMapInfo();
	

	FReply OpenProjcetDir();
	FReply OpenFbxDir();
	FReply OpenSceneMap();

	//Save and Load Setting.json
	void WriteSetting();
	void LoadSetting();
	FReply SaveSetting();

	
	void SaveInMap(ECheckBoxState State);
	void SaveInShot(ECheckBoxState State);
	void RefreshList();

	FReply CreateMap();


};