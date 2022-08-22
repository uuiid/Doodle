#pragma once

#include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "Reply.h"
#include "Paths.h"
#include "SUserWidget.h"
#include "MapGenerator/DataType.h"


class SCleanUpMapUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCleanUpMapUI)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);


	FString DefaultOpenProjectDir = FPaths::GameContentDir();
	FString DefaultOpenOutputDir = FPaths::GameContentDir();


	TArray<TSharedPtr<FString>> ItemsMap, ItemsMapPackage,ItemsSequence, ItemsShot, ItemsEmpty;
	TArray<TSharedPtr<FMapInfo>> ItemsMapInfo;

	TArray<TArray<TSharedPtr<FString>>> ItemsAllSequence, ItemsAllSequencePackage;
	TSharedPtr<class SEditableTextBox> TextPorject, TextOutput;
	TSharedPtr<class SCheckBox> CheckBoxShot, CheckBoxMap, CheckBoxOverwrite;

	TSharedPtr<class SListView<TSharedPtr<FString>>>ListViewSequence, ListViewMap;
	TSharedPtr<class SListView<TSharedPtr<FMapInfo>>>ListViewMapInfo;

	TSharedRef<ITableRow> GenerateList(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateMapInfoList(TSharedPtr<struct FMapInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void ShowSequence(TSharedPtr<FMapInfo> Item, ESelectInfo::Type Direct);
	void SetMapLoad(ECheckBoxState State);

	bool IsSaveInMap = true;
	void ItemsUpdateContent();

	FReply OpenProjcetDir();
	FReply OpenOutputDir();

	void SaveInMap(ECheckBoxState State);
	void SaveInShot(ECheckBoxState State);

	FReply CleanUpMap();


};